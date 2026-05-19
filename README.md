# InGen - AI-Powered Multi-Layer Instrument Sampler

Welcome to **InGen**, a premium C++ VST3/AU/Standalone sampler plugin seamlessly paired with a local Python AI generation backend. InGen enables you to generate, analyze, crop, pitch-correct, time-stretch, and play multi-layered sample instruments directly in your DAW or as a standalone application.

The sampler uses a **Dual-Layer architecture**:
*   **Layer A (Tonal Core):** Generated via Meta AudioCraft **AudioGen** (with sustained single-note prompt enrichment), then auto-pitched, snapped, and mapped.
*   **Layer B (Foley/Mechanic Noise):** Generated via **AudioGen** (with transient-noise prompt enrichment) to inject organic life (wood clicks, key taps, pedal squeaks) into the digital sound.

---

## 🛠️ System Requirements & Hardware Guidelines

Your development and generation PC is perfectly optimized for this setup:
*   **CPU:** AMD Ryzen 9 7900X (Handles C++ offline DSP analysis, YIN pitch detection, and RubberBand time-stretching in milliseconds).
*   **GPU:** NVIDIA GeForce GTX 1070 (8GB VRAM is fully utilized and highly efficient now that we load a single shared AudioGen model instead of both MusicGen and AudioGen).
*   **RAM:** 32 GB RAM (Smooth multitasking of DAW, C++ development, and Python server).

---

## 🚀 Step-by-Step Startup Guide

Follow these steps to set up the backend and compile/run the plugin:

### Part 1: Start the local Python AI Server

The Python server hosts the `facebook/audiogen-medium` model (1.5B parameters) and exposes a FastAPI REST API for the C++ plugin.

1.  **Open PowerShell / Terminal** and navigate to the server directory:
    ```powershell
    cd c:\Users\peetb\Desktop\InGen\server
    ```

2.  **Create a Python Virtual Environment** (highly recommended, Python 3.9 - 3.11):
    ```powershell
    python -m venv venv
    ```

3.  **Activate the Virtual Environment**:
    *   **Windows (PowerShell):**
        ```powershell
        .\venv\Scripts\Activate.ps1
        ```
    *   **Windows (CMD):**
        ```cmd
        .\venv\Scripts\activate.bat
        ```

4.  **Install PyTorch with CUDA Support** (crucial for GPU acceleration on your GTX 1070):
    Go to the [official PyTorch website](https://pytorch.org/get-started/locally/) and run the appropriate installation command. Example for CUDA 11.8/12.1:
    ```powershell
    pip install torch torchaudio --index-url https://download.pytorch.org/whl/cu121
    ```

5.  **Install Server Requirements**:
    ```powershell
    pip install -r requirements.txt
    ```

6.  **(Optional) Set up Sony Woosh DFlow** — only needed if you use the `woosh-dflow` model in the plugin:
    From the `server` directory (venv active):
    ```powershell
    cd models
    python download_woosh.py
    cd ..
    ```
    This clones [Sony Woosh](https://github.com/SonyResearch/Woosh) into `server/models/woosh/` (gitignored) and downloads checkpoint weights from the [woosh-sfx releases](https://github.com/SonyResearch/woosh-sfx/releases). Re-run the script anytime to fetch missing checkpoints only.

7.  **Install Meta AudioCraft (from Source)**:
    AudioCraft needs to be installed directly from the GitHub repository:
    ```powershell
    pip install git+https://github.com/facebookresearch/audiocraft.git
    ```

8.  **Run the Server**:
    Start the FastAPI server on `http://127.0.0.1:8000`:
    ```powershell
    python app/main.py
    ```
    *On the first run, the server will automatically download the `facebook/audiogen-medium` weights (~3 GB) from HuggingFace.*

9.  **Verify Health:**
    Open your browser and go to `http://127.0.0.1:8000/health`. You should see:
    ```json
    {"status": "healthy", "server": "InGen Server 1.0.0"}
    ```
    You can also view the interactive Swagger API documentation at `http://127.0.0.1:8000/docs`.

---

### Part 2: Build & Launch the C++ JUCE Plugin

The client frontend is built in C++ using the **JUCE Framework**.

1.  **Navigate to the Client Directory:**
    ```powershell
    cd c:\Users\peetb\Desktop\InGen\client
    ```

2.  **Open the Project in Projucer / IDE:**
    *   Open `InGen.jucer` in **Projucer**.
    *   If using CMake, open the root folder in an IDE like CLion, Visual Studio, or VS Code.

3.  **Configure Exporters:**
    *   In Projucer, configure the exporters for **Visual Studio** (on Windows) or **Xcode** (on macOS).
    *   Enable VST3, AU, or Standalone formats as desired.

4.  **Compile & Run:**
    *   Open the generated solution file in Visual Studio (e.g., `Builds/VisualStudio2022/InGen.sln`).
    *   Build the project in **Release** or **Debug** mode.
    *   Run the Standalone application or load the compiled `.vst3` / `.component` file in your favorite DAW (e.g., Ableton Live, FL Studio, Reaper, Cubase).

---

## 🎹 Sampler Workflow in the DAW

1.  Open the **InGen Sampler** UI in your DAW.
2.  **Input Tonal Prompt (Layer A):** Describe the instrument sound you want to play (e.g., *“dreamy warm analog brass synth, vintage felt piano tone”*).
3.  **Input Foley Prompt (Layer B):** Describe the organic mechanism or tactile click you want (e.g., *“wooden keys, metallic mechanical tap, vinyl crackle”*).
4.  **Press GENERATE:** The plugin will asynchronously poll the local Python backend. Once complete:
    *   **Layer A** is analyzed (Pitch detection, snapped to closest zero-crossings, corrected to standard equal temperament).
    *   **Layer B** is parsed as unpitched foley sound.
5.  **Play!** Play chords or leads on your MIDI keyboard. The plugin will trigger the pristine tonal instrument core (Layer A) along with tactile, velocity-sensitive foley clicks (Layer B) for a fully immersive acoustic playing feel.

---

## ⚠️ Troubleshooting

*   **CUDA Out of Memory (OOM):** We optimized the generation server to only use **AudioGen** for both layers. This saves massive amounts of GPU memory. Ensure you do not have other heavy GPU tasks (games, video rendering) running on your GTX 1070 while generating.
*   **Port 8000 Conflict:** If another app is using Port 8000, you can change the host port in `server/app/main.py` (line 130) and update the URL in `client/Source/Network/LocalPythonServerClient.h` (line 36 & 95) accordingly, then rebuild the plugin.
*   **Woosh / `server/models/woosh` dirty or submodule warnings:** Woosh is not tracked in this repo. Run `python models/download_woosh.py` from `server/` and ignore local changes inside `woosh/` — the folder is listed in `.gitignore`.
