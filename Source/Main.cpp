#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class OrpheusPlusApplication : public juce::JUCEApplication
{
public:
    OrpheusPlusApplication() {}

    const juce::String getApplicationName() override    { return "Orpheus Plus"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise(const juce::String&) override
    {
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        if (mainWindow != nullptr)
        {
            auto* mc = dynamic_cast<MainComponent*>(mainWindow->getContentComponent());
            if (mc != nullptr && mc->hasUnsavedChanges())
            {
                auto options = juce::MessageBoxOptions()
                    .withIconType(juce::MessageBoxIconType::QuestionIcon)
                    .withTitle("Unsaved Changes")
                    .withMessage("You have unsaved changes. Do you want to save before quitting?")
                    .withButton("Save")
                    .withButton("Don't Save")
                    .withButton("Cancel");

                juce::AlertWindow::showAsync(options, [this, mc](int result)
                {
                    if (result == 0 || result == 3) return;  // Cancel or closed
                    if (result == 1)  // Save
                        mc->getCommandManager().invokeDirectly(MainComponent::cmdSaveProject, false);
                    // result == 2 => Don't Save
                    quit();
                });
                return;
            }
        }
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override {}

    //==============================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(juce::ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            setResizable(true, true);
            setResizeLimits(800, 500, 10000, 10000);
            centreWithSize(1280, 800);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(OrpheusPlusApplication)
