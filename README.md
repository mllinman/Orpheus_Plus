# Orpheus Plus — The Ultimate AI Studio Co-Producer

Orpheus Plus is a state-of-the-art, fully offline Desktop Digital Audio Workstation (DAW) built in JUCE 8 and C++20. It represents a paradigm shift in modern music production, shifting the focus entirely towards **flawless recording, mastering, fine-tuning, and editing—all powered by real-time AI.**

The ultimate goal of Orpheus Plus is simple: **Perfect audio recorded every time.** It achieves this by combining latency-free playback with deeply embedded neural networks that autocorrect pitch, timing, and timbre without ever sounding robotic or machine-generated, cleaning up AI-generated tracks, and seamlessly separating elements into manageable stems.

---

## The Vision: The Future of Production

### 1. The Human Vocal Guarantee & AI Humanization
Through the use of advanced local ONNX models, Orpheus Plus dynamically injects unvoiced stochastic analog noise, breath mapping, and human-like micro-variations into autocorrected and AI-generated tracks. Heavy pitch correction, formant shifting, voice cloning, and AI-generated music sound organically performed by a real band.

### 2. Zero-Latency AI Playback
Orpheus Plus natively integrates ONNX Runtime via C++ directly into the `AudioProcessorGraph`. This allows dense neural acoustic models (Stem Separation, AI Humanizer, Generative Diffusion) to run in real-time alongside traditional VST3/AU plugins on the GPU (DirectML/CoreML).

### 3. Absolute Offline Autonomy & Project Management
The DAW is entirely self-contained. No subscriptions, no cloud latency, no external Python APIs. Every neural network runs 100% locally. Projects are smartly managed using the `.orph` format, automatically organizing and safely containing all audio files.

---

## Completed Architecture Milestones (Phases 1-19)

- **Dynamic Workspace Manager & Glassmorphic UI**: Fully customizable, dockable panel system with breathtaking aesthetic rendering.
- **ONNX GPU Acceleration**: Zero-latency neural network inference offloaded to the GPU.
- **Lock-Free Audio Engine**: Absolute zero-latency monitoring utilizing atomic operations and `AbstractFifo` architectures.
- **Plugin Sandboxing**: Out-Of-Process (OOP) VST3/AU hosting ensuring the DAW never crashes from third-party plugins.
- **Generative Sound Design**: Local Text-to-Sample generation directly dropping AI-generated sounds into the timeline.
- **AI Spectral Carving & Latent Match EQ**: Intelligent multi-band unmasking and mastering.
- **Cinematic & Spatial Ecosystem**: Frame-accurate video playback, Foley ADR, 3D Dolby Atmos panning, and MusicXML score generation.

---

## The Future Roadmap (Phases 20-24)

As Orpheus Plus transcends traditional DAW capabilities, the next evolution focuses on absolute creative freedom, advanced automation, and future-proofed engineering.

### Phase 20 — Auto-Mixing Master Assistant
- **Automated Mixing Engine**: An AI that "listens" to the entire session and automatically balances faders, EQ, and panning to achieve a professional mix.
- **Reference Track Sync**: Drag and drop a commercial reference track; the AI will continuously adapt the mix bus compressor, saturation, and multi-band EQ to match the target's sonic signature dynamically.

### Phase 21 — Visual Node-Based DSP Sandbox
- **Modular Patching Environment**: A Max/MSP or PureData-style node-based sandbox directly inside the DAW.
- **Custom Synth & FX Creation**: Users can drag oscillators, filters, neural network blocks, and math nodes, wiring them together visually to create proprietary synthesizers and effects that compile at runtime.

### Phase 22 — Cloud Collaboration & Intelligent Versioning
- **Real-Time Studio Sync**: Low-latency network protocol allowing multiple producers to co-edit the same `.orph` session over the internet seamlessly.
- **AI Version Diffing**: A system that analyzes two saved versions of a mix and generates a verbal summary of the differences (e.g., "In Version 2, the snare drum has 2dB more presence at 4kHz and the bass is more heavily sidechained").

### Phase 23 — Polyphonic Transcription 2.0 & MPE
- **Intelligent Audio-To-MIDI**: Converting complex, full polyphonic audio (like an acoustic guitar or piano) into separated MIDI tracks with velocity and articulation data.
- **MIDI Polyphonic Expression (MPE) Engine**: Full support for advanced MPE controllers (like ROLI) natively integrated into the Piano Roll and Auto-Arranger.

### Phase 24 — Cross-Platform Build System Overhaul & CI/CD
- **Universal Binary & Linux Support**: Upgrading the CMake architecture for universal Apple Silicon/Intel binaries and expanding support for Linux Wayland standalone environments.
- **Automated CI/CD Pipelines**: Integration with GitHub Actions for automated nightly builds, static analysis, and regression testing of the audio graph.
- **WASM Web-Player Export**: The ability to export a playable, interactive mix directly to WebAssembly for embedding in portfolios and websites.

---

## Building & Installation

### Prerequisites
- **CMake 3.22+**
- **C++20 Compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux).
- **JUCE 8**: Retrieved automatically or mapped via `git submodule`.
- **ONNX Runtime**: Handled entirely via CMake `FetchContent`.

### Instructions
```bash
# 1. Clone and fetch submodules
git clone https://github.com/yourusername/OrpheusPlus.git
cd OrpheusPlus
git submodule update --init --recursive

# 2. Configure (CMake handles downloading ONNX Runtime & linking JUCE)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release

# 4. Run
./build/OrpheusPlus_artefacts/Release/OrpheusPlus
```

---

## License
MIT — built on JUCE 8.
