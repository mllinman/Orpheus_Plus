#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class ScriptingEngine
{
public:
    ScriptingEngine(AudioEngine& engine);
    ~ScriptingEngine();

    juce::var executeScript(const juce::String& javascriptCode);
    void registerDAWObjects();

private:
    AudioEngine& audioEngine;
    juce::JavascriptEngine jsEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptingEngine)
};
