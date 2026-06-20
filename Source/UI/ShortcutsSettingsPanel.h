#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

class ShortcutsSettingsPanel : public juce::Component, public juce::TableListBoxModel
{
public:
    ShortcutsSettingsPanel(juce::ApplicationCommandManager& cmdManager);
    ~ShortcutsSettingsPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(juce::Graphics&, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(juce::Graphics&, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;

private:
    juce::ApplicationCommandManager& commandManager;
    juce::TableListBox table;
    juce::Array<juce::CommandID> commandIDs;
    
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShortcutsSettingsPanel)
};
