#pragma once
#include <JuceHeader.h>

class AudioEngine;

//==============================================================================
class PluginManager
{
public:
    explicit PluginManager(AudioEngine& engine);
    ~PluginManager();

    //── Scanning ─────────────────────────────────────────────────────────────
    void scanForPlugins();
    void cancelScan();
    bool isScanRunning() const { return scanning.load(); }

    const juce::KnownPluginList& getKnownPluginList() const { return knownPlugins; }
    juce::KnownPluginList& getKnownPluginList() { return knownPlugins; }

    //── Loading ──────────────────────────────────────────────────────────────
    std::unique_ptr<juce::AudioPluginInstance>
    loadPlugin(const juce::PluginDescription& desc, juce::String& errorMessage);

    void addPluginToTrack(int trackIndex, const juce::PluginDescription& desc);
    void removePluginFromTrack(int trackIndex, int pluginSlot);
    void movePlugin(int trackIndex, int oldSlot, int newSlot);
    void openPluginEditor(int trackIndex, int pluginSlot);
    juce::String getPluginName(int nodeID) const;

    //── Persistence ──────────────────────────────────────────────────────────
    void savePluginList(const juce::File& file);
    void loadPluginList(const juce::File& file);

    //── Plugin search paths ──────────────────────────────────────────────────
    void addSearchPath(const juce::File& path);
    juce::FileSearchPath getDefaultVST3Paths();
    juce::FileSearchPath getDefaultAUPaths(); // macOS only

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void scanProgress(float progress, const juce::String& currentPlugin) {}
        virtual void scanComplete() {}
        virtual void pluginListChanged() {}
    };
    void addListener(Listener* l)    { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

private:
    AudioEngine& engine;
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    std::atomic<bool> scanning { false };
    std::unique_ptr<juce::PluginDirectoryScanner> scanner;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};
