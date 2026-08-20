#include "OSCManager.h"

OSCManager::OSCManager(AudioEngine& engine)
    : audioEngine(engine)
{
}

OSCManager::~OSCManager()
{
    disconnect();
}

bool OSCManager::connect(int receivePort, const juce::String& targetHost, int sendPort)
{
    disconnect();

    if (!juce::OSCReceiver::connect(receivePort))
        return false;

    if (!sender.connect(targetHost, sendPort))
    {
        juce::OSCReceiver::disconnect();
        return false;
    }

    juce::OSCReceiver::addListener(this);
    isConnected = true;
    return true;
}

void OSCManager::disconnect()
{
    if (isConnected)
    {
        juce::OSCReceiver::removeListener(this);
        juce::OSCReceiver::disconnect();
        sender.disconnect();
        isConnected = false;
    }
}

void OSCManager::oscMessageReceived(const juce::OSCMessage& message)
{
    auto address = message.getAddressPattern().toString();
    
    // Example: /track/1/volume 0.75
    if (address.startsWith("/track/"))
    {
        auto parts = juce::StringArray::fromTokens(address, "/", "");
        if (parts.size() >= 3)
        {
            int trackIndex = parts[1].getIntValue();
            juce::String param = parts[2];
            
            if (param == "volume" && message.size() == 1 && message[0].isFloat32())
            {
                audioEngine.setTrackVolume(trackIndex, message[0].getFloat32());
                // Send feedback back to the controller
                sendFeedback(address, message[0].getFloat32());
            }
            else if (param == "pan" && message.size() == 1 && message[0].isFloat32())
            {
                audioEngine.setTrackPan(trackIndex, message[0].getFloat32());
                sendFeedback(address, message[0].getFloat32());
            }
        }
    }
}

void OSCManager::oscBundleReceived(const juce::OSCBundle& bundle)
{
    for (const auto& element : bundle)
    {
        if (element.isMessage())
            oscMessageReceived(element.getMessage());
        else if (element.isBundle())
            oscBundleReceived(element.getBundle());
    }
}

void OSCManager::sendFeedback(const juce::String& address, float value)
{
    if (isConnected)
    {
        juce::OSCMessage msg(address);
        msg.addFloat32(value);
        sender.send(msg);
    }
}
