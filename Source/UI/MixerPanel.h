#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

//==============================================================================
class MixerPanel : public juce::Component,
                   public AudioEngine::Listener,
                   private juce::Timer
{
public:
    MixerPanel(AudioEngine& engine, AppState& state);
    ~MixerPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void trackListChanged() override;

private:
    void timerCallback() override;
    void rebuildStrips();

    AudioEngine& audioEngine;
    AppState&    appState;

    juce::Viewport channelViewport;
    juce::Component channelContainer;

    struct ChannelStrip : public juce::Component
    {
        int trackIndex;
        AudioEngine& engine;

        juce::Slider     fader;
        juce::Slider     panKnob;
        juce::TextButton muteBtn  { "M" };
        juce::TextButton soloBtn  { "S" };
        juce::Label      nameLabel;
        float peakL = 0.0f, peakR = 0.0f;

        struct PluginSlot : public juce::TextButton,
                            public juce::DragAndDropTarget
        {
            int trackIndex;
            int slotIndex;
            AudioEngine& engine;
            
            PluginSlot(int trk, int slot, AudioEngine& e) 
                : trackIndex(trk), slotIndex(slot), engine(e) {}
            
            bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
            void itemDropped(const SourceDetails& dragSourceDetails) override;
        };

        juce::OwnedArray<PluginSlot> pluginSlots;

        ChannelStrip(int idx, AudioEngine& e);
        ~ChannelStrip() override;

        void resized() override;
        void paint(juce::Graphics&) override;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
    };

    juce::OwnedArray<ChannelStrip> strips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};
