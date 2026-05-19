import os
import sys
import importlib.machinery
from types import ModuleType
import torch

# 1. Force environment to ignore memory-efficient attention (bypasses xformers checks)
os.environ["IGNORE_MEMORY_EFFICIENT"] = "1"

# 2. Universal MagicMock class to simulate heavy dependencies (spacy) at import-time
class MagicMock:
    def __init__(self, name="mock"):
        self.__name__ = name
    def __getattr__(self, name):
        if name in ("__file__", "__path__", "__modules__"):
            return None
        return MagicMock(name)
    def __call__(self, *args, **kwargs):
        return MagicMock("call")

# 3. Mock xformers with a real PyTorch scaled_dot_product_attention fallback for MAGNeT
class LowerTriangularMask:
    def __init__(self, *args, **kwargs):
        pass

def memory_efficient_attention(query, key, value, attn_bias=None, p=0.0, scale=None):
    # Transpose [B, T, H, D] to [B, H, T, D] for PyTorch's scaled_dot_product_attention
    q = query.transpose(1, 2)
    k = key.transpose(1, 2)
    v = value.transpose(1, 2)
    
    is_causal = False
    attn_mask = None
    
    if attn_bias is not None:
        if attn_bias.__class__.__name__ == "LowerTriangularMask":
            is_causal = True
        elif torch.is_tensor(attn_bias):
            attn_mask = attn_bias
            
    out = torch.nn.functional.scaled_dot_product_attention(
        q, k, v,
        attn_mask=attn_mask,
        dropout_p=p,
        is_causal=is_causal,
        scale=scale
    )
    # Transpose back to [B, T, H, D]
    return out.transpose(1, 2)

def unbind(input, dim=0):
    return torch.unbind(input, dim=dim)

class MockOpsModule(ModuleType):
    def __getattr__(self, name):
        if name in ("__file__", "__path__", "__modules__"):
            return None
        if name == "LowerTriangularMask":
            return LowerTriangularMask
        if name == "memory_efficient_attention":
            return memory_efficient_attention
        if name == "unbind":
            return unbind
        return MagicMock(name)

mock_xformers = ModuleType("xformers")
mock_xformers.__spec__ = importlib.machinery.ModuleSpec("xformers", None)

mock_ops = MockOpsModule("xformers.ops")
mock_ops.__spec__ = importlib.machinery.ModuleSpec("xformers.ops", None)
mock_xformers.ops = mock_ops

sys.modules["xformers"] = mock_xformers
sys.modules["xformers.ops"] = mock_ops

# 4. Mock spacy and its typical sub-imports
mock_spacy = MagicMock("spacy")
mock_spacy.__spec__ = importlib.machinery.ModuleSpec("spacy", None)
sys.modules["spacy"] = mock_spacy
sys.modules["spacy.tokens"] = mock_spacy
sys.modules["spacy.cli"] = mock_spacy
sys.modules["spacy.lang"] = mock_spacy
sys.modules["spacy.lang.en"] = mock_spacy

import torchaudio
import uuid
from audiocraft.models import MusicGen, AudioGen

