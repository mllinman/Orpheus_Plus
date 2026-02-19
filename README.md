# Orpheus Plus — Advanced JUCE C++ DAW

A complete rewrite of the Orpheus web DAW as a professional, offline-capable desktop application
built with JUCE 7 and C++20. Targeting Windows, macOS, and Linux.

---

## Features

| Feature | Status | Technology |
|---|---|---|
| Multi-track timeline | ✅ Core | JUCE AudioProcessorGraph |
| Audio clips (WAV/AIFF/MP3/FLAC) | ✅ Core | JUCE AudioFormatManager |
| MIDI clips + Piano Roll | ✅ Core | JUCE MidiMessageSequence |
| VST3 / AU plugin hosting | ✅ Core | JUCE PluginManager |
| Real-time mixer with level meters | ✅ Core | JUCE DSP |
| Advanced mastering module | ✅ Core | JUCE DSP / Custom |
| LUFS / True Peak metering | ✅ Core | Custom EBU R128 |
| Mid/Side EQ | ✅ Core | Custom |
| Multiband compressor | ✅ Core | Custom |
| Spectrum analyzer | ✅ Core | JUCE FFT |
| AI Stem Separation | ✅ AI (Demucs v4) | Python + ChildProcess |
| Audio to MIDI | ✅ AI (Basic Pitch) | Python + ChildProcess |
| Auto-Tune / Pitch Correction | ✅ DSP (YIN + Phase Vocoder) | Custom C++ |
| Audio Cleanup (noise, de-ess, hum) | ✅ DSP | Custom C++ |
| Neural Noise Removal | ✅ AI (RNNoise) | Python + ChildProcess |
| Project save/load (.orpheus) | ✅ Core | JUCE ValueTree + XML |
| Undo/Redo | ✅ Core | JUCE UndoManager |
| MIDI transport control (MMC) | ✅ Core | JUCE MIDI |
| Export mix (WAV/AIFF/FLAC) | ✅ Core | JUCE AudioFormatWriter |
| Export stems | ✅ Core | Per-track offline render |
| Dark theme | ✅ UI | Custom LookAndFeel |

---

## Prerequisites

### Build Tools
- **CMake 3.22+**
- **C++20 compiler**: MSVC 2022 (Windows), Xcode 14+ (macOS), GCC 12+ / Clang 15+ (Linux)
- **JUCE 7**: `git submodule add https://github.com/juce-framework/JUCE.git JUCE`

### Optional (AI features)
Install these Python packages for AI-powered features:

```bash
pip install demucs          # Stem separation (Demucs v4)
pip install basic-pitch     # Audio to MIDI (Spotify Basic Pitch)
pip install crepe           # Monophonic pitch detection
pip install librosa pretty_midi  # CREPE + onset detection helpers
pip install rnnoise-python soundfile  # Neural noise removal
```

### Optional (Better pitch shifting)
For production-quality pitch correction, integrate **Rubber Band Library**:
```
https://breakfastquay.com/rubberband/
```
Replace the phase vocoder in `AutoTuneProcessor.cpp` with Rubber Band calls.

---

## Building

