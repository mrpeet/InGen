# ROADMAP.md - Development Milestones

## High-Level Milestones

- [x] **Phase 0: Bootstrap & Definition** <!-- id: 0 -->
  - [x] Create `.system/` governance files <!-- id: 0.1 -->
  - [x] Gather requirements (Core concept, layers, DSP, and preset management) <!-- id: 0.2 -->
  - [x] Approve Mission, Tech Stack, & Requirements <!-- id: 0.3 -->
  - [x] User approval of initial ROADMAP & ARCHITECTURE <!-- id: 0.4 -->
- [x] **Phase 1: Architecture Setup & C++ Boilerplate** <!-- id: 1 -->
  - [x] Set up JUCE CMake multi-target project (VST3, AU, Standalone) <!-- id: 1.1 -->
  - [x] Set up local Python server codebase (FastAPI, PyTorch, HuggingFace transformers for AudioCraft) <!-- id: 1.2 -->
  - [x] Define abstract C++ classes: `AudioGeneratorAPI`, client structures, and data models <!-- id: 1.3 -->
- [x] **Phase 2: Local Python Server & Generation Pipeline** <!-- id: 2 -->
  - [x] Implement local FastAPI endpoints for `MusicGen` and `AudioGen` <!-- id: 2.1 -->
  - [x] Build async Python generation workers with chunked polling or SSE (Server-Sent Events) <!-- id: 2.2 -->
  - [x] Establish sample cache directory in Python server (`/cache/`) <!-- id: 2.3 -->
- [x] **Phase 3: DSP & Sample Analysis Pipeline (C++)** <!-- id: 3 -->
  - [x] Integrate RubberBand Library (or a high-quality phase vocoder SDK) into C++ project <!-- id: 3.1 -->
  - [x] Implement `PitchDetector` class utilizing the YIN algorithm (via Aubio or custom lightweight C++ class) <!-- id: 3.2 -->
  - [x] Implement `TransientDetector` with threshold/flux detection and zero-crossing snapping <!-- id: 3.3 -->
  - [x] Build asynchronous background worker `SampleAnalysisJob` to run analysis off the audio thread <!-- id: 3.4 -->
- [x] **Phase 4: Dual-Layer Sampler Engine** <!-- id: 4 -->
  - [x] Build custom `LayerASynthesiser` (inherits from `juce::Synthesiser` with custom voice interpolation and RubberBand pitch-shifting/time-stretching) <!-- id: 4.1 -->
  - [x] Build custom `LayerBSynthesiser` (handles Foley, triggers Note-On, Note-Off, Round-Robin, and Velocity zone selection) <!-- id: 4.2 -->
  - [x] Implement shared state controller `LayerLinker` to link/decouple active preset selections <!-- id: 4.3 -->
- [x] **Phase 5: Preset Serialization & Export** <!-- id: 5 -->
  - [x] Implement JSON metadata schema reader/writer (`PresetSerializer` using `juce::var` and `juce::JSON`) <!-- id: 5.1 -->
  - [x] Implement `PresetArchiver` to pack all WAV files and JSON metadata into a single `.ingsam` zip file <!-- id: 5.2 -->
- [x] **Phase 6: UI/UX & Interactive Editor** <!-- id: 6 -->
  - [x] Design glassmorphic UI editor using JUCE vectors and custom shaders/LookAndFeel <!-- id: 6.1 -->
  - [x] Build interactive `WaveformVisualizer` showing transient markers, crop sliders, and zero-crossing locks <!-- id: 6.2 -->
  - [x] Create UI components for prompt input, generation settings, ADSR knobs, and the chain-link icon <!-- id: 6.3 -->
- [x] **Phase 7: DAW Validation, Profiling & Refinement** <!-- id: 7 -->
  - [x] Validate real-time DSP performance under load in multiple DAWs (Ableton Live, Logic Pro, Reaper) <!-- id: 7.1 -->
  - [x] Profiling memory and thread safety to ensure zero audio glitches during async sample loading <!-- id: 7.2 -->
