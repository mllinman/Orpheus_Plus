#pragma once
#include <JuceHeader.h>

class AppState;

//==============================================================================
struct StemSeparationResult
{
    juce::File vocals;
    juce::File drums;
    juce::File bass;
    juce::File guitar;
    juce::File piano;
    juce::File other;
};

//==============================================================================
class StemSeparator
{
public:
    enum class Model { Demucs4, Demucs4HQ, Spleeter2, Spleeter4, Spleeter5 };

    StemSeparator();
    ~StemSeparator();

    void setModel(Model m) { currentModel = m; }
    Model getModel() const { return currentModel; }

    // Asynchronously separate stems, calls onComplete when done
    void separate(const juce::File& inputFile,
                  AppState& appState,
                  std::function<void(StemSeparationResult)> onComplete = {});

    void cancel();
    bool isRunning() const { return running.load(); }
    float getProgress() const { return progress.load(); }

    // Check if AI backend is available
    static bool isBackendAvailable();
    static juce::File getModelPath(Model m);

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void stemSeparationProgress(float progress) {}
        virtual void stemSeparationComplete(const StemSeparationResult& result) {}
        virtual void stemSeparationFailed(const juce::String& error) {}
    };
    void addListener(Listener* l)    { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

private:
    void runSeparation(const juce::File& inputFile,
                       const juce::File& outputDir,
                       std::function<void(StemSeparationResult)> onComplete);

    bool runDemucs(const juce::File& inputFile, const juce::File& outputDir);
    bool runSpleeter(const juce::File& inputFile, const juce::File& outputDir);
    StemSeparationResult collectResults(const juce::File& outputDir);

    Model currentModel = Model::Demucs4;
    std::atomic<bool>  running  { false };
    std::atomic<float> progress { 0.0f  };
    juce::Thread::ThreadID separationThread = nullptr;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemSeparator)
};
