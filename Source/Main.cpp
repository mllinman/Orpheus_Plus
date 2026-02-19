#include <JuceHeader.h>
// #include "MainComponent.h"

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
        if (auto* mw = mainWindow.get())
        {
            if (mw->getMainComponent().hasUnsavedChanges())
            {
                juce::AlertWindow::showOkCancelBox(
                    juce::MessageBoxIconType::QuestionIcon,
                    "Unsaved Changes",
                    "You have unsaved changes. Quit anyway?",
                    "Quit", "Cancel",
                    nullptr,
                    juce::ModalCallbackFunction::create([this](int result) {
                        if (result == 1)
                            quit();
                    }));
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
            mainComponent = std::make_unique<MainComponent>();
            setContentOwned(mainComponent.get(), true);

            setResizable(true, true);
            setResizeLimits(1280, 720, 7680, 4320);

            centreWithSize(1600, 960);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

        MainComponent& getMainComponent() { return *mainComponent; }

    private:
        std::unique_ptr<MainComponent> mainComponent;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
START_JUCE_APPLICATION(OrpheusPlusApplication)
