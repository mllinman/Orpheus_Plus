# Orpheus Plus — The Ultimate AI Studio Co-Producer

Orpheus Plus is a state-of-the-art, fully offline Desktop Digital Audio Workstation (DAW) built in JUCE 8 and C++20. It represents a paradigm shift in modern music production, shifting the focus entirely towards **flawless recording, mastering, fine-tuning, and editing—all powered by real-time AI.**

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

### Core Architecture & UI
- **Multi-track Timeline**: High-performance arrangement view with comping and beat-matching.
- **Dynamic Workspace Manager**: A fully customizable, dockable panel system allowing users to tear off, resize, and construct their perfect studio layout. 
- **Glassmorphic Design**: A breathtaking, top-tier aesthetic powered by modern UI rendering techniques, frosted glass blurring, and dynamic accent gradients.
- **Project Settings & Smart Directories**: Dedicated `.orph` files with intelligent auto-creation of `Audio/` sub-folders, keeping sessions flawlessly organized.
- **Universal Export Engine**: Render master mixes and stems natively in WAV, FLAC, OGG, ALAC, AIFF, MP3, and AAC with customizable bit depths and sample rates.
- **Spotify Mastering Preset**: One-click export enforcing exact -14 LUFS / -1 dB True Peak standards dynamically.

### AI Processing & DSP
- **Native ONNX Runtime**: Local inference built directly into the C++ graph.
- **The Flawless Vocal Suite & Humanization**: AI-driven pitch correction and a post-processing neural layer that guarantees the vocal takes sound organically recorded by injecting realistic breath mapping, sibilance, and subtle analog imperfections to outsmart AI detectors.
- **Timbre Transfer**: Deep fake vocals replacing timbre while retaining exact timing.
- **Latent Space Match-EQ**: Upload a reference track, and the AI learns its exact characteristics dynamically.
- **Phase Alignment AI**: Automatically phase-align multi-mic recordings.
- **Stem Auto-Arranger Expansion**: Advanced extraction that separates, renames, categorizes, and optimally arranges stems for mixing (Vocals, Electric Guitar, Bass, Drums, Percussion, Synth).
- **Dynamic AI Mastering**: Automatically configures Linear Phase EQs and Multiband Compressors to achieve optimal LUFS loudness levels.

---

## The All-New Roadmap

With the foundational AI processing and dynamic UI ecosystem completed, the new roadmap aims to push Orpheus Plus into the realm of Spatial Audio, Generative AI Composition, and Cinematic Scoring.

### Phase 15 — Spatial Audio & Immersive Mixing
- **Immersive Engine**: Native routing for 7.1.4 Dolby Atmos and Ambisonics, with a binaural renderer for headphone monitoring.
- **3D Panner UI**: A modern, interactive spatial panner integrated into the `TrackSettingsPanel` and `MixerPanel`.
- **Spatial AI Reverb**: Convolution reverb that maps impulse responses into 3D space dynamically.

### Phase 16 — AI Composition Co-Pilot
- **Generative Progression Engine**: AI-driven chord progression suggestions that adapt to the user's genre and style.
- **Melody Autocomplete**: Feed a 4-bar melody and have the Co-Pilot generate infinite, stylistically matching variations.
- **Rhythm Style Transfer**: Extract the groove/timing from an audio file and apply it instantly to a MIDI piano roll sequence.

### Phase 17 — The Scoring & Cinematic Ecosystem
- **Video Playback Engine**: Frame-accurate video synchronization for film scoring and sound design.
- **Dialogue Leveling & Foley Match**: AI tools to automatically clean up location audio and match ADR (Automated Dialogue Replacement) to the original acoustic space.
- **Score View Enhancement**: Printing and exporting traditional sheet music directly from MIDI clips.

### Phase 18 — GPU Acceleration & Zero-Latency Overhaul
- **ONNX DirectML / CoreML Integration**: Offloading neural network processing to the GPU to free up the CPU for audio threads.
- **Lock-Free Architecture**: A complete audit and refactor of the audio graph using lock-free rings and atomic structures to achieve absolute zero-latency monitoring.
- **Plugin Sandboxing**: Isolating VST3/AU plugins in separate processes to prevent external crashes from bringing down the DAW.

### Phase 19 — Generative Sound Design & Advanced Spectral Processing
- **Text-to-Sample Generation**: A dedicated panel for localized, ONNX-driven text-to-audio generation, allowing users to type "vintage 808 kick with tape saturation" and instantly drop it into the timeline.
- **AI Spectral Carving**: Intelligent unmasking that analyzes multiple tracks and dynamically sculpts out conflicting frequencies in real-time.
- **Advanced Macro Modulation Matrix**: A global LFO and Envelope routing matrix capable of targeting any parameter in the DAW for complex, generative sound design.

---

## Building & Installation

### Prerequisites
- **CMake 3.22+**
- **C++20 Compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux).
- **JUCE 8**: Retrieved automatically or mapped via `git submodule`.
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
MIT — built on JUCE 8.
