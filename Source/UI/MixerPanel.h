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

        ChannelStrip(int idx, AudioEngine& e) : trackIndex(idx), engine(e)
        {
            auto& info = engine.getTrackInfo(trackIndex);
            nameLabel.setText(info.name, juce::dontSendNotification);
            nameLabel.setFont(juce::Font(10.0f));
            nameLabel.setJustificationType(juce::Justification::centred);
            nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible(nameLabel);

            fader.setSliderStyle(juce::Slider::LinearVertical);
            fader.setRange(0.0, 1.5, 0.001);
            fader.setValue(1.0, juce::dontSendNotification);
            fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            fader.onValueChange = [this] { engine.setTrackVolume(trackIndex, (float)fader.getValue()); };
            addAndMakeVisible(fader);

            panKnob.setSliderStyle(juce::Slider::Rotary);
            panKnob.setRange(-1.0, 1.0, 0.001);
            panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            panKnob.onValueChange = [this] { engine.setTrackPan(trackIndex, (float)panKnob.getValue()); };
            addAndMakeVisible(panKnob);

            muteBtn.setToggleable(true);
            muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffFFB300));
            muteBtn.onClick = [this] { engine.setTrackMute(trackIndex, muteBtn.getToggleState()); };
            addAndMakeVisible(muteBtn);

            soloBtn.setToggleable(true);
            soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00BCD4));
            soloBtn.onClick = [this] { engine.setTrackSolo(trackIndex, soloBtn.getToggleState()); };
            addAndMakeVisible(soloBtn);
        }

        void resized() override
        {
            auto b = getLocalBounds().reduced(2);
            nameLabel.setBounds(b.removeFromTop(14));
            auto btnRow = b.removeFromBottom(24);
            muteBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(1));
            soloBtn.setBounds(btnRow.reduced(1));
            panKnob.setBounds(b.removeFromTop(40).reduced(4));
            fader.setBounds(b.reduced(4, 0));
        }

        void paint(juce::Graphics& g) override
        {
            auto& info = engine.getTrackInfo(trackIndex);
            g.fillAll(juce::Colour(0xff1a1a2e));
            g.setColour(info.colour.withAlpha(0.6f));
            g.fillRect(0, 0, getWidth(), 3);
            g.setColour(juce::Colour(0xff0d0d1a));
            g.drawRect(getLocalBounds());

            // Level meter
            float meterW = 6.0f;
            float meterX = (float)(getWidth() - 16);
            float meterH = (float)(getHeight() - 50);
            float meterY = 14.0f;

            g.setColour(juce::Colour(0xff0d0d1a));
            g.fillRect(meterX, meterY, meterW * 2 + 2, meterH);

            auto meterColour = [](float peak) {
                return peak > 0.9f ? juce::Colour(0xffe94560) :
                       peak > 0.7f ? juce::Colour(0xffffd54f) :
                                     juce::Colour(0xff4caf50);
            };

            g.setColour(meterColour(peakL));
            g.fillRect(meterX, meterY + meterH * (1.0f - peakL), meterW, meterH * peakL);
            g.setColour(meterColour(peakR));
            g.fillRect(meterX + meterW + 2, meterY + meterH * (1.0f - peakR), meterW, meterH * peakR);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
    };

    juce::OwnedArray<ChannelStrip> strips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};
