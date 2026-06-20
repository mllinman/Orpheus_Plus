#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

class MacroControlPanel : public juce::Component
{
public:
    MacroControlPanel();
    ~MacroControlPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct MacroKnob {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
    };
    std::vector<MacroKnob> macros;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroControlPanel)
};
