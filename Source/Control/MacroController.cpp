#include "MacroController.h"

MacroController::MacroController(const juce::String& name)
    : name(name)
{
}

void MacroController::setValue(float newValue)
{
    currentValue = juce::jlimit(0.0f, 1.0f, newValue);

    for (auto& target : targets)
    {
        if (target.cachedParameter != nullptr)
        {
            // Simple depth scaling mapping: 0.0 to 1.0 applied as range based on depth
            float scaledValue = currentValue * target.depth;
            target.cachedParameter->setValueNotifyingHost(scaledValue);
        }
    }
}

void MacroController::addTarget(int trackIndex, const juce::String& paramID, float depth, juce::AudioProcessorParameter* param)
{
    targets.push_back({trackIndex, paramID, depth, param});
}

void MacroController::removeTarget(int trackIndex, const juce::String& paramID)
{
    targets.erase(std::remove_if(targets.begin(), targets.end(),
        [trackIndex, paramID](const MacroTarget& t) { 
            return t.trackIndex == trackIndex && t.paramID == paramID; 
        }), targets.end());
}

void MacroController::syncWithAudioEngine(class AudioEngine* engine)
{
    // MOCK: This would iterate over targets and map `cachedParameter` correctly via engine pointer
    // e.g. target.cachedParameter = engine->getTrack(target.trackIndex)->getParameter(target.paramID);
    juce::ignoreUnused(engine);
}
