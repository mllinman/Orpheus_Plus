#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

#include "../Control/MacroController.h"

class MacroControlPanel : public juce::Component, public juce::DragAndDropContainer
{
public:
    MacroControlPanel();
    ~MacroControlPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseDrag(const juce::MouseEvent& e) override;

private:
    struct MacroKnob {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<MacroController> controller;
    };
    std::vector<MacroKnob> macros;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroControlPanel)
};
