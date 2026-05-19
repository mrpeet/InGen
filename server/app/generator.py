import os
import torch
import torchaudio
import uuid
from audiocraft.models import MusicGen, AudioGen

class AudioCraftGenerator:
    def __init__(self, cache_dir: str):
        self.cache_dir = cache_dir
        # Auto-detect CUDA GPU for hardware acceleration, fallback to CPU
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self._music_model = None
        self._audio_model = None
        print(f"[InGen Generator] Initialized on device: {self.device}")

    def get_music_model(self):
        """Lazy load the MusicGen model for tonal sample generation."""
        if self._music_model is None:
            print("[InGen Generator] Loading 'facebook/musicgen-small' model (MusicGen)...")
            # We use 'musicgen-small' for faster execution and lower VRAM/RAM footprints
            self._music_model = MusicGen.get_pretrained('facebook/musicgen-small', device=self.device)
            print("[InGen Generator] 'facebook/musicgen-small' loaded successfully.")
        return self._music_model

    def get_audio_model(self):
        """Lazy load the AudioGen model for foley/mechanical noise generation."""
        if self._audio_model is None:
            print("[InGen Generator] Loading 'facebook/audiogen-medium' model (AudioGen)...")
            self._audio_model = AudioGen.get_pretrained('facebook/audiogen-medium', device=self.device)
            print("[InGen Generator] 'facebook/audiogen-medium' loaded successfully.")
        return self._audio_model

    def generate_tonal(self, prompt: str, count: int, duration: float, temperature: float) -> list:
        """
        Generates tonal audio samples using MusicGen.
        :param prompt: Text prompt describing the target instrument.
        :param count: Number of variations/samples to generate.
        :param duration: Target duration of each sample in seconds (one-shots are typically 2-3s).
        :param temperature: Sampling temperature controlling random variation.
        :return: List of dicts containing generated filenames and default pitch properties.
        """
        model = self.get_music_model()
        model.set_generation_params(duration=duration, temperature=temperature)

        # Generate 'count' variations in parallel (if batch fits) or sequentially
        # For simplicity and GPU safety under consumer cards, we use parallel batching.
        descriptions = [prompt] * count
        print(f"[InGen Generator] Synthesizing {count} tonal samples using MusicGen...")
        
        # wavs shape: [B, C, T] where B=batch, C=channels, T=samples
        wavs = model.generate(descriptions)
        
        generated_samples = []
        for i, wav in enumerate(wavs):
            filename = f"tonal_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            # torchaudio.save expects [C, T] tensor. 
            # We squeeze out the batch dimension to get [C, T]
            audio_tensor = wav.cpu()
            
            torchaudio.save(filepath, audio_tensor, model.sample_rate)
            print(f"[InGen Generator] Saved tonal sample: {filepath}")
            
            # Initial pitch metadata. 
            # Root MIDI key is default 60 (Middle C). C++ will perform precise YIN analysis.
            generated_samples.append({
                "filename": filename,
                "midi_root_key": 60,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples

    def generate_foley(self, prompt: str, count: int, duration: float, temperature: float) -> list:
        """
        Generates foley/mechanical noise samples using AudioGen.
        :param prompt: Text prompt describing the mechanical click.
        :param count: Number of variations/samples to generate.
        :param duration: Target duration of each foley one-shot in seconds.
        :param temperature: Sampling temperature.
        :return: List of dicts containing generated filenames.
        """
        model = self.get_audio_model()
        model.set_generation_params(duration=duration, temperature=temperature)
        
        descriptions = [prompt] * count
        print(f"[InGen Generator] Synthesizing {count} foley samples using AudioGen...")
        
        wavs = model.generate(descriptions)
        
        generated_samples = []
        for i, wav in enumerate(wavs):
            filename = f"foley_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            audio_tensor = wav.cpu()
            
            torchaudio.save(filepath, audio_tensor, model.sample_rate)
            print(f"[InGen Generator] Saved foley sample: {filepath}")
            
            # Foley/FX sounds are unpitched, mapped to MIDI -1
            generated_samples.append({
                "filename": filename,
                "midi_root_key": -1,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples
