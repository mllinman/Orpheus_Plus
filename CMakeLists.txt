cmake_minimum_required(VERSION 3.22)
project(OrpheusPlus VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ────────────────────────────────────────────────────────────────────
# Clone JUCE into the project or point to your local copy:
#   git submodule add https://github.com/juce-framework/JUCE.git JUCE
add_subdirectory(JUCE)

# ── Third-party deps ─────────────────────────────────────────────────────────
# Rubber Band Library (pitch/time stretching)
# find_package(RubberBand REQUIRED)

# ONNX Runtime (AI model inference for stem sep, audio-to-MIDI, noise removal)
# find_package(onnxruntime REQUIRED)

# RNNoise (neural noise suppression) - compile from source
# add_subdirectory(third_party/rnnoise)

# ── OrpheusPlus target ───────────────────────────────────────────────────────
juce_add_gui_app(OrpheusPlus
    PRODUCT_NAME        "Orpheus Plus"
    COMPANY_NAME        "Orpheus Audio"
    BUNDLE_ID           "com.orpheusaudio.orpheusplus"

    MICROPHONE_PERMISSION_ENABLED   TRUE
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   TRUE
    PLUGINHOST_AU       TRUE
)

juce_generate_juce_header(OrpheusPlus)

target_sources(OrpheusPlus PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp

    # Audio Engine
    Source/Audio/AudioEngine.cpp
    Source/Audio/TrackProcessor.cpp
    Source/Audio/MixerProcessor.cpp
    Source/Audio/PluginManager.cpp

    # Timeline
    Source/Timeline/TimelineComponent.cpp
    Source/Timeline/TrackLaneComponent.cpp
    Source/Timeline/AudioClip.cpp
    Source/Timeline/MidiClip.cpp
    Source/Timeline/TransportController.cpp

    # Piano Roll
    Source/PianoRoll/PianoRollComponent.cpp


    # Mastering
    Source/Mastering/MasteringModule.cpp
    Source/Mastering/MultibandCompressor.cpp
    Source/Mastering/LinearPhaseEQ.cpp
    Source/Mastering/LUFSMeter.cpp

    # AI Features
    Source/StemSeparation/StemSeparator.cpp
    Source/AudioToMidi/AudioToMidiConverter.cpp
    Source/PitchCorrection/AutoTuneProcessor.cpp
    Source/AudioCleanup/AudioCleanupProcessor.cpp

    # Project Management
    Source/Project/ProjectManager.cpp
    Source/Project/AppState.cpp

    # UI
    Source/UI/OrpheusLookAndFeel.cpp
    Source/UI/TransportBar.cpp
    Source/UI/MixerPanel.cpp
    Source/UI/SpectrumAnalyzer.cpp
    Source/UI/PluginBrowser.cpp

)

target_compile_definitions(OrpheusPlus PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OrpheusPlus,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OrpheusPlus,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=0
)

target_link_libraries(OrpheusPlus PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
