import os
import logging
from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field
from typing import List
from generator import AudioCraftGenerator

# Silence spammy GET /health and GET /task/ requests from Uvicorn's terminal console logging
class HealthCheckFilter(logging.Filter):
    def filter(self, record: logging.LogRecord) -> bool:
        msg = record.getMessage()
        return "GET /health" not in msg and "GET /task/" not in msg

logging.getLogger("uvicorn.access").addFilter(HealthCheckFilter())

@asynccontextmanager
async def lifespan(app: FastAPI):
    import asyncio
    # Silence spammy standard library asyncio ConnectionResetError (WinError 10054) on Windows
    loop = asyncio.get_running_loop()
    def custom_exception_handler(loop, context):
        exc = context.get("exception")
        if isinstance(exc, ConnectionResetError) or (exc and "10054" in str(exc)):
            return  # Suppress traceback
        loop.default_exception_handler(context)
    loop.set_exception_handler(custom_exception_handler)

    # Warm up / pre-load the default AI model in a background thread so the first generation is instant
    import threading
    def preload():
        print("[InGen Server] Pre-loading default model 'audiogen-medium' in background...")
        try:
            generator.get_audiogen_model()
            print("[InGen Server] Default model 'audiogen-medium' pre-loaded successfully. Ready for instant generation!")
        except Exception as e:
            print(f"[InGen Server] Warm-up pre-load error: {e}")
            
    threading.Thread(target=preload, daemon=True).start()
    yield

app = FastAPI(
    title="InGen Audio Generation Server",
    description="Local Python server hosting Meta AudioCraft for the InGen Sampler plugin.",
    version="1.0.0",
    lifespan=lifespan
)

# Core Directories Setup
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Load .env file manually from the server root if it exists
env_path = os.path.join(BASE_DIR, ".env")
if os.path.exists(env_path):
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, val = line.split("=", 1)
                os.environ[key.strip()] = val.strip().strip("'\"")

CACHE_DIR = os.path.join(BASE_DIR, "cache")
os.makedirs(CACHE_DIR, exist_ok=True)

# Mount cache directory for static WAV delivery
app.mount("/cache", StaticFiles(directory=CACHE_DIR), name="cache")

# Initialize AudioCraft Generator Core (Lazy loading inside)
generator = AudioCraftGenerator(CACHE_DIR)

import uuid
import threading
from typing import Dict, Any

# Global task manager to store running generation tasks
tasks: Dict[str, Dict[str, Any]] = {}

# Request Models
class GenerationRequest(BaseModel):
    prompt: str = Field(..., description="Text prompt for the generator.")
    note_count: int = Field(default=5, ge=1, le=24, description="Number of tonal samples to generate.")
    octaves: int = Field(default=2, ge=1, le=5, description="Keyboard range in octaves.")
    foley_count: int = Field(default=5, ge=1, le=20, description="Number of foley samples to generate.")
    temperature: float = Field(default=1.0, ge=0.0, le=2.0, description="Generation temperature.")
    model: str = Field(default="audiogen-medium", description="Selected AI model for generation.")

# Response Models
class SampleMetadata(BaseModel):
    url: str
    filename: str
    midi_root_key: int
    pitch_correction_cents: float = 0.0

class GenerationResponse(BaseModel):
    status: str
    message: str
    samples: List[SampleMetadata]

class TaskAcceptedResponse(BaseModel):
    status: str
    task_id: str

@app.get("/health")
def health_check():
    """Verify that the FastAPI server is running and healthy."""
    return {"status": "healthy", "server": "InGen Server 1.0.0"}

@app.get("/task/{task_id}")
async def get_task_status(task_id: str):
    """Retrieve the status and results of a background generation task."""
    if task_id not in tasks:
        raise HTTPException(status_code=404, detail="Task not found")
    return tasks[task_id]

