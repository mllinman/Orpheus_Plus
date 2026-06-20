#include "ShortcutsSettingsPanel.h"

ShortcutsSettingsPanel::ShortcutsSettingsPanel(juce::ApplicationCommandManager& cmdManager)
    : commandManager(cmdManager)
{
    titleLabel.setText("Keyboard Shortcuts", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textPrimary());
    addAndMakeVisible(titleLabel);

    table.setModel(this);
    table.getHeader().addColumn("Command", 1, 200, 50, 400);
    table.getHeader().addColumn("Shortcut", 2, 200, 50, 400);
    table.setColour(juce::ListBox::backgroundColourId, OrpheusLookAndFeel::bgDarker());
    addAndMakeVisible(table);

    for (auto cat : commandManager.getCommandCategories())
        commandIDs.addArray(commandManager.getCommandsInCategory(cat));
}

ShortcutsSettingsPanel::~ShortcutsSettingsPanel() {}

void ShortcutsSettingsPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgPanel());
}

void ShortcutsSettingsPanel::resized()
{
    auto area = getLocalBounds().reduced(10);
    titleLabel.setBounds(area.removeFromTop(40));
    table.setBounds(area);
}

int ShortcutsSettingsPanel::getNumRows()
{
    return commandIDs.size();
}

void ShortcutsSettingsPanel::paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(OrpheusLookAndFeel::accentBlue().withAlpha(0.3f));
    else if (rowNumber % 2 == 0)
        g.fillAll(OrpheusLookAndFeel::bgDark().withAlpha(0.5f));
}

void ShortcutsSettingsPanel::paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/)
{
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(14.0f);

    if (auto* cmdTarget = commandManager.getFirstCommandTarget(commandIDs[rowNumber]))
    {
        juce::ApplicationCommandInfo info(commandIDs[rowNumber]);
        cmdTarget->getCommandInfo(commandIDs[rowNumber], info);

        juce::String text;
        if (columnId == 1)
        {
            text = info.shortName;
        }
        else if (columnId == 2)
        {
            auto keypresses = commandManager.getKeyMappings()->getKeyPressesAssignedToCommand(commandIDs[rowNumber]);
            for (auto kp : keypresses)
            {
                text += kp.getTextDescription() + "  ";
            }
        }

        g.drawText(text, 5, 0, width - 10, height, juce::Justification::centredLeft, true);
    }
}
