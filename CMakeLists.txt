cmake_minimum_required(VERSION 3.24)
project(OrpheusPlus VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── JUCE ────────────────────────────────────────────────────────────────────
add_subdirectory(JUCE)

# ── Third-party deps ─────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    onnxruntime
    URL "https://github.com/microsoft/onnxruntime/releases/download/v1.29.0/onnxruntime-win-x64-1.29.0.zip"
)
FetchContent_MakeAvailable(onnxruntime)

set(ONNXRUNTIME_INCLUDE_DIR ${onnxruntime_SOURCE_DIR}/include)
set(ONNXRUNTIME_LIB_DIR ${onnxruntime_SOURCE_DIR}/lib)

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

    # Utility
    Source/Util/OrpheusLogger.cpp

    # UI
    Source/UI/ExportDialog.cpp
    Source/UI/WorkspaceManager.cpp
    Source/UI/SpatialPannerUI.cpp
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
    Source/UI/AutoMixPanel.cpp
    Source/UI/SmartTrackFixerPanel.cpp
    Source/UI/ToolbarComponent.cpp
    Source/UI/CollapsiblePanel.cpp
    Source/UI/StatusBar.cpp
    Source/UI/AICoPilotPanel.cpp
    Source/UI/ADRPanel.cpp
    Source/UI/DistributionPrepPanel.cpp
    Source/UI/SettingsHubPanel.cpp

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
    Source/Audio/SpatialReverbProcessor.cpp
    Source/Audio/OrpheusSynth.cpp
    Source/Audio/MidiGeneratorProcessor.cpp
    Source/Audio/ChordGeneratorProcessor.cpp
    Source/Audio/Arpeggiator.cpp
    Source/Audio/ArpeggiatorProcessor.cpp
    Source/Audio/SimpleDelayProcessor.cpp
    Source/Audio/TransientDetector.cpp
    Source/Audio/TempoFollower.cpp
    Source/Audio/MelodyCompleter.cpp

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
    Source/Mastering/PhaseAlignmentAI.cpp
    Source/Mastering/AdvancedConvolutionReverb.cpp
    Source/Mastering/LatentMatchEQ.cpp
    Source/Mastering/SpectralCarver.cpp

    # AI Features
    Source/StemSeparation/StemSeparator.cpp
    Source/AudioToMidi/AudioToMidiConverter.cpp
    Source/PitchCorrection/VocalSuiteProcessor.cpp
    Source/AudioCleanup/AudioCleanupProcessor.cpp
    Source/AudioCleanup/AIHumanizerProcessor.cpp
    Source/AudioCleanup/ADRProcessor.cpp
    Source/AudioCleanup/DistributionPrepProcessor.cpp
    Source/VoiceCloning/VoiceConversionProcessor.cpp
    Source/VoiceCloning/SpeakerEmbeddingExtractor.cpp
    Source/AI/AIPluginHost.cpp
    Source/AI/ArrangementCoPilot.cpp
    Source/AI/CompositionCoPilot.cpp
    Source/AI/TextToSampleGenerator.cpp
    Source/AI/AutoMixer.cpp
    Source/AI/TrackElementClassifier.cpp
    Source/AI/SmartTrackFixer.cpp
    Source/AI/VariationGeneratorAI.cpp

    # Project Management
    Source/Project/ProjectManager.cpp
    Source/Project/AppState.cpp

    # Transcription
    Source/Transcription/TablatureGenerator.cpp

    # Phase 8, 9, 10, 12 Expansion Modules
    Source/Hardware/MackieControlSurface.cpp
    Source/Hardware/OSCManager.cpp
    Source/Cloud/CloudSyncManager.cpp
    Source/Video/VideoTrackComponent.cpp
    Source/Score/ScoreViewComponent.cpp
    Source/Scripting/ScriptingEngine.cpp
    Source/Control/MacroController.cpp
    Source/Control/MultiBreakpointEnvelope.cpp

    # Export Architecture
    Source/Export/AudioExportManager.cpp
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
    juce::juce_osc
    juce::juce_recommended_config_flags
    ${ONNXRUNTIME_LIB_DIR}/onnxruntime.lib
)

# ── MSVC ICE Workarounds & Build Hardening ────────────────────────────────────
if(MSVC)
    target_compile_options(OrpheusPlus PRIVATE
        /bigobj          # Support large object files (100+ source files)
        /d2FH4-          # Disable FH4 exception handling (avoids linker ICE)
        /FS              # Force synchronous PDB writes (parallel build safety)
        /Zc:__cplusplus  # Report correct C++ standard to headers
        /GL-             # Disable Whole Program Optimization (avoids C1001 ICE)
        /W4              # High warning level for future-proofing
        /wd4100          # Suppress: unreferenced formal parameter (common in JUCE overrides)
        /wd4244          # Suppress: conversion from double to float (common in audio code)
        /wd4267          # Suppress: conversion from size_t (common in JUCE containers)
    )

    set(CMAKE_CXX_FLAGS_RELEASE "/O1 /DNDEBUG" CACHE STRING "Release flags" FORCE)
    set(CMAKE_C_FLAGS_RELEASE   "/O1 /DNDEBUG" CACHE STRING "Release flags" FORCE)
endif()

# Copy the onnxruntime.dll to the output directory so the executable can find it at runtime
add_custom_command(TARGET OrpheusPlus POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ONNXRUNTIME_LIB_DIR}/onnxruntime.dll"
        "$<TARGET_FILE_DIR:OrpheusPlus>"
)
