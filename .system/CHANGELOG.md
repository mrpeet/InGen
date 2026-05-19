# CHANGELOG.md

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - 2026-05-19

### Added
- Created complete **PRD.md** detailing features (Text-to-Audio parameters, Dual-Layer synthesis, pitch correction, zero-crossing snaps, and preset archiving).
- Created complete **ARCHITECTURE.md** explaining the client-server setup, C++ API abstraction, DSP algorithms (YIN, zero-crossing snapping, spectral flux formula), offline RubberBand pre-rendering strategy, and custom JUCE class hierarchies.
- Created **ROADMAP.md** detailing Phase 1 through Phase 7 milestones and granular task lists.
- Updated **AGENTS.md** with the concrete C++/Python tech stack, directory layouts, namespace (`ingen`), and new **Code Quality & Refactoring** rules.
- Documented technical rationale (YIN, RubberBand thread safety, decoupled dual-layer synthesis) in **MEMORY.md** to prevent structural regressions.

## [0.3.0] - 2026-05-19

### Added
- Created **client/CMakeLists.txt** configured with CMake FetchContent for automated, self-contained JUCE (v7.0.12) integration.
- Created C++ abstract interface **AudioGeneratorAPI.h** in `client/Source/Network/` defining the text-to-audio generation contract.
- Created C++ entry points **PluginProcessor.h/cpp** and **PluginEditor.h/cpp** providing the structural foundation of the JUCE application.
- Created **server/requirements.txt** and **server/app/main.py** for the FastAPI local backend.
- Implemented static WAV caching (`/cache/`) and mock endpoints for tonal and foley synthesis in the FastAPI server.
- Set up local Python virtual environment (`server/venv`), installed starlette, uvicorn, pydantic, and fastapi dependencies, and launched the server running in the background at `http://127.0.0.1:8000`.

## [0.4.0] - 2026-05-19

### Added
- Created **client/Source/DSP/PitchDetector.h/cpp** implementing a self-contained YIN fundamental frequency estimation engine and MIDI mapping utility.
- Created **client/Source/DSP/TransientDetector.h/cpp** implementing sliding RMS window energy onset tracking and sub-sample zero-crossing alignment search.
- Created **client/Source/DSP/SampleAnalysisJob.h/cpp** providing a secure, asynchronous background Thread to load WAV files, compute crop markers, execute YIN pitch detection, and deliver results thread-safely to the message thread via `juce::MessageManager::callAsync`.
- Updated **client/CMakeLists.txt** to compile the three new DSP modules and link the necessary **juce_audio_formats** dependency.

## [0.5.0] - 2026-05-19

### Added
- Integrated **signalsmith-stretch** via CMake FetchContent to provide a world-class spectral phase-vocoder SDK with zero compilation dependencies.
- Created **client/Source/Engine/SamplerSoundBase.h** mapping properties of `SamplerSoundA` (tonal pitch, crop markers, ADSR params) and `SamplerSoundB` (foley, velocity zones, and variations).
- Created **client/Source/Engine/SamplerVoiceA.h/cpp** implementing high-performance resampled linear-interpolation playback with dynamic host sample rate scaling and active ADSR envelope triggers.
- Created **client/Source/Engine/SamplerVoiceB.h/cpp** implementing unpitched foley sample playback supporting strike-velocity checks, Round-Robin cycling, and release-triggering (Note-Off).
- Created **client/Source/Engine/SamplerEngine.h/cpp** coordinating both layer synthesisers in parallel and exposing public loading hooks.
- Integrated **SamplerEngine** into **InGenSamplerAudioProcessor** to orchestrate thread-safe MIDI processing and parallel block rendering.
- Updated **client/CMakeLists.txt** to compile the seven new Sampler Engine playback components.

## [0.6.0] - 2026-05-19

### Added
- Created **client/Source/Serialization/PresetSerializer.h/cpp** implementing JSON serialization mapping for dynamic objects, ADSR values, and multi-sample lists.
- Created **client/Source/Serialization/PresetArchiver.h/cpp** implementing export and import wrappers for self-contained `.ingsam` ZIP packages, including recursive temporary workspace setup and cleanup.
- Updated **client/CMakeLists.txt** to compile the four new serialization files.

## [0.7.0] - 2026-05-19

### Added
- Created **client/Source/UI/GlassmorphicLookAndFeel.h/cpp** implementing premium vector design schemes, translucent gradient panels (frosted glass), glowing neon-cyan attack dials, and dynamic hover outlines.
- Created **client/Source/UI/WaveformVisualizer.h/cpp** implementing a 60fps peak-scanning waveform graph, interactive boundary markers for crop boundaries, and sign-change zero-crossing lock search algorithms.
- Re-architected **client/Source/PluginEditor.h/cpp** to arrange widgets responsively, link ADSR sliders to SamplerVoice DSP models, and bind preset linkage toggles.
- Updated **client/CMakeLists.txt** to compile the four new UI components.

## [1.0.0] - 2026-05-19

### Added
- Integrated C++ preprocessor definitions to decouple VST2 legacy dependency, ensuring standalone compilation using standard Windows VST3 SDK boundaries.
- Verified MSVC 19 thread configurations to guarantee zero memory allocations in real-time `processBlock` and `renderNextBlock` paths.
- Deployed full Release VST3 and Standalone executable build pipelines.





