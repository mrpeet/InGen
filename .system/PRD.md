# Product Requirements Document (PRD) - AI-Powered Multi-Layer Instrument Sampler (InGen)

## 1. Executive Summary
- **Project Mission:** To bridge the gap between prompt-based generative AI audio models and real-time musical expressiveness. "InGen" is a premium VST3/AU/Standalone instrument plugin that enables music producers to generate, analyze, auto-tune, time-stretch, and play multi-layered sample instruments directly in their DAW.
- **Target Audience:** Modern music producers, sound designers, and composers looking for unique, infinitely customizable, and instantly playable sample instruments generated on-the-fly.
- **Core Value Proposition:** Turn natural language descriptions (prompts) into fully playable, multi-velocity, pitch-corrected sample instruments containing both tonal cores (Layer A) and organic mechanical noises (Layer B).

---

## 2. Product Objectives
- **Generative Synergy:** Seamlessly orchestrate local Python AI generation (Meta AudioCraft) and cloud-based fallback APIs in an intuitive, unified UI.
- **Perfect Playability:** Automatically transform raw, out-of-tune generated audio files into highly accurate, zero-crossing cropped, and pitch-corrected MIDI maps.
- **Organic Depth:** Enable double-layer synthesis where tonal components are separate from acoustic "mechanical noises" to mimic the tactile feel of physical instruments (e.g., key clicks, pedal squeaks).
- **DAW Integration:** Provide rock-solid real-time DSP performance in major DAWs (VST3/AU/Standalone formats).

---

## 3. User Persona & Stories
- **User Persona: Marcus (Film Composer)**
  * *Needs:* Wants organic, unique sounds like "a fragile trumpet played in a dusty, metallic room with heavy key clicking."
  * *Friction:* Text-to-audio models generate long WAVs that are out of pitch, have silence at the start, and don't change pitch across the keyboard without sounding unnatural.
- **User Stories:**
  * *Story 1:* As a producer, I want to type "vintage felt piano" and have the plugin generate 5 pitch-corrected samples distributed across 2 octaves, so that I can immediately play chords.
  * *Story 2:* As a sound designer, I want to unlink the mechanical noise (Layer B) from the tonal sample (Layer A) so I can pair metallic mechanical clicks with a synthesized vocal pad.
  * *Story 3:* As a composer, I want to adjust the crop boundaries of a sample manually if the auto-transient detection clipped a soft attack, ensuring perfect transient control.

---

## 4. Functional Requirements

### 4.1. Network & Generation Layer
- **Local Generation Server:** A Python backend exposing endpoints for Meta AudioCraft models (MusicGen for tonal samples, AudioGen for mechanical noises).
- **Abstract C++ API Client:** An abstract interface `AudioGeneratorAPI` in the JUCE code. A `LocalPythonServerClient` subclass will execute async HTTP POST requests. A `CloudAPIFallbackClient` can be easily substituted.
- **Generation Parameters:** Prompt text, sample count (notes/octaves), foley count, duration, temperature.

### 4.2. Audio Analysis Pipeline (DSP)
- **Transient Detection:** Amplitude envelope tracking and spectral flux analysis to find the sound's start.
- **Zero-Crossing Snapping:** Automatically adjust crop markers (start/end) to the nearest zero-crossing to prevent pops/clicks.
- **Pitch Detection:** Estimate fundamental frequency ($f_0$) using the YIN algorithm or McLeod Pitch Method (MPM) on the generated tonal sample.
- **Auto-Tuning & Mapping:** Calculate the deviation from the nearest equal-tempered MIDI note, fine-tune the sample playback rate, and map the sample to its root note.
- **Time-Stretching Engine:** Integrate the **RubberBand Library** (or a premium phase vocoder) to shift notes without altering sample duration.

### 4.3. Dual-Layer Synthesis Engine
- **Layer A (Tonal):** Plays the primary pitched, pitch-corrected, and time-stretched sample. Controlled by a dedicated 4-stage ADSR envelope.
- **Layer B (Foley/Mechanics):** Plays unpitched organic/mechanical clicks or noises.
  * *Trigger modes:* Note-On (trigger on key press), Note-Off (trigger on key release - e.g., damper noise).
  * *Velocity Zones:* Trigger different samples based on incoming MIDI velocity.
  * *Variation:* Round-robin scheduling or random select to avoid the "machine gun" effect.
- **Layer Linking:** A toggle (Chain icon) that links preset changes for Layer A & B or decouples them entirely.

### 4.4. Preset & File Management
- **Serialization:** Save active configurations, ADSR curves, velocity bounds, crop markers, and pitch offsets as JSON or XML.
- **Shareable Archive:** Bundle all raw `.wav` samples and the metadata file into a single zipped binary file `.ingsam` (InGen Sampler Instrument).

---

## 5. Non-Functional Requirements
- **Low Latency Playback:** The C++ audio thread must remain strictly non-blocking. File reading, network I/O, and pitch analysis must be offloaded to background threads.
- **Aesthetic Excellence:** Modern, premium UI built using JUCE's LookAndFeel class or custom vector rendering, featuring smooth waveform visualizers, real-time gain meters, and glassmorphic UI panels.
- **Cross-Platform Compatibility:** Must build and run on both Windows and macOS as VST3, AU, and Standalone.
