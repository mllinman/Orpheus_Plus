#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"
#include "../Timeline/SessionClip.h"

class SessionViewPanel : public juce::Component, public AudioEngine::Listener
{
public:
    SessionViewPanel(AudioEngine& engine);
    ~SessionViewPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // AudioEngine::Listener
    void trackListChanged() override;

private:
    void drawGrid(juce::Graphics& g);
    void drawClipCell(juce::Graphics& g, int trackIdx, int sceneIdx, juce::Rectangle<int> bounds);
    
    AudioEngine& audioEngine;
    
    struct Scene {
        juce::String name;
    };
    std::vector<Scene> scenes;
    
    // We mock the session clip grid here. In a full architecture, this grid would
    // live inside AudioEngine or a dedicated SessionManager.
    // [trackIndex][sceneIndex]
    std::vector<std::vector<std::unique_ptr<SessionClip>>> clipGrid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionViewPanel)
};
