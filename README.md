# Orpheus Plus — The Ultimate AI Studio Co-Producer

Orpheus Plus is a state-of-the-art, fully offline Desktop Digital Audio Workstation (DAW) built in JUCE 7 and C++20. It represents a paradigm shift in modern music production, shifting the focus entirely towards **flawless recording, mastering, fine-tuning, and editing—all powered by real-time AI.**

The ultimate goal of Orpheus Plus is simple: **Perfect vocals recorded every time, even if the person is a horrible singer.** It achieves this by combining latency-free playback with deeply embedded neural networks that autocorrect pitch, timing, and timbre without ever sounding robotic or machine-generated. 

The output? **Pristine, 100% human-sounding `.wav` files that flawlessly bypass AI detectors**, completely offline and completely self-reliant.

---

## The Vision: The Future of Production

### 1. The Human Vocal Guarantee
Through the use of advanced local ONNX models, Orpheus Plus dynamically injects unvoiced stochastic analog noise and human-like micro-variations into autocorrected tracks. This means heavy pitch correction, formant shifting, and voice cloning sound organically recorded. The exported vocal stems read as human, effectively bypassing AI audio detection software.

### 2. Zero-Latency AI Playback
The days of rendering AI effects offline are over. Orpheus Plus natively integrates ONNX Runtime via C++ `FetchContent` directly into the `AudioProcessorGraph`. This allows incredibly dense neural acoustic models (Match EQ, Stem Separation, Vocal Restoration) to run in real-time alongside traditional VST3/AU plugins without compromising the audio buffer.

### 3. Absolute Offline Autonomy
The DAW is entirely self-contained. No subscriptions, no cloud latency, no external Python APIs. Every neural network, from Stem Separators to Text-to-Sample generators, runs 100% locally on your machine, ensuring your intellectual property never leaves your studio.

---

## Current Status & Completed Milestones

### Core Architecture
- **Multi-track Timeline**: High-performance arrangement view with swipe-and-drag comping and beat-matching.
- **Unified Piano Roll**: Multi-track MIDI overlay and editing mapped directly to a `juce::MPESynthesiser` for rich polyphonic expression.
- **Universal Export Engine**: Render master mixes and stems natively in WAV, FLAC, OGG, AIFF, MP3, and AAC with customizable bit depths and sample rates.
- **Spotify Mastering Preset**: One-click export enforcing exact -14 LUFS / -1 dB True Peak standards dynamically.

### AI Processing & DSP
- **Native ONNX Runtime**: Local inference built directly into the C++ graph.
- **AI Stem Extraction Automation**: Seamlessly bounce the mix to a temporary file, pass it through the ONNX StemSeparator, and auto-import Vocals, Bass, Drums, and Guitar back into the DAW timeline.
- **Dynamic AI Mastering**: Automatically configures Linear Phase EQs and Multiband Compressors to achieve optimal LUFS loudness levels.
- **Vocal Suite & Voice Cloning**: Train and apply custom vocal profiles natively in the DAW, complete with organic pitch correction.

---

## The New Roadmap: The Road to Perfection

With the core architecture established, the new roadmap completely doubles down on the **Vocal, Mastering, and Editing** workflow, ensuring that the AI seamlessly bridges the gap between amateur recordings and billboard-ready masters.

### Phase 1 — The Flawless Vocal Suite
- **Neural Auto-Tune**: An AI-driven pitch correction module that analyzes the intent of the singer and applies completely invisible, artifact-free correction.
- **Stochastic Humanizer**: A post-processing neural layer that guarantees the vocal takes sound organically recorded by injecting realistic breath mapping, sibilance, and subtle analog imperfections to outsmart AI detectors.
- **Timbre Transfer & Deep Fake Vocals**: Sing a guide track with terrible pitch/tone, and let the AI fully replace the timbre with a world-class vocalist profile—while retaining your exact timing and emotional delivery.

### Phase 2 — The Intelligent Mastering & Mixing Engineer
- **Latent Space Match-EQ**: Upload a reference track, and the AI learns its exact compression, saturation, and EQ characteristics via latent embeddings, dynamically applying them to your master bus in real time.
- **Smart Spectral Carving**: AI-driven dynamic EQ that automatically ducks conflicting frequencies (e.g., carving space out of the electric guitars only in the exact milliseconds the vocal is singing) without tedious sidechain routing.
- **Phase Alignment AI**: Automatically phase-align multi-mic recordings (e.g., drum kits or layered acoustic guitars) to achieve maximum punch and clarity with zero comb filtering.

### Phase 3 — The Generative Studio Co-Pilot
- **Text-to-Sample ONNX Pipeline**: Type a prompt directly into the timeline ("Give me a gritty 808 sub bass drop") and the local model generates, trims, and places the `.wav` file precisely on the beat.
- **Generative MIDI Auto-Completion**: Highlight a MIDI block and have the DAW auto-complete the melody, bassline, or chord progression in the style and key of your current project.
- **Arrangement Energy Analyzer**: The AI maps the energy curve of your song and suggests structural changes (adding risers, dropping the beat, introducing transition fills).

### Phase 4 — Ultimate Professional Polish & Workflow
- **Stem Auto-Arranger Expansion**: Upgrading the export engine to seamlessly parse, isolate, name, and color-coordinate every single element of the track into distinct `.wav` stems for professional mixdown delivery.
- **Hardware Control Integration**: Deep native mapping for MCU/HUI motorized fader control surfaces to blend tactile, physical mixing with the AI backend.
- **Zero-Latency Monitoring Optimization**: Rewriting the underlying block processing to predict buffer underruns, allowing the DAW to handle infinite neural plugins on a standard laptop CPU.

---

## Building & Installation

### Prerequisites
- **CMake 3.22+**
- **C++20 Compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux).
- **JUCE 7**: Retrieved automatically or mapped via `git submodule`.
- **ONNX Runtime**: Handled entirely via CMake `FetchContent` during the configuration step.

*Note on MSVC: To prevent internal compiler errors (`C1001`) during Release builds, Link-Time Code Generation (`/GL`) is disabled for certain optimization flags in the CMake setup, or you can build in Debug mode.*

### Instructions
```bash
# 1. Clone and fetch submodules
git clone https://github.com/yourusername/OrpheusPlus.git
cd OrpheusPlus
git submodule update --init --recursive

# 2. Configure (CMake handles downloading ONNX Runtime & linking JUCE)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release -j$(nproc)

# 4. Run
./build/OrpheusPlus_artefacts/Release/OrpheusPlus
```

---

## License
MIT — built on JUCE 7 (JUCE personal/educational license or commercial as needed).
