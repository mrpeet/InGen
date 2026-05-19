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
- [ ] **Phase 6: UI/UX & Interactive Editor** <!-- id: 6 -->
  - [ ] Design glassmorphic UI editor using JUCE vectors and custom shaders/LookAndFeel <!-- id: 6.1 -->
  - [ ] Build interactive `WaveformVisualizer` showing transient markers, crop sliders, and zero-crossing locks <!-- id: 6.2 -->
  - [ ] Create UI components for prompt input, generation settings, ADSR knobs, and the chain-link icon <!-- id: 6.3 -->
- [ ] **Phase 7: DAW Validation, Profiling & Refinement** <!-- id: 7 -->
  - [ ] Validate real-time DSP performance under load in multiple DAWs (Ableton Live, Logic Pro, Reaper) <!-- id: 7.1 -->
  - [ ] Profiling memory and thread safety to ensure zero audio glitches during async sample loading <!-- id: 7.2 -->

---

## Current Phase Details (Phase 6: UI/UX & Interactive Editor)
- **Objective:** Design the beautiful vector-based custom glassmorphic GUI editor.
- **Immediate Task:** Write the custom LookAndFeel, layout the responsive controls, and construct the interactive waveform visualizer.
