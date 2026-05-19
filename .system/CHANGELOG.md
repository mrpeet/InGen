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

