#pragma once
#include <JuceHeader.h>
#include "../Audio/AudioEngine.h"

class OSCManager : public juce::OSCReceiver,
                   public juce::OSCReceiver::Listener<juce::OSCReceiver::MessageLoopCallback>
{
public:
    OSCManager(AudioEngine& engine);
    ~OSCManager() override;

    bool connect(int receivePort, const juce::String& targetHost, int sendPort);
    void disconnect();

    void oscMessageReceived(const juce::OSCMessage& message) override;
    void oscBundleReceived(const juce::OSCBundle& bundle) override;

    void sendFeedback(const juce::String& address, float value);

private:
    AudioEngine& audioEngine;
    juce::OSCSender sender;
    bool isConnected = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSCManager)
};
