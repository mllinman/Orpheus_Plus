#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"
#include "OrpheusLookAndFeel.h"
#include "SpatialPannerUI.h"

//==============================================================================
class TrackSettingsPanel : public juce::Component,
                           public juce::ChangeListener,
                           private juce::Timer
{
public:
    TrackSettingsPanel(AudioEngine& engine, AppState& state);
    ~TrackSettingsPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void setTrackIndex(int index);
    int  getTrackIndex() const { return currentTrack; }

private:
    void timerCallback() override;
    void refreshFromTrack();
    void paintSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                      const juce::String& title);

    AudioEngine& audioEngine;
    AppState&    appState;
    int          currentTrack = -1;

    // ── Track Info ──
    juce::Label      trackNameLabel;
    juce::TextEditor trackNameEditor;
    juce::Label      trackTypeLabel;

    // ── Volume / Pan ──
    juce::Slider volumeKnob;
    juce::Label  volumeLabel    { {}, "VOL" };
    juce::Label  volumeReadout;
    juce::Slider panKnob;
    juce::Label  panLabel       { {}, "PAN" };
    juce::Label  panReadout;

    // ── Input / Output Routing ──
    juce::Label    inputLabel   { {}, "INPUT" };
    juce::ComboBox inputCombo;
    juce::Label    outputLabel  { {}, "OUTPUT" };
    juce::ComboBox outputCombo;

    // ── Plugin Chain (8 insert slots) ──
    juce::Label          insertsLabel { {}, "INSERTS" };
    static constexpr int NUM_INSERT_SLOTS = 8;
    std::array<juce::TextButton, NUM_INSERT_SLOTS> insertSlots;
    std::array<juce::ToggleButton, NUM_INSERT_SLOTS> insertBypasses;

    // ── Sends (4 slots) ──
    juce::Label      sendsLabel { {}, "SENDS" };
    static constexpr int NUM_SENDS = 4;
    std::array<juce::Slider, NUM_SENDS>   sendLevelKnobs;
    std::array<juce::ComboBox, NUM_SENDS> sendDestCombos;
    std::array<juce::Label, NUM_SENDS>    sendLabels;

    // ── Inline EQ (4 bands) ──
    juce::Label  eqLabel { {}, "EQ" };
    juce::ToggleButton eqEnable { "EQ" };
    static constexpr int NUM_EQ_BANDS = 4;
    struct EQBandControls {
        juce::Slider freq;
        juce::Slider gain;
        juce::Slider q;
        juce::Label  label;
    };
    std::array<EQBandControls, NUM_EQ_BANDS> eqBands;

    // ── Quick Access Toggles ──
    juce::ToggleButton autoTuneToggle  { "Vocal Suite" };
    juce::ToggleButton cleanupToggle   { "Cleanup" };

    // ── Spatial Audio ──
    juce::Label spatialLabel { {}, "SPATIAL" };
    std::unique_ptr<SpatialPannerUI> spatialPanner;

    // ── Mute / Solo / Arm ──
    juce::ToggleButton muteBtn  { "M" };
    juce::ToggleButton soloBtn  { "S" };
    juce::ToggleButton armBtn   { "R" };

    juce::Viewport viewport;
    juce::Component contentArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackSettingsPanel)
};
