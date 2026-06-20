#pragma once
#include <JuceHeader.h>

class AudioEngine;

class ScriptingEngine
{
public:
    ScriptingEngine(AudioEngine& engine) {}
    ~ScriptingEngine() {}

    juce::var executeScript(const juce::String& code) { return {}; }
    void registerDAWObjects() {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingEngine)
};
