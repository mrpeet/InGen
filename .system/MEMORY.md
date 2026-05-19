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

## [2026-05-19] - DSP & Playback Engine Implementations

### 4. Self-Contained C++ YIN Pitch Detector
- **Context:** AudioCraft outputs tonal WAV files, but they are not mathematically guaranteed to be in perfect pitch or perfectly centered on the expected MIDI note.
- **Decision:** We implemented a pure, standalone C++ YIN algorithm inside `PitchDetector`.
- **Reasoning:** By avoiding external heavy DSP frameworks (like Aubio or Librosa), the plugin compiles in seconds, is self-contained, and runs extremely fast. We utilize:
  1. *Cumulative Mean Normalized Difference* to effectively eliminate octave errors.
  2. *Parabolic Interpolation* to extract sub-sample frequency information ($f_0$) from the discrete difference function.
  3. Cents deviation calculations: $\text{cents} = 1200 \cdot \log_2(f_{\text{actual}} / f_{\text{target}})$, which are delivered to the playback engine for fine-tuning.

### 5. Sliding RMS Transient Snapping & Zero-Crossing Lock
- **Context:** AI-generated samples often contain small silence padding at the beginning or decay slowly into noise.
- **Problem:** Inconsistent start times destroy the feel of a playable instrument, while slicing audio at arbitrary points creates sudden voltage jumps, causing audible clicks ("pops").
- **Decision:** Created `TransientDetector` which:
  1. Uses a 256-sample sliding RMS window to locate the sound onset (transient start) and decay floor (silence end).
  2. Searches bidirectionally around target crop markers to lock the play boundary onto the nearest perfect zero-crossing ($x[i] \cdot x[i-1] \le 0$).
- **Reasoning:** Zero-crossing snapping guarantees crackle-free sample triggers without forcing a slow volume fade-in at the DSP voice layer.

### 6. Voice Decoupling: Resampling and Release-Trigger Wait States
- **Context:** Layer A (tonal) and Layer B (foley) have fundamentally different playback mechanics.
- **Decision:** Mapped distinct voice logic:
  - *Layer A (Tonal):* Utilizes fractional linear-interpolation resampling. The step ratio ($playbackRatio = 2^{\frac{semitones}{12}} \cdot \frac{SR_{sample}}{SR_{host}}$) combines MIDI distance and the YIN-measured cent offset. This enables flawless pitch correction.
  - *Layer B (Foley):* Handles unpitched mechanics. It supports *Velocity Zones* (triggering clicking noises only on hard attacks), *Round-Robin* buffer cycling, and *Note-Off (Release Triggers)*.
  - *Wait-State Architecture:* Release foley voices remain completely silent during `startNote`, entering an active wait state. They are only triggered when `stopNote` is called by the host DAW (simulating mechanical dampers landing on strings).

## [2026-05-19] - Serialization & Preset Management

### 7. Self-Contained Preset Staging and Zip Compression (.ingsam)
- **Context:** Presets generated using local neural models require multiple physical `.wav` files (tonal layers + foley round-robin layers) alongside their numerical/text configuration (JSON).
- **Problem:** Storing absolute local file paths in presets breaks when presets are shared between users or moved to other directories.
- **Decision:** We established a standardized, self-contained zip packaging format `.ingsam` coordinated by `PresetArchiver`.
- **Reasoning:**
  1. *Staging Workspace:* When exporting, the archiver allocates a temporary folder in the system Temp directory, copies the target assets into it under generic relative filenames (`layerA_sample.wav`, `layerB_variation_X.wav`), and generates a `preset.json` detailing crop coordinates, ADSR envelopes, and prompt instructions.
  2. *Relative Mapping:* Inside the ZIP, all sample paths are mapped relatively.
  3. *Uncompromised Portability:* When importing, `PresetArchiver` decompresses the package using `juce::ZipFile` into an isolation directory, parses `preset.json` back into C++ structures via `PresetSerializer`, and binds the WAV files to the active sampler voices.
  4. *Housekeeping:* The staging directory is recursively deleted immediately after compression to guarantee that no orphaned gigabytes clutter the user's hard drive.

## [2026-05-19] - Vector UI & Waveform Rendering

### 8. High-Performance Cached Peak-Path Waveform Rendering
- **Context:** Rendering real-time waveforms with millions of audio samples inside a 60fps GUI can cause extreme CPU load, block the message loop, and lead to sluggish DAW UI responses.
- **Decision:** We designed a dynamic pixel-cache scanning approach inside `WaveformVisualizer`.
- **Reasoning:** 
  1. *Divided Scanning:* Instead of drawing all sample vertices, the component scans the underlying audio buffer only at pixel-boundaries, computing local min/max peaks for each column.
  2. *Zero-Crossing Snapping:* When the user drags start/end crop boundaries, mouse coordinates are translated to samples, snapped to zero-crossing boundary transitions in real-time, and bound directly to the active sampler voice, preventing audible clicks without causing audio-thread locking.
  3. *Frosted Glassmorphism:* Utilizes high-precision vector strokes in `GlassmorphicLookAndFeel` with smooth alpha-blended arcs, glowing cyber-neon colors, and rounded corners to establish a premium standalone & VST3 visual experience.

## [2026-05-19] - Decoupling VST2 & DAW Validation

### 9. Steinberg VST2 Legacy Decoupling and Real-time Profiling
- **Context:** Modern hosts and developer environments do not ship with discontinued VST2 headers, but VST3 wrappers in JUCE default to including them for replacement compatibility.
- **Decision:** Explicitly bound `JUCE_VST3_CAN_REPLACE_VST2=0` preprocessor macros at the CMake layer.
- **Reasoning:**
  1. *Bypassing Legacy Headers:* Tells the VST3 preprocessor compiler to skip compiling legacy wrapper components requiring discontinued Steinberg files, eliminating compile-time directory errors.
  2. *Zero-Allocation Audio Thread:* Verified that our dual-layer rendering blocks (`processBlock`) perform no system allocations, file accesses, or thread-locking operations. Temporary workspaces and deserialization occur solely on the Message manager thread or isolated async thread pools (`SampleAnalysisJob`), maintaining strict audio thread safety and ensuring glitch-free audio performance under heavy load in host DAWs.




