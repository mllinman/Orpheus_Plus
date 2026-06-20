# Orpheus Plus — Advanced JUCE C++ DAW

A complete rewrite of the Orpheus web DAW as a professional, offline-capable desktop application built with JUCE 7 and C++20. Targeting Windows, macOS, and Linux, it brings massive improvements to performance, AI integrations, and professional mixing capabilities natively to the desktop.

---

## Current Status & Completed Features

Orpheus Plus has transitioned far beyond its web origins. The core audio engine has been built, and extensive, state-of-the-art AI and workflow features have been successfully natively integrated into the app.

### Core Architecture & Playback
- **Multi-track Timeline**: High-performance arrangement view with `JUCE AudioProcessorGraph`.
- **Smart Audio Comping & Takes**: Swipe-and-drag UI for assembling perfect takes, with automatic transient detection and beat-matching.
- **Unified Key Editor (Piano Roll)**: Multi-track MIDI overlay and editing directly in the arrangement view.
- **MPE Synth Integration**: Replaced the legacy synth with a `juce::MPESynthesiser` for rich polyphonic expression, pitch bends, and per-note articulation.
- **Plugin Hosting**: Full VST3 & AU scanning, loading, and routing via `JUCE PluginManager`.

### AI & DSP Processing
- **Native ONNX Runtime**: Local, offline AI inference built directly into the C++ source via `FetchContent`—no Python runtime required.
- **Dynamic AI Mastering**: Evaluates track dynamics via RMS and Peak calculations, automatically configuring Linear Phase EQs and Multiband Compressors to achieve optimal LUFS loudness levels.
- **Vocal Suite & Pitch Correction**: Real-time pitch correction and format shifting (Auto-Tune style) built in natively without external plugins.
- **AI Voice Cloning / Profile Training**: Train and apply custom vocal profiles natively in the DAW via ONNX embedding extractors.
- **Stem Separation**: Split mixed audio tracks into isolated vocals, drums, bass, and other instruments.
- **Audio to MIDI**: Convert monophonic and polyphonic audio clips to MIDI notes instantly.
- **Vocal Pitch Game**: Real-time vocal training tool that displays user singing pitch vs correct song key to train vocality accuracy.

### Advanced Composition & Workflow
- **Generative MIDI**: Built-in AI Chord Generation (`ChordGeneratorProcessor`) and Arpeggiators.
- **Macro Controls**: Centralized knob clusters to control multiple parameters within third-party plugins directly from a single interface.
- **Custom Hotkeys**: Configurable keyboard shortcut mapping (`ShortcutsSettingsPanel`).
- **Comprehensive Automation**: Smoothly drawn automation curves via a moving average window, completely synchronized to the audio callback.
- **In-App Documentation**: An embedded, searchable User Manual to help users learn every feature on the fly.

---

## Prerequisites

### Build Tools
- **CMake 3.22+**
- **C++20 Compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux).
- **JUCE 7**: Retrieved automatically or mapped via `git submodule add https://github.com/juce-framework/JUCE.git JUCE`.
- **ONNX Runtime**: Handled entirely via CMake `FetchContent` during the configuration step.

*Note on MSVC: To prevent internal compiler errors (`C1001`) during Release builds, Link-Time Code Generation (`/GL`) is disabled for certain optimization flags in the CMake setup, or you can build in Debug mode.*

---

## Building

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

### macOS (Xcode)
```bash
cmake -B build -G Xcode
open build/OrpheusPlus.xcodeproj
```

### Windows (Visual Studio)
```bash
cmake -B build -G "Visual Studio 17 2022"
# Open build/OrpheusPlus.sln
```

---

## Future Roadmap

With the major AI and composition paradigms successfully established, the future roadmap is focused on hardware integration, ecosystem expansion, and ultimate professional polish.

### Phase 8 — Hardware & Ecosystem
- **Hardware Control Surfaces**: Full native support for Mackie Control Universal (MCU), HUI protocols, and motorized fader banks.
- **Cloud Collaboration & Sync**: Optional end-to-end encrypted cloud project sync, allowing real-time multi-user project sessions over a network.
- **Advanced Video Sync**: Import and scrub video files directly on a video track in the timeline for scoring to picture.
- **Score & Notation View**: Translate MIDI regions dynamically into standard sheet music for classical composition and printing.
- **ReWire / Advanced Output Routing**: Multi-channel surround outputs (Atmos 7.1.4) and advanced bussing.

### Phase 9 — Expansion Modules
- **Advanced Convolution Reverb**: Expansion of the existing reverb processor to allow importing user Impulse Responses (IRs) with 3D graphical room visualization.
- **Custom Scripting Environment**: Exposing internal DAW states and actions to a Lua or Python scripting engine so power users can create macro scripts or custom generative sequences.
- **Third-Party AI Plugins API**: Allowing third-party developers to plug custom ONNX/TensorFlow Lite models directly into Orpheus Plus's generic inference handlers.

---

## Migrating from Orpheus (Web)

Your existing Orpheus_Plus web project maps to this desktop C++ codebase as follows:

| Orpheus Web | Orpheus Plus JUCE |
|---|---|
| Web Audio API AudioContext | AudioEngine + AudioProcessorGraph |
| Tone.js Transport | AudioEngine transport (play/stop/BPM) |
| HTML5 Canvas timeline | TimelineComponent + TrackLaneComponent |
| Piano roll (JS) | PianoRollComponent |
| Prisma DB projects | ProjectManager (.orpheus XML files) |
| Railway server | Removed (fully offline, 100% local processing) |
| Python Microservices | Replaced by native C++ ONNX Runtime inference |
| Vite dev server | CMake build system |
| Electron wrapper | Native JUCE standalone C++ app |

---

## License
MIT — built on JUCE 7 (JUCE personal/educational license or commercial as needed).
