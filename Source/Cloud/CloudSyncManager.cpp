#include "CloudSyncManager.h"

CloudSyncManager::CloudSyncManager(AppState& state)
    : appState(state)
{
    appState.getState().addListener(this);
}

CloudSyncManager::~CloudSyncManager()
{
    appState.getState().removeListener(this);
    stopTimer();
}

void CloudSyncManager::connectToServer(const juce::String& endpoint)
{
    serverEndpoint = endpoint;
    connected = true;
    startTimer(100); // Poll/sync every 100ms
}

void CloudSyncManager::disconnect()
{
    connected = false;
    stopTimer();
}

bool CloudSyncManager::isConnected() const
{
    return connected;
}

void CloudSyncManager::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& prop)
{
    if (!connected) return;
    const juce::ScopedLock sl(syncLock);
    // In a real app, generate a JSON patch diff
}

void CloudSyncManager::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (!connected) return;
    const juce::ScopedLock sl(syncLock);
}

void CloudSyncManager::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index)
{
    if (!connected) return;
    const juce::ScopedLock sl(syncLock);
}

void CloudSyncManager::timerCallback()
{
    syncPendingChanges();
}

void CloudSyncManager::syncPendingChanges()
{
    const juce::ScopedLock sl(syncLock);
    // Push differences to REST API / WebSocket
}
