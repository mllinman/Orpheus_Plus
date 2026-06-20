#pragma once
#include <JuceHeader.h>
#include "../Project/AppState.h"

class CloudSyncManager : public juce::ValueTree::Listener, public juce::Timer
{
public:
    CloudSyncManager(AppState& state);
    ~CloudSyncManager() override;

    void connectToServer(const juce::String& endpoint);
    void disconnect();
    bool isConnected() const;

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    // Timer
    void timerCallback() override;

private:
    void syncPendingChanges();

    AppState& appState;
    juce::String serverEndpoint;
    bool connected = false;
    
    juce::ValueTree pendingChanges; // A holding tree for diffs
    juce::CriticalSection syncLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudSyncManager)
};
