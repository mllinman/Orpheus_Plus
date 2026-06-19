#pragma once
#include <JuceHeader.h>
#include "OrpheusLookAndFeel.h"

class UserManualPanel : public juce::Component, public juce::ListBoxModel
{
public:
    UserManualPanel();
    ~UserManualPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;

private:
    void populateManualContent();
    void loadTopic(int index);

    juce::ListBox topicsList;
    juce::TextEditor contentEditor;
    
    struct ManualTopic {
        juce::String title;
        juce::String content;
    };
    
    std::vector<ManualTopic> topics;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UserManualPanel)
};
