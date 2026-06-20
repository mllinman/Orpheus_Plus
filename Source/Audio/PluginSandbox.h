#pragma once
#include <JuceHeader.h>

class PluginSandboxWorker : public juce::InterprocessConnection
{
public:
    PluginSandboxWorker();
    ~PluginSandboxWorker() override;

    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock& message) override;

    bool isConnectedToHost() const { return connected; }
    void sendAudioBufferToHost(const juce::AudioBuffer<float>& buffer);

private:
    std::atomic<bool> connected { false };
};

class PluginSandboxHost : public juce::InterprocessConnection
{
public:
    PluginSandboxHost();
    ~PluginSandboxHost() override;

    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock& message) override;

    bool launchSandboxProcess(const juce::String& pluginPath);
    void killSandboxProcess();
    
    void processAudioBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

private:
    std::unique_ptr<juce::ChildProcess> childProcess;
    std::atomic<bool> childIsRunning { false };
    
    juce::CriticalSection lock; // Only for non-realtime IPC setup, not used in processBlock
};
