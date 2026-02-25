#include "AudioMidiSettingsPanel.h"
#include "OrpheusLookAndFeel.h"

AudioMidiSettingsPanel::AudioMidiSettingsPanel(juce::AudioDeviceManager& deviceManager)
    : tabComponent(juce::TabbedButtonBar::TabsAtTop)
{
    audioSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 256,
        0, 256,
        false, false,
        true, false);

    midiSetupComp = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 0, 0, 0,
        true, true,
        false, false);

    tabComponent.addTab("Audio Connect", OrpheusLookAndFeel::bgElevated(), audioSetupComp.get(), false);
    tabComponent.addTab("MIDI Hub", OrpheusLookAndFeel::bgElevated(), midiSetupComp.get(), false);
    tabComponent.setTabBarDepth(30);

    addAndMakeVisible(tabComponent);

    setSize(520, 500);
}

AudioMidiSettingsPanel::~AudioMidiSettingsPanel()
{
}

void AudioMidiSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    auto headerArea = getLocalBounds().removeFromTop(40);
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::bgSurface(), 0, 0,
        OrpheusLookAndFeel::bgPanel(), 0, 40.0f, false));
    g.fillRect(headerArea);

    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(16.0f).boldened());
    g.drawText("AUDIO & MIDI SETTINGS", headerArea.reduced(20, 0), juce::Justification::centredLeft);
}

void AudioMidiSettingsPanel::resized()
{
    auto area = getLocalBounds().withTrimmedTop(40).reduced(10);
    tabComponent.setBounds(area);
}
