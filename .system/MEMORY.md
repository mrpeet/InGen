# MEMORY.md

This file captures the fundamental reasoning ("Why") behind architectural patterns, major refactoring paths, and critical technical choices to prevent future regression.

## [2026-05-19] - Initial System Architecture Setup

### 1. Offline Pre-Rendering for RubberBand Library
- **Context:** Integrating time-stretching and pitch-shifting inside a VST3/AU instrument plugin requires high quality, especially across several keyboard octaves. 
- **Problem:** Processing a high-quality phase-vocoder (like RubberBand) inside the high-priority real-time audio thread (`juce::AudioProcessor::processBlock`) is extremely CPU-expensive, especially with multiple polyphonic voices. This frequently causes audio dropouts (buffer underruns / glitches) on lower buffer sizes (e.g., 64/128 samples).
- **Decision:** All pitch-shifting and time-stretching operations are offloaded to an asynchronous background worker pool (`RubberBandStretcherJob`). When a new sample is generated/loaded, we stretch and shift it in a separate thread for the designated key range and store the processed PCM buffers in RAM.
- **Why:** This shifts the high CPU cost from the real-time playback phase to the loading/mapping phase, guaranteeing 100% glitch-free playback using standard, lightweight linear/Lagrange interpolation in the audio thread.

### 2. Double-Layer Playback Engine (Layer A & B Decoupling)
- **Context:** Physical acoustic instruments produce two distinct acoustic components: the vibrating medium (tonal pitch, e.g., strings) and the physical mechanical apparatus (transient noises, e.g., key strikes, pedal squeaks).
- **Problem:** Linking these elements statically within a single sampler sound forces the user to rebuild mechanical setups every time a different instrument profile is selected.
- **Decision:** We decoupled the playback engine into two independent JUCE synthesiser instances (`SamplerLayerA` and `SamplerLayerB`). They are coordinated via a central controller `SamplerEngine`.
- **Why:** This architecture enables complete structural versatility. The user can toggle a "Chain Link" in the UI to either link them together (changing presets updates both) or decouple them (keeping mechanical clicks of a grand piano while swapping Layer A to a synthetic brass lead).

### 3. API Abstraction Layer
- **Context:** The immediate implementation requires a local Python FastAPI server using local models (AudioCraft).
- **Problem:** Running models locally on consumer GPUs is resource-heavy, and we might want to deploy cloud endpoints (like OpenAI or custom cloud instances) for web users or lighter installations.
- **Decision:** Defined a pure abstract C++ class `AudioGeneratorAPI` in the network layer.
- **Why:** This ensures the DSP and UI code have zero hard dependencies on local servers or Python environments. Swapping between local generation and cloud providers requires only switching a single subclass factory without refactoring any core plugin logic.
