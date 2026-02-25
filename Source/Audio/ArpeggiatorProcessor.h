#pragma once
#include <JuceHeader.h>
#include <vector>
#include <set>

class ArpeggiatorProcessor : public juce::AudioProcessor
{
public:
    ArpeggiatorProcessor();
    ~ArpeggiatorProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    const juce::String getName() const override { return "Arpeggiator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    std::set<int> activeNotes;
    int currentNoteIndex = 0;
    double timeSinceLastNote = 0.0;
    int lastNotePlayed = -1;
    
    // Simple parameters
    double rateDivision = 0.25; // 1/16th note default
    float gateLength = 0.8f;    // 80% of step size

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArpeggiatorProcessor)
};