class AudioCraftGenerator:
    def __init__(self, cache_dir: str):
        self.cache_dir = cache_dir
        self.device = "cuda" if torch.cuda.is_available() else "cpu"
        self._audio_model = None         # AudioGen Medium
        self._stable_audio_model = None  # Stable Audio Open
        self._magnet_model = None        # Audio-MAGNeT Small
        self._woosh_model = None         # Woosh DFlow
        print(f"[InGen Generator] Initialized on device: {self.device} (Optimized for multi-model loading)")

    def _unload_dormant_models(self, keep_model_type: str):
        """Unload other models from GPU VRAM to prevent OOM on GTX 1070 (8GB)."""
        unloaded = False
        if keep_model_type != "audiogen" and self._audio_model is not None:
            print("[InGen Generator] Unloading 'facebook/audiogen-medium' to free VRAM...")
            self._audio_model = None
            unloaded = True
        if keep_model_type != "stable_audio" and self._stable_audio_model is not None:
            print("[InGen Generator] Unloading 'stabilityai/stable-audio-open-1.0' to free VRAM...")
            self._stable_audio_model = None
            unloaded = True
        if keep_model_type != "magnet" and self._magnet_model is not None:
            print("[InGen Generator] Unloading 'facebook/audio-magnet-small' to free VRAM...")
            self._magnet_model = None
            unloaded = True
        if keep_model_type != "woosh" and getattr(self, "_woosh_model", None) not in [None, "FAILED"]:
            print("[InGen Generator] Unloading 'Sony Woosh DFlow' to free VRAM...")
            self._woosh_model = None
            unloaded = True
        
        if unloaded and torch.cuda.is_available():
            torch.cuda.empty_cache()
            print("[InGen Generator] Dormant models unloaded. CUDA cache cleared.")

    def _derive_foley_prompt(self, prompt: str, trigger_mode: str = "note_on") -> str:
        """
        Dynamically extracts and rewrites a foley/mechanical prompt from a tonal prompt.
        Strips purely musical/tonal terms and injects tactile material transients.
        Supports note_on (fast attack, mechanical hammer click) vs note_off (key release, damper landing).
        """
        prompt_lower = prompt.lower()
        
        # 1. Translate instrument terms to their mechanical/physical equivalents
        mechanical_additions = []
        if trigger_mode == "note_off":
            if any(w in prompt_lower for w in ["piano", "upright", "grand piano", "felt-damped"]):
                mechanical_additions.append("felt damper landing onto string, key release dampener squeak, mechanical piano action release click")
            elif any(w in prompt_lower for w in ["synth", "synthesizer", "button", "switch", "toggle"]):
                mechanical_additions.append("mechanical switch click release, spring pop snap key release")
            elif any(w in prompt_lower for w in ["violin", "cello", "string", "viola", "double bass", "bowed"]):
                mechanical_additions.append("resin bow release scrape, wooden click, string dampening squeak")
            elif any(w in prompt_lower for w in ["guitar", "acoustic", "electric guitar", "bass", "fret"]):
                mechanical_additions.append("string release buzz, fingernail release scrape, fret contact click")
            elif any(w in prompt_lower for w in ["flute", "clarinet", "saxophone", "trumpet", "horn", "wind", "valve"]):
                mechanical_additions.append("metal pad key release click, breath release puff clack")
            else:
                mechanical_additions.append("mechanical key release squeak, soft dampening click, key release click")
        else: # note_on
            if any(w in prompt_lower for w in ["piano", "upright", "grand piano", "felt-damped"]):
                mechanical_additions.append("felt hammer strike, mechanical wooden piano key click, transient hammer impact")
            if any(w in prompt_lower for w in ["violin", "cello", "string", "viola", "double bass", "bowed"]):
                mechanical_additions.append("resin bow hair friction, wooden body knock, scratchy bow scrape")
            if any(w in prompt_lower for w in ["guitar", "acoustic", "electric guitar", "bass", "fret"]):
                mechanical_additions.append("fingernail pluck click, string fret buzz, fingernail nail strike tap")
            if any(w in prompt_lower for w in ["synth", "synthesizer", "button", "switch", "toggle"]):
                mechanical_additions.append("plastic button click, mechanical switch click, toggle snap snap")
            if any(w in prompt_lower for w in ["flute", "clarinet", "saxophone", "trumpet", "horn", "wind", "valve"]):
                mechanical_additions.append("metal pad key click, physical breath hiss, valve snap clack")
            if any(w in prompt_lower for w in ["drum", "snare", "kick", "percussion", "rim"]):
                mechanical_additions.append("drumstick rimshot wood click, skin transient slap, mechanical pedal tap")
            
        # If no specific instrument matched, add generic physical mechanics
        if not mechanical_additions:
            if trigger_mode == "note_off":
                mechanical_additions.append("mechanical release click, tactile key release, soft damper landing click")
            else:
                mechanical_additions.append("tactile surface tap click, physical mechanical impact transient")
                
        # 2. Extract material adjectives from the original prompt
        tactile_keywords = ["felt", "wood", "wooden", "metal", "metallic", "plastic", "glass", "dry", "soft", "hard", "click", "tap", "knock", "scrape", "close-miked", "intimate", "strike", "hammer", "spring", "friction"]
        extracted_materials = [w for w in tactile_keywords if w in prompt_lower]
        
        # 3. Clean negative musical/tonal words that confuse the generator
        negative_words = [
            "single-note", "single note", "note", "tone", "pitch", "pitch-correction", "pitch correction", 
            "in tune", "tuned", "harmonic", "harmony", "melody", "chord", "chords", 
            "scale", "sustained", "sustain", "long decay", "release decay", "oscillator", 
            "sine", "sawtooth", "square wave", "warmth", "lush", "perfectly in tune"
        ]
        
        cleaned_words = []
        for word in prompt.split():
            clean_word = word.strip(".,;:!?\"'()[]")
            if clean_word.lower() not in negative_words:
                cleaned_words.append(clean_word)
        
        cleaned_base = " ".join(cleaned_words[:25]) # Keep first 25 words of prompt context
        
        # 4. Synthesize the ultimate foley prompt
        derived = (
            f"extremely short non-pitched mechanical transient click, physical impact noise, tactile tap of {cleaned_base}, "
            f"{', '.join(mechanical_additions)}, "
            f"{', '.join(extracted_materials) if extracted_materials else ''}, "
            f"strictly no musical pitch, no tone, no melody, purely dry organic foley mechanism, duration under 0.2 seconds"
        ).replace(", ,", ",").strip(", ")
        
        return derived

    def _derive_tonal_prompts(self, base_prompt: str, count: int, model_name: str = "audiogen") -> list:
        """
        Dynamically distributes 6 logical pitch register categories across the requested note count.
        Ensures a beautiful, natural frequency spectrum from sub-bass to piercing treble.
        """
        registers = [
            "deep sub-bass, heavy low-end frequency register",
            "warm bass, solid low-frequency presence",
            "rich lower midrange register",
            "clear midrange, neutral presence",
            "bright upper midrange, singing register",
            "piercing high-pitched treble, crisp harmonics",
        ]
        
        prompts = []
        for i in range(count):
            if count <= 1:
                reg_desc = registers[3] # Default to clear midrange
            else:
                reg_idx = int((i / (count - 1)) * (len(registers) - 1))
                reg_desc = registers[reg_idx]
                
            if model_name == "stable-audio":
                prompt = (
                    f"sustained single musical pitch tone of {base_prompt}, in {reg_desc}, "
                    f"isolated clean musical pitch, long decay, perfectly in tune, dry monophonic sound"
                )
            else: # audiogen or magnet
                prompt = (
                    f"pure sustained monophonic single note tone of {base_prompt}, in {reg_desc}, "
                    f"isolated clean musical pitch, one-shot sampler instrument note, long decay, "
                    f"perfectly in tune, strictly no chords, no melody, no background noise, dry single sound"
                )
            prompts.append(prompt)
        return prompts

    def _normalize_audio(self, audio_np, target_peak: float = 0.9):
        """
        Peak-normalizes audio data (Tensor, NumPy array, or list) to the target_peak limit (-1.0 dBFS).
        Prevents volume jumps between generated notes.
        """
        import numpy as np
        if isinstance(audio_np, torch.Tensor):
            audio_np = audio_np.cpu().numpy()
        elif isinstance(audio_np, list):
            audio_np = np.array(audio_np)
            
        max_val = np.max(np.abs(audio_np))
        if max_val > 1e-5:
            audio_np = audio_np * (target_peak / max_val)
        return audio_np

    def get_music_model(self):
        """Deprecated/Redirected: MusicGen is disabled to save VRAM."""
        return self.get_audiogen_model()

    def get_audiogen_model(self):
        """Lazy load the AudioGen model for tonal and foley generation."""
        self._unload_dormant_models("audiogen")
        if self._audio_model is None:
            print("[InGen Generator] Loading 'facebook/audiogen-medium' model (AudioGen)...")
            self._audio_model = AudioGen.get_pretrained('facebook/audiogen-medium', device=self.device)
            print("[InGen Generator] 'facebook/audiogen-medium' loaded successfully.")
        return self._audio_model

    def get_magnet_model(self):
        """Lazy load the Audio-MAGNeT Small model."""
        self._unload_dormant_models("magnet")
        if self._magnet_model is None:
            from audiocraft.models import MAGNeT
            print("[InGen Generator] Loading 'facebook/audio-magnet-small' model (MAGNeT)...")
            self._magnet_model = MAGNeT.get_pretrained('facebook/audio-magnet-small', device=self.device)
            print("[InGen Generator] 'facebook/audio-magnet-small' loaded successfully.")
        return self._magnet_model

    def get_stable_audio_model(self):
        """Lazy load the Stable Audio Open Small model in float16 to save VRAM."""
        self._unload_dormant_models("stable_audio")
        if self._stable_audio_model is None:
            from diffusers import StableAudioPipeline
            print("[InGen Generator] Loading 'stabilityai/stable-audio-open-1.0' model in float16...")
            self._stable_audio_model = StableAudioPipeline.from_pretrained(
                "stabilityai/stable-audio-open-1.0",
                torch_dtype=torch.float16 if self.device == "cuda" else torch.float32
            )
            self._stable_audio_model = self._stable_audio_model.to(self.device)
            print("[InGen Generator] 'stabilityai/stable-audio-open-1.0' loaded successfully.")
        return self._stable_audio_model

    def get_woosh_model(self):
        """Lazy load the Sony Woosh DFlow model."""
        self._unload_dormant_models("woosh")
        if not hasattr(self, "_woosh_model"):
            self._woosh_model = None
            
        if self._woosh_model is None:
            server_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
            woosh_dir = os.path.join(server_dir, "models", "woosh")
            
            if woosh_dir not in sys.path:
                sys.path.append(woosh_dir)
                
            try:
                from woosh.model.flowmap_from_pretrained import FlowMapFromPretrained
                from woosh.components.base import LoadConfig
                
                print("[InGen Generator] Loading 'Sony Woosh DFlow' model...")
                checkpoint_path = os.path.join(woosh_dir, "checkpoints", "Woosh-DFlow")
                if not os.path.exists(os.path.join(checkpoint_path, "weights.safetensors")) and not os.path.exists(os.path.join(checkpoint_path, "weights.pt")):
                    raise FileNotFoundError(
                        f"Checkpoint weights not found at {checkpoint_path}. "
                        "From server/: run `python models/download_woosh.py`."
                    )
                    
                self._woosh_model = FlowMapFromPretrained(LoadConfig(path=checkpoint_path))
                self._woosh_model = self._woosh_model.eval().to(self.device)
                print("[InGen Generator] 'Sony Woosh DFlow' loaded successfully.")
            except Exception as e:
                print(f"[InGen Generator] Failed to load Woosh model: {e}")
                self._woosh_model = "FAILED"
        return self._woosh_model

    def generate_tonal(self, prompt: str, count: int, duration: float, temperature: float, model: str = "audiogen-medium") -> list:
        print(f"[InGen Generator] generate_tonal called with model: {model}")
        if model == "stable-audio-open":
            return self._generate_stable_audio(prompt, count, duration, temperature, is_tonal=True)
        elif model == "magnet-small":
            return self._generate_magnet(prompt, count, duration, temperature, is_tonal=True)
        elif model == "woosh-dflow":
            return self._generate_woosh(prompt, count, duration, temperature, is_tonal=True)
        else: # Default: audiogen-medium
            return self._generate_audiogen(prompt, count, duration, temperature, is_tonal=True)

    def generate_foley(self, prompt: str, count: int, duration: float, temperature: float, model: str = "audiogen-medium") -> list:
        print(f"[InGen Generator] generate_foley called with model: {model}")
        if model == "stable-audio-open":
            return self._generate_stable_audio(prompt, count, duration, temperature, is_tonal=False)
        elif model == "magnet-small":
            return self._generate_magnet(prompt, count, duration, temperature, is_tonal=False)
        elif model == "woosh-dflow":
            return self._generate_woosh(prompt, count, duration, temperature, is_tonal=False)
        else: # Default: audiogen-medium
            return self._generate_audiogen(prompt, count, duration, temperature, is_tonal=False)

    def _generate_audiogen(self, prompt: str, count: int, duration: float, temperature: float, is_tonal: bool) -> list:
        model = self.get_audiogen_model()
        model.set_generation_params(duration=duration, temperature=temperature)

        if is_tonal:
            descriptions = self._derive_tonal_prompts(prompt, count, "audiogen")
            prefix = "tonal"
            midi_key = 60
            foley_note_on_count = count
        else:
            half = count // 2
            foley_note_on_count = count - half
            descriptions = []
            for _ in range(foley_note_on_count):
                descriptions.append(self._derive_foley_prompt(prompt, "note_on"))
            for _ in range(half):
                descriptions.append(self._derive_foley_prompt(prompt, "note_off"))
            prefix = "foley"
            midi_key = -1

        print(f"[InGen Generator] Synthesizing {count} samples using AudioGen with dynamic mechanical actions...")
        
        wavs = model.generate(descriptions)
        
        generated_samples = []
        for i, wav in enumerate(wavs):
            if is_tonal:
                current_prefix = prefix
            else:
                current_prefix = "foley_note_off" if i >= foley_note_on_count else "foley_note_on"

            filename = f"{current_prefix}_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            import scipy.io.wavfile as wavfile
            audio_np = wav.cpu().numpy()
            
            # Apply volume peak normalization to -1.0 dBFS (0.9 peak)
            audio_np = self._normalize_audio(audio_np, 0.9)
            
            if len(audio_np.shape) == 2:
                audio_np = audio_np.T
            wavfile.write(filepath, model.sample_rate, audio_np)
            print(f"[InGen Generator] Saved AudioGen sample: {filepath}")
            
            generated_samples.append({
                "filename": filename,
                "midi_root_key": midi_key,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples

    def _generate_magnet(self, prompt: str, count: int, duration: float, temperature: float, is_tonal: bool) -> list:
        model = self.get_magnet_model()
        model.set_generation_params(temperature=temperature)

        if is_tonal:
            descriptions = self._derive_tonal_prompts(prompt, count, "magnet")
            prefix = "tonal"
            midi_key = 60
            foley_note_on_count = count
        else:
            half = count // 2
            foley_note_on_count = count - half
            descriptions = []
            for _ in range(foley_note_on_count):
                descriptions.append(self._derive_foley_prompt(prompt, "note_on"))
            for _ in range(half):
                descriptions.append(self._derive_foley_prompt(prompt, "note_off"))
            prefix = "foley"
            midi_key = -1

        print(f"[InGen Generator] Synthesizing {count} samples using MAGNeT with dynamic mechanical actions...")
        
        wavs = model.generate(descriptions)
        
        generated_samples = []
        for i, wav in enumerate(wavs):
            if is_tonal:
                current_prefix = prefix
            else:
                current_prefix = "foley_note_off" if i >= foley_note_on_count else "foley_note_on"

            filename = f"{current_prefix}_magnet_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            # Slice the audio tensor to the requested duration (MAGNeT generates fixed-length blocks)
            max_samples = int(duration * model.sample_rate)
            if wav.shape[-1] > max_samples:
                wav = wav[..., :max_samples]
            
            import scipy.io.wavfile as wavfile
            audio_np = wav.cpu().numpy()
            
            # Apply volume peak normalization to -1.0 dBFS (0.9 peak)
            audio_np = self._normalize_audio(audio_np, 0.9)
            
            if len(audio_np.shape) == 2:
                audio_np = audio_np.T
            wavfile.write(filepath, model.sample_rate, audio_np)
            print(f"[InGen Generator] Saved MAGNeT sample: {filepath}")
            
            generated_samples.append({
                "filename": filename,
                "midi_root_key": midi_key,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples

    def _generate_stable_audio(self, prompt: str, count: int, duration: float, temperature: float, is_tonal: bool) -> list:
        pipeline = self.get_stable_audio_model()
        
        if is_tonal:
            descriptions = self._derive_tonal_prompts(prompt, count, "stable-audio")
            prefix = "tonal"
            midi_key = 60
            foley_note_on_count = count
        else:
            half = count // 2
            foley_note_on_count = count - half
            descriptions = []
            for _ in range(foley_note_on_count):
                descriptions.append(self._derive_foley_prompt(prompt, "note_on"))
            for _ in range(half):
                descriptions.append(self._derive_foley_prompt(prompt, "note_off"))
            prefix = "foley"
            midi_key = -1

        print(f"[InGen Generator] Synthesizing {count} samples using Stable Audio Open with dynamic mechanical actions...")
        
        generated_samples = []
        for i in range(count):
            # Create seed variations for diversity
            generator = torch.Generator(self.device).manual_seed(100 + i)
            
            output = pipeline(
                prompt=descriptions[i],
                num_inference_steps=50,
                audio_end_in_s=duration,
                generator=generator
            )
            
            # Retrieve generated audio: [channels, samples]
            audio_np = output.audios[0].cpu().numpy().astype("float32")
            
            # Apply volume peak normalization to -1.0 dBFS (0.9 peak)
            audio_np = self._normalize_audio(audio_np, 0.9)
            
            if len(audio_np.shape) == 2:
                audio_np = audio_np.T # Shape: [T, C] for scipy
                
            if is_tonal:
                current_prefix = prefix
            else:
                current_prefix = "foley_note_off" if i >= foley_note_on_count else "foley_note_on"

            filename = f"{current_prefix}_stable_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            import scipy.io.wavfile as wavfile
            wavfile.write(filepath, pipeline.vae.config.sampling_rate, audio_np)
            print(f"[InGen Generator] Saved Stable Audio sample: {filepath}")
            
            generated_samples.append({
                "filename": filename,
                "midi_root_key": midi_key,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples

    def _generate_woosh(self, prompt: str, count: int, duration: float, temperature: float, is_tonal: bool) -> list:
        """
        Sony Woosh DFlow integration.
        """
        model = self.get_woosh_model()
        if model == "FAILED" or model is None:
            print("[InGen Generator] Woosh model is not available. Falling back to AudioGen.")
            return self._generate_audiogen(prompt, count, duration, temperature, is_tonal)
            
        from woosh.inference.flowmap_sampler import sample_euler
        
        if is_tonal:
            descriptions = self._derive_tonal_prompts(prompt, count, "woosh")
            prefix = "tonal"
            midi_key = 60
            foley_note_on_count = count
        else:
            half = count // 2
            foley_note_on_count = count - half
            descriptions = []
            for _ in range(foley_note_on_count):
                descriptions.append(self._derive_foley_prompt(prompt, "note_on"))
            for _ in range(half):
                descriptions.append(self._derive_foley_prompt(prompt, "note_off"))
            prefix = "foley"
            midi_key = -1

        print(f"[InGen Generator] Synthesizing {count} samples using Sony Woosh DFlow...")
        
        generated_samples = []
        for i, desc in enumerate(descriptions):
            # Model latent shape [1, 128, 501] roughly corresponds to 10s audio at 48kHz
            noise = torch.randn(1, 128, 501).to(self.device)
            
            cond = model.get_cond(
                {"audio": None, "description": [desc]},
                no_dropout=True,
                device=self.device,
            )
            
            with torch.inference_mode():
                x_fake = sample_euler(
                    model=model,
                    noise=noise,
                    cond=cond,
                    num_steps=4,
                    renoise=[0, 0.5, 0.5, 0.3],
                    cfg=4.5,
                )
                audio_fake = model.autoencoder.inverse(x_fake)
                
            audio_np = audio_fake[0].cpu().numpy()
            audio_np = self._normalize_audio(audio_np, 0.9)
            
            sample_rate = 48000
            max_samples = int(duration * sample_rate)
            if len(audio_np.shape) == 2: # [C, T]
                if audio_np.shape[1] > max_samples:
                    audio_np = audio_np[:, :max_samples]
                audio_np = audio_np.T # [T, C]
            else: # [T]
                if audio_np.shape[0] > max_samples:
                    audio_np = audio_np[:max_samples]
                    
            if is_tonal:
                current_prefix = prefix
            else:
                current_prefix = "foley_note_off" if i >= foley_note_on_count else "foley_note_on"
                
            filename = f"{current_prefix}_woosh_{uuid.uuid4().hex[:8]}_{i}.wav"
            filepath = os.path.join(self.cache_dir, filename)
            
            import scipy.io.wavfile as wavfile
            wavfile.write(filepath, sample_rate, audio_np)
            print(f"[InGen Generator] Saved Woosh DFlow sample: {filepath}")
            
            generated_samples.append({
                "filename": filename,
                "midi_root_key": midi_key,
                "pitch_correction_cents": 0.0
            })
            
        return generated_samples
