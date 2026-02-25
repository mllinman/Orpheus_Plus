#pragma once
#include <JuceHeader.h>
#include <fstream>
#include <mutex>

//==============================================================================
// Centralized logger for Orpheus Plus — captures errors, warnings, info,
// and unhandled crashes to a rotating log file.
//==============================================================================
class OrpheusLogger : public juce::Logger
{
public:
    enum class Level { Debug, Info, Warning, Error, Fatal };

    static OrpheusLogger& getInstance();

    //── Initialise / Shutdown ─────────────────────────────────────────────
    void initialise();
    void shutdown();

    //── Logging helpers ───────────────────────────────────────────────────
    static void logDebug  (const juce::String& msg) { getInstance().log(Level::Debug,   msg); }
    static void logInfo   (const juce::String& msg) { getInstance().log(Level::Info,    msg); }
    static void logWarning(const juce::String& msg) { getInstance().log(Level::Warning, msg); }
    static void logError  (const juce::String& msg) { getInstance().log(Level::Error,   msg); }
    static void logFatal  (const juce::String& msg) { getInstance().log(Level::Fatal,   msg); }

    //── Crash / exception capture ─────────────────────────────────────────
    static void installCrashHandler();

    //── Log file location ─────────────────────────────────────────────────
    static juce::File getLogDirectory();
    static juce::File getCurrentLogFile();

    void log(Level level, const juce::String& message);

    // juce::Logger override — captures JUCE's own DBG / Logger::writeToLog
    void logMessage(const juce::String& message) override;

private:
    OrpheusLogger() = default;
    ~OrpheusLogger() override;

    void rotateLogFiles();
    juce::String levelToString(Level level) const;
    juce::String getTimestamp() const;

    std::mutex  fileMutex;
    std::ofstream logStream;
    bool initialised = false;

    static constexpr int    MAX_LOG_FILES   = 5;
    static constexpr size_t MAX_LOG_SIZE_KB = 2048; // 2 MB per file

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrpheusLogger)
};
