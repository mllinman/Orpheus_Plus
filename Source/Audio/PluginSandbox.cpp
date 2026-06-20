#include "PluginSandbox.h"

// ─── PluginSandboxWorker ───────────────────────────────────────────────────────

PluginSandboxWorker::PluginSandboxWorker() : juce::InterprocessConnection(false)
{
}

PluginSandboxWorker::~PluginSandboxWorker()
{
    disconnect();
}

void PluginSandboxWorker::connectionMade()
{
    connected = true;
    juce::Logger::writeToLog("PluginSandboxWorker: Connected to host.");
}

void PluginSandboxWorker::connectionLost()
{
    connected = false;
    juce::Logger::writeToLog("PluginSandboxWorker: Lost connection to host.");
}

void PluginSandboxWorker::messageReceived(const juce::MemoryBlock& message)
{
    // IPC communication parsing logic here
    juce::ignoreUnused(message);
}

void PluginSandboxWorker::sendAudioBufferToHost(const juce::AudioBuffer<float>& buffer)
{
    // Serialize audio buffer and send over IPC
    juce::ignoreUnused(buffer);
}

// ─── PluginSandboxHost ─────────────────────────────────────────────────────────

PluginSandboxHost::PluginSandboxHost() : juce::InterprocessConnection(false)
{
}

PluginSandboxHost::~PluginSandboxHost()
{
    killSandboxProcess();
    disconnect();
}

void PluginSandboxHost::connectionMade()
{
    juce::Logger::writeToLog("PluginSandboxHost: Connected to worker process.");
}

void PluginSandboxHost::connectionLost()
{
    juce::Logger::writeToLog("PluginSandboxHost: Lost connection to worker process.");
}

void PluginSandboxHost::messageReceived(const juce::MemoryBlock& message)
{
    // Parse response from worker (e.g. processed audio)
    juce::ignoreUnused(message);
}

bool PluginSandboxHost::launchSandboxProcess(const juce::String& pluginPath)
{
    juce::ScopedLock sl(lock);
    childProcess = std::make_unique<juce::ChildProcess>();

    // Mock command line to launch our own exe with specific flags
    juce::StringArray args;
    args.add(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName());
    args.add("--plugin-sandbox");
    args.add("--path=" + pluginPath);

    if (childProcess->start(args))
    {
        childIsRunning = true;
        // In a real implementation we would open a named pipe or socket here
        // and connect()
        return true;
    }
    return false;
}

void PluginSandboxHost::killSandboxProcess()
{
    juce::ScopedLock sl(lock);
    if (childProcess && childIsRunning)
    {
        childProcess->kill();
        childIsRunning = false;
        childProcess.reset();
    }
}

void PluginSandboxHost::processAudioBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (!childIsRunning) return;

    // 1. Serialize input buffer & midi to shared memory / IPC socket
    // 2. Signal worker
    // 3. Wait (or use double buffering/lock-free IPC queue)
    // 4. Read processed output into buffer
    
    juce::ignoreUnused(buffer, midiMessages);
}
