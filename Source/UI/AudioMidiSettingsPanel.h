#pragma once
#include <JuceHeader.h>

//==============================================================================
class AudioMidiSettingsPanel : public juce::Component
{
public:
    AudioMidiSettingsPanel(juce::AudioDeviceManager& deviceManager);
    ~AudioMidiSettingsPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::TabbedComponent tabComponent;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSetupComp;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> midiSetupComp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioMidiSettingsPanel)
};
