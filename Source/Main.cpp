#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

class VocalHarmonizerApplication : public juce::JUCEApplication
{
public:
    VocalHarmonizerApplication() {}

    const juce::String getApplicationName() override    { return "Vocal Harmonizer"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }

    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow (getApplicationName()));
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow (juce::String name)
            : DocumentWindow (name,
                               juce::Desktop::getInstance().getDefaultLookAndFeel()
                                   .findColour (juce::ResizableWindow::backgroundColourId),
                               DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent(), true);

            centreWithSize (getWidth(), getHeight());
            setResizable (true, true);
            setVisible (true);
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (VocalHarmonizerApplication)
