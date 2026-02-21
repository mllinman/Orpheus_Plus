#pragma once
#include <JuceHeader.h>
#include "OrpheusSynth.h"
#include "AudioEngine.h"

/**
 * A processor that renders MIDI clips using the built-in OrpheusSynth.
 */
class MidiGeneratorProcessor : public juce::AudioProcessor
{
public:
    MidiGeneratorProcessor(OrpheusTrackInfo& info, AudioEngine& engine);
    ~MidiGeneratorProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getName() const override { return "MIDI Synth Generator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    void setPlayhead(double pos) { currentPlayhead.store(pos); }

private:
    OrpheusTrackInfo& trackInfo;
    AudioEngine& audioEngine;
    OrpheusSynth synth;
    std::atomic<double> currentPlayhead { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiGeneratorProcessor)
};
