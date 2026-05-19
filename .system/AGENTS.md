# AGENTS.md

## Project Metadata
- **Project Name:** InGen Sampler
- **Tech Stack:**
  - **Client / Audio Engine:** C++ (JUCE Framework, targeting VST3, AU, Standalone).
  - **External C++ Libraries:** RubberBand Library (time-stretching & pitch-shifting), Aubio or custom C++ DSP (for YIN pitch detection & spectral flux transient detection).
  - **Local Python Server Backend:** FastAPI, PyTorch, Meta AudioCraft (MusicGen & AudioGen models).
  - **Design/UI:** Custom glassmorphic vector UI (JUCE Graphic Context / OpenGL / custom shaders).
- **Namespace:** `ingen`
- **Role:** Autonomous Senior Architect & Lead Developer

## Workflow & Governance
- **Strict Documentation-First Workflow:** Every architectural change or task roadmap phase must be documented before execution.
- **Atomic-Step Principle:** Work in small, incremental, and verifiable chunks. Do not modify more than 2-3 files in a single execution.
- **Language Protocol:**
  - Conversations with the User: German (Deutsch)
  - Code comments & Documentation in `.system/`: English
- **Auto-Documentation Rule:** Update `CHANGELOG.md` and `MEMORY.md` after every atomic implementation step.

## Code Quality & Refactoring
- **Modular Design:** Prioritize small, focused files over large, monolithic ones. 
- **Refactoring:** If a file exceeds a reasonable length or handles too many responsibilities, suggest and implement a refactoring into smaller components or hooks.
- **Safety First:** Refactor only when it is sensible and can be done safely without breaking dependencies. Always verify the impact before moving code.
- **Clean Code:** Apply DRY (Don't Repeat Yourself) and SOLID principles.

## Folder Structure
- `.system/`: Project governance, rules, roadmaps, and requirements.
- `client/`: JUCE C++ project folder.
  - `Source/`: Main plugin files, processors, editor components.
    - `DSP/`: Pitch detection, transient cropping, RubberBand wrappers.
    - `Engine/`: Synthesizer layers (A and B), voice & sound mapping.
    - `Network/`: Abstract API clients & server polling.
    - `UI/`: Interactive editors, waveform visualizers, settings panels.
    - `Preset/`: Serialization & ZIP packing.
- `server/`: Python backend folder.
  - `app/`: FastAPI application endpoints and tasks queue.
  - `models/`: Loader scripts for MusicGen/AudioGen.
  - `cache/`: Directory to temporarily store generated `.wav` files.