```bash
# 1. Clone and get JUCE submodule
git clone https://github.com/yourusername/OrpheusPlus.git
cd OrpheusPlus
git submodule add https://github.com/juce-framework/JUCE.git JUCE
git submodule update --init --recursive

# 2. Configure
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

## Project Structure

```
OrpheusPlus/
├── CMakeLists.txt
├── JUCE/                          ← JUCE submodule
├── Source/
│   ├── Main.cpp                   ← App entry point
│   ├── MainComponent.h/.cpp       ← Top-level UI + menu + command routing
│   ├── Audio/
│   │   ├── AudioEngine.h/.cpp     ← Core audio engine, transport, track mgmt
│   │   └── PluginManager.h/.cpp   ← VST3/AU scanning + loading
│   ├── Timeline/
│   │   ├── TimelineComponent.h/.cpp     ← Main arranger view
│   │   ├── TrackLaneComponent.h/.cpp    ← Individual track row + clips
│   │   └── TransportController.h/.cpp  ← MMC MIDI transport
│   ├── PianoRoll/
│   │   └── PianoRollComponent.h/.cpp   ← MIDI editor
│   ├── Mastering/
│   │   └── MasteringModule.h/.cpp      ← EQ + comp + limiter + LUFS meter
│   ├── StemSeparation/
│   │   └── StemSeparator.h/.cpp        ← Demucs v4 / Spleeter integration
│   ├── AudioToMidi/
│   │   └── AudioToMidiConverter.h/.cpp ← Basic Pitch / CREPE integration
│   ├── PitchCorrection/
│   │   └── AutoTuneProcessor.h/.cpp    ← YIN pitch detection + phase vocoder
│   ├── AudioCleanup/
│   │   └── AudioCleanupProcessor.h/.cpp ← Noise reduction, de-click, de-ess, hum
│   ├── Project/
│   │   ├── AppState.h/.cpp             ← ValueTree state + undo
│   │   └── ProjectManager.h/.cpp       ← File load/save
│   └── UI/
│       ├── OrpheusLookAndFeel.h/.cpp   ← Dark theme
│       ├── TransportBar.h/.cpp         ← Playback controls + BPM + meters
│       ├── MixerPanel.h/.cpp           ← Channel strip mixer
│       ├── SpectrumAnalyzer.h/.cpp     ← FFT spectrum display
│       └── PluginBrowser.h/.cpp        ← Plugin list + scan
```

---

## Keyboard Shortcuts

| Action | Shortcut |
|---|---|
| New Project | Cmd/Ctrl + N |
| Open Project | Cmd/Ctrl + O |
| Save | Cmd/Ctrl + S |
| Save As | Cmd/Ctrl + Shift + S |
| Play / Pause | Space |
| Stop | Enter |
| Record | Cmd/Ctrl + R |
| Add Audio Track | Cmd/Ctrl + T |
| Add MIDI Track | Cmd/Ctrl + Shift + T |
| Export Mix | Cmd/Ctrl + E |
| Export Stems | Cmd/Ctrl + Shift + E |
| Plugin Browser | Cmd/Ctrl + P |
| Undo | Cmd/Ctrl + Z |
| Redo | Cmd/Ctrl + Shift + Z |
| Zoom In (Timeline) | Cmd/Ctrl + Scroll Up |
| Zoom Out (Timeline) | Cmd/Ctrl + Scroll Down |

---

## Development Roadmap

### Phase 1 — Core ✅ (This codebase)
- Audio engine, timeline, MIDI, VST hosting, mastering, AI features, project management

### Phase 2 — Polish
- [ ] Full AudioProcessorGraph wiring (clip → FX chain → master bus)
- [ ] Per-track plugin chains with drag-and-drop reordering
- [ ] Automation lanes per parameter
- [ ] MIDI learn for all controls
- [ ] Rubber Band Library integration for better pitch shifting

### Phase 3 — Advanced
- [ ] ONNX Runtime direct integration (run Demucs/BasicPitch natively, no Python)
- [ ] Convolution reverb with IR loading
- [ ] Built-in synth using JUCE's Synthesiser class
- [ ] Sidechain routing
- [ ] ReWire / VST3 instrument tracks
- [ ] Comping and takes

### Phase 4 — Pro
- [ ] Hardware control surface support (Mackie Control, HUI)
- [ ] Video sync
- [ ] Cloud project sync (optional, using your existing Railway backend)
- [ ] Score/notation view

---

## Migrating from Orpheus (Web)

Your existing Orpheus_Plus web project maps to this codebase as follows:

| Orpheus Web | Orpheus Plus JUCE |
|---|---|
| Web Audio API AudioContext | AudioEngine + AudioProcessorGraph |
| Tone.js Transport | AudioEngine transport (play/stop/BPM) |
| HTML5 Canvas timeline | TimelineComponent + TrackLaneComponent |
| Piano roll (JS) | PianoRollComponent |
| Prisma DB projects | ProjectManager (.orpheus XML files) |
| Railway server | Removed (fully offline) |
| Vite dev server | CMake build system |
| Electron wrapper | Native JUCE standalone app |

---

## License
MIT — built on JUCE 7 (JUCE personal/educational license or commercial as needed).
