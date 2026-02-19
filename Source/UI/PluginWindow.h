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
        delete this; // managed by itself? or owner?
        // If managed by PluginManager/AudioEngine, we should notify or just hide.
        // For now, let's assume valid pointer ownership elsewhere or self-delete if simple.
        // Better: standard JUCE way for independent windows is `delete this` on close, 
        // BUT we need to clear the reference in PluginManager/Map.
        // So maybe just setVisible(false) ?
        
        // Actually, let's use a callback.
        if (onClose) onClose();
        delete this;
    }

    std::function<void()> onClose;

private:
    juce::AudioProcessor& processor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
