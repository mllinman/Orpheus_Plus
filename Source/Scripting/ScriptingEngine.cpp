#include "ScriptingEngine.h"

ScriptingEngine::ScriptingEngine(AudioEngine& engine)
    : audioEngine(engine)
{
    registerDAWObjects();
}

ScriptingEngine::~ScriptingEngine()
{
}

juce::var ScriptingEngine::executeScript(const juce::String& javascriptCode)
{
    return jsEngine.evaluate(javascriptCode);
}

void ScriptingEngine::registerDAWObjects()
{
    // Register top-level DAW interface for Javascript
    // e.g. jsEngine.registerNativeObject("DAW", myDynamicObjectWrapper);
}
