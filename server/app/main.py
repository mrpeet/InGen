import os
from fastapi import FastAPI, HTTPException
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field
from typing import List
from server.app.generator import AudioCraftGenerator

app = FastAPI(
    title="InGen Audio Generation Server",
    description="Local Python server hosting Meta AudioCraft for the InGen Sampler plugin.",
    version="1.0.0"
)

# Core Directories Setup
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CACHE_DIR = os.path.join(BASE_DIR, "cache")
os.makedirs(CACHE_DIR, exist_ok=True)

# Mount cache directory for static WAV delivery
app.mount("/cache", StaticFiles(directory=CACHE_DIR), name="cache")

# Initialize AudioCraft Generator Core (Lazy loading inside)
generator = AudioCraftGenerator(CACHE_DIR)

# Request Models
class GenerationRequest(BaseModel):
    prompt: str = Field(..., description="Text prompt for the generator.")
    note_count: int = Field(default=5, ge=1, le=24, description="Number of tonal samples to generate.")
    octaves: int = Field(default=2, ge=1, le=5, description="Keyboard range in octaves.")
    foley_count: int = Field(default=5, ge=1, le=20, description="Number of foley samples to generate.")
    temperature: float = Field(default=1.0, ge=0.0, le=2.0, description="Generation temperature.")

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

@app.get("/health")
def health_check():
    """Verify that the FastAPI server is running and healthy."""
    return {"status": "healthy", "server": "InGen Server 1.0.0"}

@app.post("/generate/tonal", response_model=GenerationResponse)
async def generate_tonal(request: GenerationRequest):
    """
    Endpoint for generating tonal instrument samples (Layer A) via MusicGen.
    """
    try:
        # One-shot instrument samples typically sound best at 3.0s duration per key
        sample_duration = 3.0
        
        # Synthesize real samples using AudioCraft MusicGen
        generated = generator.generate_tonal(
            prompt=request.prompt,
            count=request.note_count,
            duration=sample_duration,
            temperature=request.temperature
        )
        
        # Distribute MIDI root keys logically across the requested octaves starting at Middle C (60)
        base_midi = 60
        step = 12 * request.octaves // max(1, request.note_count - 1) if request.note_count > 1 else 0
        
        response_samples = []
        for i, sample in enumerate(generated):
            midi_note = base_midi + (i * step)
            response_samples.append(SampleMetadata(
                url=f"/cache/{sample['filename']}",
                filename=sample['filename'],
                midi_root_key=midi_note,
                pitch_correction_cents=0.0 # C++ YIN will determine precise tuning adjustments
            ))
            
        return GenerationResponse(
            status="success",
            message=f"MusicGen generated {request.note_count} tonal samples successfully.",
            samples=response_samples
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/generate/foley", response_model=GenerationResponse)
async def generate_foley(request: GenerationRequest):
    """
    Endpoint for generating mechanical/foley one-shots (Layer B) via AudioGen.
    """
    try:
        # Foley noises are short transient sounds; 1.5s is ideal
        foley_duration = 1.5
        
        # Synthesize foley samples using AudioCraft AudioGen
        generated = generator.generate_foley(
            prompt=request.prompt,
            count=request.foley_count,
            duration=foley_duration,
            temperature=request.temperature
        )
        
        response_samples = []
        for sample in generated:
            response_samples.append(SampleMetadata(
                url=f"/cache/{sample['filename']}",
                filename=sample['filename'],
                midi_root_key=-1, # Unpitched mechanical noise
                pitch_correction_cents=0.0
            ))
            
        return GenerationResponse(
            status="success",
            message=f"AudioGen generated {request.foley_count} mechanical samples successfully.",
            samples=response_samples
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
