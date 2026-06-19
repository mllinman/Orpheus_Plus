#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Project/AppState.h"

//==============================================================================
struct MidiLearnSlider : public juce::Slider
{
    std::function<void()> onMidiLearn;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown()) {
            juce::PopupMenu m;
            m.addItem(1, "Learn MIDI CC");
            m.showMenuAsync(juce::PopupMenu::Options{}, [this](int res) {
                if (res == 1 && onMidiLearn) onMidiLearn();
            });
        } else {
            juce::Slider::mouseDown(e);
        }
    }
};

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

    int masterStripWidth = 100;
    juce::StretchableLayoutManager horizontalLayout;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerBar;

    struct ChannelStrip : public juce::Component
    {
        ChannelStrip(int idx, AudioEngine& e);
        ~ChannelStrip() override;

        void resized() override;
        void paint(juce::Graphics&) override;
        void paintOverChildren(juce::Graphics&) override;

        int trackIndex;
        AudioEngine& engine;
        juce::Slider& getFader() { return fader; }

    private:
        MidiLearnSlider  fader;
        MidiLearnSlider  panKnob;
        MidiLearnSlider  sweetenerKnob;
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
            void mouseDrag(const juce::MouseEvent& e) override;
        };

        juce::OwnedArray<PluginSlot> pluginSlots;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
    };

    // A specialized strip for the Master bus
    struct MasterStrip : public juce::Component
    {
        MasterStrip(AudioEngine& e);
        ~MasterStrip() override;

        void resized() override;
        void paint(juce::Graphics&) override;

        AudioEngine& engine;
        
    private:
        MidiLearnSlider  fader;
        juce::Label      nameLabel;
        float peakL = 0.0f, peakR = 0.0f;

        juce::TextButton autoMixBtn { "AI Auto-Mix" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterStrip)
    };

    juce::OwnedArray<ChannelStrip> strips;
    std::unique_ptr<MasterStrip>   masterStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};
