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
        if (target.parameter != nullptr)
        {
            // Simple linear scaling for now
            float scaledValue = target.minScaling + currentValue * (target.maxScaling - target.minScaling);
            target.parameter->setValueNotifyingHost(scaledValue);
        }
    }
}

void MacroController::addTarget(juce::AudioProcessorParameter* param, float minScaling, float maxScaling)
{
    targets.push_back({param, minScaling, maxScaling});
}

void MacroController::removeTarget(juce::AudioProcessorParameter* param)
{
    targets.erase(std::remove_if(targets.begin(), targets.end(),
        [param](const Target& t) { return t.parameter == param; }), targets.end());
}
