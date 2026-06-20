# Orpheus Plus — The Ultimate AI Studio Co-Producer

Orpheus Plus is a state-of-the-art, fully offline Desktop Digital Audio Workstation (DAW) built in JUCE 7 and C++20. It represents a paradigm shift in modern music production, shifting the focus entirely towards **flawless recording, mastering, fine-tuning, and editing—all powered by real-time AI.**

The ultimate goal of Orpheus Plus is simple: **Perfect audio recorded every time.** It achieves this by combining latency-free playback with deeply embedded neural networks that autocorrect pitch, timing, and timbre without ever sounding robotic or machine-generated, cleaning up AI-generated tracks, and seamlessly separating elements into manageable stems.

The output? **Pristine, 100% human-sounding, high-quality audio files that flawlessly bypass AI detectors**, completely offline and completely self-reliant.

---

## The Vision: The Future of Production

### 1. The Human Vocal Guarantee & AI Humanization
Through the use of advanced local ONNX models, Orpheus Plus dynamically injects unvoiced stochastic analog noise, breath mapping, and human-like micro-variations into autocorrected and AI-generated tracks (like SUNO/Suno audio). This means heavy pitch correction, formant shifting, voice cloning, and AI-generated music sound organically performed by a real band.

### 2. Zero-Latency AI Playback
The days of rendering AI effects offline are over. Orpheus Plus natively integrates ONNX Runtime via C++ `FetchContent` directly into the `AudioProcessorGraph`. This allows dense neural acoustic models (Stem Separation, AI Humanizer, Match EQ) to run in real-time alongside traditional plugins.

### 3. Absolute Offline Autonomy & Project Management
The DAW is entirely self-contained. No subscriptions, no cloud latency, no external Python APIs. Every neural network runs 100% locally. Projects are smartly managed using the `.orph` format, automatically organizing and safely containing all audio files in dedicated project folders.

---

## Current Architecture & Completed Milestones

### Core Architecture
- **Multi-track Timeline**: High-performance arrangement view with comping and beat-matching.
- **Project Settings & Smart Directories**: Dedicated `.orph` files with intelligent auto-creation of `Audio/` sub-folders, keeping sessions flawlessly organized.
- **Universal Export Engine**: Render master mixes and stems natively in WAV, FLAC, OGG, ALAC, AIFF, MP3, and AAC with customizable bit depths and sample rates.
- **Spotify Mastering Preset**: One-click export enforcing exact -14 LUFS / -1 dB True Peak standards dynamically.

### AI Processing & DSP
- **Native ONNX Runtime**: Local inference built directly into the C++ graph.
- **AI Humanizer**: Offline drag-and-drop and real-time processing to clean up AI-generated chatter, clicks, pops, and crackle.
- **AI Stem Extraction Automation**: Seamlessly bounce the mix to a temporary file, pass it through the ONNX StemSeparator, and auto-import Vocals, Bass, Drums, Synths, and Guitars back into the DAW timeline.
- **Dynamic AI Mastering**: Automatically configures Linear Phase EQs and Multiband Compressors to achieve optimal LUFS loudness levels.

---

## The All-New Roadmap

With the core architecture established, the new roadmap aims to finalize the ecosystem and deliver the ultimate professional polish.

### Phase 11 — The Flawless Vocal Suite & Humanization
- **Neural Auto-Tune**: AI-driven pitch correction that analyzes intent.
- **Stochastic Humanizer Tool**: (COMPLETED) A post-processing neural layer that guarantees the vocal takes sound organically recorded by injecting realistic breath mapping, sibilance, and subtle analog imperfections to outsmart AI detectors.
- **Timbre Transfer**: Deep fake vocals replacing timbre while retaining your exact timing.

### Phase 12 — The Intelligent Mastering & Mixing Engineer
- **Latent Space Match-EQ**: Upload a reference track, and the AI learns its exact characteristics dynamically.
- **Phase Alignment AI**: Automatically phase-align multi-mic recordings.
- **Stem Auto-Arranger Expansion**: Advanced extraction that separates, renames, categorizes, and optimally arranges stems for mixing (Vocals, Electric Guitar, Bass, Drums, Percussion, Synth).

### Phase 13 — Ecosystem Expansion & Ultimate Professional Polish
- **Advanced Multi-Format Import/Export Engine**: Export individual tracks, multi-track stems, and master mixes in high-quality `.wav`, `.mp3`, `AAC`, `OGG`, `FLAC`, `ALAC`, and `AIFF` natively.
- **Spotify Ready Standard**: Enforce true peak limits and integrated LUFS with an automatic master export preset.
- **Hardware Control Integration**: Deep native mapping for MCU/HUI motorized fader control surfaces.
- **Zero-Latency Monitoring Optimization**: Complete overhaul of the audio buffer handling for predicting buffer underruns, ensuring infinite neural plugins on standard laptop CPUs.

---

## Building & Installation

### Prerequisites
- **CMake 3.22+**
- **C++20 Compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux).
- **JUCE 7**: Retrieved automatically or mapped via `git submodule`.
- **ONNX Runtime**: Handled entirely via CMake `FetchContent` during the configuration step.

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
MIT — built on JUCE 7.
