#include "MacroControlPanel.h"

MacroControlPanel::MacroControlPanel()
{
    for (int i = 0; i < 8; ++i)
    {
        MacroKnob knob;
        
        knob.slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        knob.slider->setRange(0.0, 1.0, 0.01);
        knob.slider->setValue(0.5);
        addAndMakeVisible(knob.slider.get());
        
        juce::String macroName = "Macro " + juce::String(i + 1);
        knob.label = std::make_unique<juce::Label>("", macroName);
        knob.label->setJustificationType(juce::Justification::centred);
        knob.label->setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
        knob.label->setFont(12.0f);
        addAndMakeVisible(knob.label.get());

        knob.controller = std::make_unique<MacroController>(macroName);
        knob.slider->onValueChange = [this, i]() {
            macros[i].controller->setValue((float)macros[i].slider->getValue());
        };
        
        macros.push_back(std::move(knob));
    }
}

MacroControlPanel::~MacroControlPanel() {}

void MacroControlPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());
    g.setColour(OrpheusLookAndFeel::borderDefault());
    g.drawRect(getLocalBounds(), 1);
}

void MacroControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    int knobSize = 60;
    int spacing = 15;
    
    // Calculate how many knobs fit in a row
    int numColumns = juce::jmax(1, bounds.getWidth() / (knobSize + spacing));
    int knobWidth = (bounds.getWidth() - (numColumns - 1) * spacing) / numColumns;
    
    for (int i = 0; i < (int)macros.size(); ++i)
    {
        int row = i / numColumns;
        int col = i % numColumns;
        
        auto area = juce::Rectangle<int>(
            bounds.getX() + col * (knobWidth + spacing),
            bounds.getY() + row * (knobSize + 25 + spacing),
            knobWidth,
            knobSize + 25
        );
        
        // Center the knob inside the cell if cell is wider
        auto knobArea = area.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize);
        macros[i].slider->setBounds(knobArea);
        macros[i].label->setBounds(area);
    }
}

void MacroControlPanel::mouseDrag(const juce::MouseEvent& e)
{
    for (int i = 0; i < (int)macros.size(); ++i)
    {
        if (macros[i].label->getBounds().contains(e.getMouseDownPosition()) ||
            macros[i].slider->getBounds().contains(e.getMouseDownPosition()))
        {
            // Start drag
            startDragging(macros[i].controller->getName(), this, juce::Image(), true);
            break;
        }
    }
}