@app.post("/generate/tonal", response_model=TaskAcceptedResponse)
async def generate_tonal(request: GenerationRequest):
    """
    Spawns tonal sample generation (Layer A) in a background thread and returns instantly.
    """
    task_id = str(uuid.uuid4())
    tasks[task_id] = {"status": "processing", "progress": 0.0, "result": None, "error": None}
    
    def background_generation():
        try:
            # Tonal single-notes should be sustained and hold a single pitch; 6.0s (max 8.0s) is ideal.
            sample_duration = 6.0
            
            generated = generator.generate_tonal(
                prompt=request.prompt,
                count=request.note_count,
                duration=sample_duration,
                temperature=request.temperature,
                model=request.model
            )
            
            # Distribute MIDI root keys logically across the requested octaves starting at Middle C (60)
            base_midi = 60
            step = 12 * request.octaves // max(1, request.note_count - 1) if request.note_count > 1 else 0
            
            response_samples = []
            for i, sample in enumerate(generated):
                midi_note = base_midi + (i * step)
                response_samples.append({
                    "url": f"/cache/{sample['filename']}",
                    "filename": sample['filename'],
                    "midi_root_key": midi_note,
                    "pitch_correction_cents": 0.0
                })
                
            tasks[task_id] = {
                "status": "success",
                "progress": 1.0,
                "result": {
                    "status": "success",
                    "message": f"{request.model} generated {request.note_count} tonal samples successfully.",
                    "samples": response_samples
                },
                "error": None
            }
            print(f"[InGen Server] Task {task_id} (Tonal) finished successfully!")
        except Exception as e:
            import traceback
            traceback.print_exc()
            tasks[task_id] = {
                "status": "failed",
                "progress": 0.0,
                "result": None,
                "error": str(e)
            }
            
    threading.Thread(target=background_generation, daemon=True).start()
    return TaskAcceptedResponse(status="accepted", task_id=task_id)

@app.post("/generate/foley", response_model=TaskAcceptedResponse)
async def generate_foley(request: GenerationRequest):
    """
    Spawns mechanical/foley sample generation (Layer B) in a background thread and returns instantly.
    """
    task_id = str(uuid.uuid4())
    tasks[task_id] = {"status": "processing", "progress": 0.0, "result": None, "error": None}
    
    def background_generation():
        try:
            # Foley clicks are short, non-pitched noise transients; 1.5s (max 3.0s) is ideal.
            foley_duration = 1.5
            
            generated = generator.generate_foley(
                prompt=request.prompt,
                count=request.foley_count,
                duration=foley_duration,
                temperature=request.temperature,
                model=request.model
            )
            
            response_samples = []
            for sample in generated:
                response_samples.append({
                    "url": f"/cache/{sample['filename']}",
                    "filename": sample['filename'],
                    "midi_root_key": -1, # Unpitched mechanical noise
                    "pitch_correction_cents": 0.0
                })
                
            tasks[task_id] = {
                "status": "success",
                "progress": 1.0,
                "result": {
                    "status": "success",
                    "message": f"{request.model} generated {request.foley_count} mechanical samples successfully.",
                    "samples": response_samples
                },
                "error": None
            }
            print(f"[InGen Server] Task {task_id} (Foley) finished successfully!")
        except Exception as e:
            import traceback
            traceback.print_exc()
            tasks[task_id] = {
                "status": "failed",
                "progress": 0.0,
                "result": None,
                "error": str(e)
            }
            
    threading.Thread(target=background_generation, daemon=True).start()
    return TaskAcceptedResponse(status="accepted", task_id=task_id)

@app.post("/shutdown")
async def shutdown_server():
    """
    Endpoint for gracefully shutting down the Python server and freeing GPU VRAM.
    Called when the plugin UI is destroyed.
    """
    import torch
    import asyncio
    print("[InGen Server] Graceful shutdown requested. Releasing GPU VRAM...")
    if torch.cuda.is_available():
        torch.cuda.empty_cache()
    
    # Terminate process in 500ms to allow sending the HTTP response back first
    async def self_terminate():
        await asyncio.sleep(0.5)
        print("[InGen Server] Terminating process.")
        os._exit(0)
        
    asyncio.create_task(self_terminate())
    return {"status": "success", "message": "InGen Server shutting down. CUDA cache cleared."}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="127.0.0.1", port=8071, log_level="info")
