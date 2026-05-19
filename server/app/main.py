import os
import uuid
from fastapi import FastAPI, HTTPException, BackgroundTasks
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field
from typing import List, Optional

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
    Endpoint for generating tonal instrument samples (Layer A).
    Currently mocked for Phase 1 testing.
    """
    try:
        # Placeholder logic for Phase 1
        # In Phase 2, this will load the Meta AudioCraft MusicGen model, 
        # synthesize WAV files, perform pitch estimation, and save them in CACHE_DIR.
        
        mock_samples = []
        # Let's distribute notes across octaves starting around Middle C (MIDI 60)
        base_midi = 60
        step = 12 * request.octaves // max(1, request.note_count - 1) if request.note_count > 1 else 0
        
        for i in range(request.note_count):
            midi_note = base_midi + (i * step)
            filename = f"tonal_mock_{uuid.uuid4().hex[:8]}_note_{midi_note}.wav"
            filepath = os.path.join(CACHE_DIR, filename)
            
            # Create a 1-second silent WAV file placeholder for Phase 1 network testing
            create_mock_wav(filepath, frequency=440.0)
            
            mock_samples.append(SampleMetadata(
                url=f"/cache/{filename}",
                filename=filename,
                midi_root_key=midi_note,
                pitch_correction_cents=0.0
            ))
            
        return GenerationResponse(
            status="success",
            message="Mock tonal samples generated successfully.",
            samples=mock_samples
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/generate/foley", response_model=GenerationResponse)
async def generate_foley(request: GenerationRequest):
    """
    Endpoint for generating mechanical/foley sounds (Layer B).
    Currently mocked for Phase 1 testing.
    """
    try:
        # Placeholder logic for Phase 1
        # In Phase 2, this will run AudioGen to generate mechanical/foley one-shots.
        
        mock_samples = []
        for i in range(request.foley_count):
            filename = f"foley_mock_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(CACHE_DIR, filename)
            
            create_mock_wav(filepath, frequency=100.0)
            
            mock_samples.append(SampleMetadata(
                url=f"/cache/{filename}",
                filename=filename,
                midi_root_key=-1, # Foley is unpitched
                pitch_correction_cents=0.0
            ))
            
        return GenerationResponse(
            status="success",
            message="Mock foley samples generated successfully.",
            samples=mock_samples
        )
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

def create_mock_wav(filepath: str, frequency: float):
    """Creates a basic 1-second silent/sine PCM WAV file for initial socket-polling testing."""
    import wave
    import struct
    import math

    sample_rate = 44100.0
    duration = 1.0 # second
    num_samples = int(sample_rate * duration)
    
    with wave.open(filepath, 'w') as wav_file:
        # Mono, 2-byte width (16-bit), 44.1kHz
        wav_file.setparams((1, 2, int(sample_rate), num_samples, 'NONE', 'not compressed'))
        
        # Write basic quiet sine wave
        for i in range(num_samples):
            t = float(i) / sample_rate
            # 0.1 amplitude sine wave
            val = int(0.1 * 32767.0 * math.sin(2.0 * math.pi * frequency * t))
            data = struct.pack('<h', val)
            wav_file.writeframesraw(data)
