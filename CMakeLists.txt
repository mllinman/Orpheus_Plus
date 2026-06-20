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
include(FetchContent)
FetchContent_Declare(
    onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v1.17.1/onnxruntime-win-x64-1.17.1.zip"
)
FetchContent_MakeAvailable(onnxruntime)

# We must explicitly add the include directory and link the library for ONNX Runtime since it doesn't provide standard CMake targets in the ZIP release.
set(ONNXRUNTIME_INCLUDE_DIR ${onnxruntime_SOURCE_DIR}/include)
set(ONNXRUNTIME_LIB_DIR ${onnxruntime_SOURCE_DIR}/lib)

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

    # Newly Added Missing Sources
    Source/Util/OrpheusLogger.cpp
    Source/UI/ExportDialog.cpp
    Source/UI/WorkspaceManager.cpp
    Source/UI/WorkspaceManager.h
    Source/UI/SpatialPannerUI.cpp
    Source/UI/SpatialPannerUI.h
    Source/AI/CompositionCoPilot.cpp
    Source/AI/CompositionCoPilot.h
    Source/UI/AICoPilotPanel.cpp
    Source/UI/AICoPilotPanel.h
    Source/Audio/SpatialReverbProcessor.cpp
    Source/Audio/SpatialReverbProcessor.h
    Source/Mastering/PhaseAlignmentAI.cpp
    Source/Control/MacroController.cpp

    # Audio Engine
    Source/Audio/AudioEngine.cpp
    Source/Audio/ClipGeneratorProcessor.cpp
    Source/Audio/TrackProcessor.cpp
    Source/Audio/TrackFaderProcessor.cpp
    Source/Audio/MixerProcessor.cpp
    Source/Audio/PluginManager.cpp
    Source/Audio/PluginSandbox.cpp
    Source/Audio/MidiLearnManager.cpp
    Source/Audio/TimeStretcher.cpp
    Source/Audio/ConvolutionReverbProcessor.cpp
    Source/Audio/OrpheusSynth.cpp
    Source/Audio/MidiGeneratorProcessor.cpp
    Source/Audio/ChordGeneratorProcessor.cpp
    Source/Audio/Arpeggiator.cpp
    Source/Audio/ArpeggiatorProcessor.cpp
    Source/Audio/SimpleDelayProcessor.cpp
    Source/Audio/TransientDetector.cpp
    Source/Audio/TempoFollower.cpp

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
    Source/PitchCorrection/VocalSuiteProcessor.cpp
    Source/AudioCleanup/AudioCleanupProcessor.cpp
    Source/AudioCleanup/AIHumanizerProcessor.cpp
    Source/VoiceCloning/VoiceConversionProcessor.cpp
    Source/VoiceCloning/SpeakerEmbeddingExtractor.cpp

    # Project Management
    Source/Project/ProjectManager.cpp
    Source/Project/AppState.cpp

    # UI
    Source/UI/OrpheusLookAndFeel.cpp
    Source/UI/TransportBar.cpp
    Source/UI/MixerPanel.cpp
    Source/UI/SpectrumAnalyzer.cpp
    Source/UI/PluginBrowser.cpp
    Source/UI/ActivePluginsView.cpp
    Source/UI/PluginWorkspacePanel.cpp
    Source/UI/TrackSettingsPanel.cpp
    Source/UI/StemSeparatorPanel.cpp
    Source/UI/AudioCleanupPanel.cpp
    Source/UI/AIHumanizerPanel.cpp
    Source/UI/AutoTunePanel.cpp
    Source/UI/TablaturePanel.cpp
    Source/UI/SessionViewPanel.cpp
    Source/UI/ModulationMatrixPanel.cpp
    Source/UI/VocalAutomationPanel.cpp
    Source/UI/VoiceCloningPanel.cpp
    Source/UI/AudioMidiSettingsPanel.cpp
    Source/UI/ProjectSettingsPanel.cpp
    Source/UI/DockablePanel.cpp
    Source/UI/LibraryPanel.cpp
    Source/UI/UserManualPanel.cpp
    Source/UI/PitchGamePanel.cpp
    Source/UI/ShortcutsSettingsPanel.cpp
    Source/UI/MacroControlPanel.cpp
    Source/UI/TextToSamplePanel.cpp

    # Transcription
    Source/Transcription/TablatureGenerator.cpp

    # Utility
    # Phase 8, 9, 10, 12 Expansion Modules
    Source/Hardware/MackieControlSurface.cpp
    Source/Cloud/CloudSyncManager.cpp
    Source/Video/VideoTrackComponent.cpp
    Source/Score/ScoreViewComponent.cpp
    Source/Mastering/AdvancedConvolutionReverb.cpp
    Source/Scripting/ScriptingEngine.cpp
    Source/AI/AIPluginHost.cpp
    Source/AI/ArrangementCoPilot.cpp
    Source/AI/TextToSampleGenerator.cpp
    Source/Audio/MelodyCompleter.cpp
    Source/Mastering/LatentMatchEQ.cpp
    Source/Mastering/SpectralCarver.cpp

    # Export Architecture
    Source/Export/AudioExportManager.cpp

    # ADR
    Source/AudioCleanup/ADRProcessor.cpp
    Source/UI/ADRPanel.cpp
)

target_include_directories(OrpheusPlus PRIVATE
    ${ONNXRUNTIME_INCLUDE_DIR}
)

target_compile_definitions(OrpheusPlus PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OrpheusPlus,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OrpheusPlus,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LADSPA=0
    USE_ONNX_RUNTIME=1
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
    ${ONNXRUNTIME_LIB_DIR}/onnxruntime.lib
)

# Copy the onnxruntime.dll to the output directory so the executable can find it at runtime
add_custom_command(TARGET OrpheusPlus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_LIB_DIR}/onnxruntime.dll"
        "$<TARGET_FILE_DIR:OrpheusPlus>"
)
