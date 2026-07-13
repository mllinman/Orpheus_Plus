#pragma once
#include <JuceHeader.h>

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow(juce::AudioProcessor& p, juce::AudioProcessorEditor* editor)
        : juce::DocumentWindow(p.getName(),
                               juce::Desktop::getInstance().getDefaultLookAndFeel()
                                   .findColour(juce::ResizableWindow::backgroundColourId),
                               juce::DocumentWindow::allButtons),
          processor(p)
    {
        setUsingNativeTitleBar(true);
        
        if (editor)
        {
            setContentOwned(editor, true);
        }
        else
        {
            auto* generic = new juce::GenericAudioProcessorEditor(p);
            setContentOwned(generic, true);
        }

        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    ~PluginWindow() override
    {
        // Detach? simple destruction.
    }

    void closeButtonPressed() override
    {
        // Fire callback before destruction so owner can clear its reference
        if (onClose) onClose();
        delete this;
    }

    std::function<void()> onClose;

private:
    juce::AudioProcessor& processor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