- [x] **Phase 8: Advanced Loop Module (Ping-Pong, Repeat & Crossfade Crossover)** <!-- id: 8 -->
  - [x] Implement `LoopMode` state (Off, Repeat/Forward, Ping-Pong/Bidirectional) in `SamplerSoundA` <!-- id: 8.1 -->
  - [x] Add `crossoverLength` parameter (0% to 100% of sample/crop size) for crossfade calculation <!-- id: 8.2 -->
  - [x] Write C++ linear/equal-power crossfade algorithms to blend loop start and end boundaries smoothly <!-- id: 8.3 -->
  - [x] Update `SamplerVoiceA` to process sample iteration and bidirectional tape-heads (Ping-Pong) <!-- id: 8.4 -->
  - [x] Add loop crop markers and a glassmorphic Loop Control Panel (Loop Mode Button, Crossfade Slider) to the UI <!-- id: 8.5 -->
- [x] **Phase 9: Engine Polish, Differentiated Prompting & DSP Refinement (The Battle Plan)** <!-- id: 9 -->
  - [x] **Octave-Differentiated Prompting (Server):** Implement frequency-range specific descriptors (e.g. "deep bass, sub-bassy, low-end rumble" for C2; "warm, neutral, mid-range presence" for C4; "piercing, bright, crisp treble harmonics" for C6) to guide the AI model to generate closer raw pitches, preventing heavy pitching distortion. <!-- id: 9.1 -->
  - [x] **Dynamic Foley Prompting (Server):** Dynamic prompt sets specifically separating Note-On Layer B (fast attack, mechanical strike, key contact, transient impact) and Note-Off Layer B (dampener falling, release click, mechanical key release sound) to match actual physical keyboard mechanics. <!-- id: 9.2 -->
  - [x] **DSP Performance Fix (C++):** Optimize YIN pitch detection. Restrict the analyzer to a small, stable 4096-sample window extracted 200ms–500ms *after* the detected crop start. This replaces the O(N^2) full-buffer bottleneck (17.5 billion loops) with a fast, 1ms analysis window, completely avoiding noisy transient attacks. <!-- id: 9.3 -->
  - [x] **Unified Volume Normalization (Server/C++):** Add peak and RMS normalization (normalizing peak to -1.0 dBFS) to all generated WAV files so that there are no volume jumps between generated notes. <!-- id: 9.4 -->
  - [x] **Transient Crop Sliders & ADSR Knobs (UI):** Expose interactive Start/End crop markers on the Waveform Visualizer and physical Knobs for ADSR envelopes and Pitch detection offset thresholds to give the user absolute control. <!-- id: 9.5 -->
- [ ] **Phase 10: Advanced Features & Creative Polish** <!-- id: 10 -->
  - [x] **Reverse Playback:** Add Reverse Mode button in UI and reverse playback logic in `SamplerVoiceA.cpp` / `SamplerVoiceB.cpp` <!-- id: 10.1 -->
  - [x] **Velocity-to-Filter Lowpass Modulation:** Implement lowpass filter in `SamplerVoiceB.cpp` modulated dynamically by MIDI velocity <!-- id: 10.2 -->
  - [ ] **Multi-Waveform Visual Overlay:** Render Layer B mechanical transient boundaries/onsets as transparent Neon-Amethyst peaks in `WaveformVisualizer` <!-- id: 10.3 -->
  - [ ] **Offline Batch Sample Upscaling:** Incorporate high-fidelity resamplers in the Python server to upscale generated audio to 44.1kHz / 48kHz <!-- id: 10.4 -->
  - [ ] **Drag-and-Drop Sample Import:** Enable interactive file dragging onto `WaveformVisualizer` to load local custom WAV/AIFF samples <!-- id: 10.5 -->

---

## Current Phase Details (Phase 10: Advanced Features & Creative Polish)
- **Objective:** Implement creative improvements: Reverse sample playback, velocity-to-filter modulation, multi-waveform overlays, high-fidelity upscaling, and drag-and-drop sample support.
- **Status:** **IN PROGRESS**. Starting with Reverse Playback!

