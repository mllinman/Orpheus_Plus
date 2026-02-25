#include "OrpheusLogger.h"
#include <csignal>
#include <iostream>

//==============================================================================
OrpheusLogger& OrpheusLogger::getInstance()
{
    static OrpheusLogger instance;
    return instance;
}

OrpheusLogger::~OrpheusLogger()
{
    shutdown();
}

//==============================================================================
juce::File OrpheusLogger::getLogDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
             .getChildFile("OrpheusPlus/Logs");
}

juce::File OrpheusLogger::getCurrentLogFile()
{
    return getLogDirectory().getChildFile("orpheus.log");
}

//==============================================================================
void OrpheusLogger::initialise()
{
    if (initialised) return;

    auto logDir = getLogDirectory();
    logDir.createDirectory();

    rotateLogFiles();

    auto logFile = getCurrentLogFile();
    logStream.open(logFile.getFullPathName().toStdString(), std::ios::app);

    if (logStream.is_open())
    {
        initialised = true;
        juce::Logger::setCurrentLogger(this);
        installCrashHandler();

        log(Level::Info, "========================================");
        log(Level::Info, "Orpheus Plus started");
        log(Level::Info, "Version: " + juce::String(ProjectInfo::versionString));
        log(Level::Info, "OS: " + juce::SystemStats::getOperatingSystemName());
        log(Level::Info, "CPU: " + juce::String(juce::SystemStats::getNumCpus()) + " cores");
        log(Level::Info, "RAM: " + juce::String(juce::SystemStats::getMemorySizeInMegabytes()) + " MB");
        log(Level::Info, "========================================");
    }
}

void OrpheusLogger::shutdown()
{
    if (!initialised) return;

    log(Level::Info, "Orpheus Plus shutting down normally.");
    log(Level::Info, "========================================\n");

    juce::Logger::setCurrentLogger(nullptr);
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logStream.is_open())
        logStream.close();
    initialised = false;
}

//==============================================================================
void OrpheusLogger::rotateLogFiles()
{
    auto logFile = getCurrentLogFile();
    if (!logFile.existsAsFile()) return;

    // Check size
    auto sizeKB = logFile.getSize() / 1024;
    if (sizeKB < (int64)MAX_LOG_SIZE_KB) return;

    // Rotate: orpheus.4.log -> deleted, orpheus.3.log -> .4, ... orpheus.log -> .1
    auto dir = getLogDirectory();
    for (int i = MAX_LOG_FILES - 1; i >= 1; --i)
    {
        auto older = dir.getChildFile("orpheus." + juce::String(i) + ".log");
        auto newer = (i == 1) ? logFile : dir.getChildFile("orpheus." + juce::String(i - 1) + ".log");

        if (i == MAX_LOG_FILES - 1 && older.existsAsFile())
            older.deleteFile();

        if (newer.existsAsFile())
            newer.moveFileTo(older);
    }
}

//==============================================================================
juce::String OrpheusLogger::getTimestamp() const
{
    return juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S");
}

juce::String OrpheusLogger::levelToString(Level level) const
{
    switch (level)
    {
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO ";
        case Level::Warning: return "WARN ";
        case Level::Error:   return "ERROR";
        case Level::Fatal:   return "FATAL";
    }
    return "?????";
}

void OrpheusLogger::log(Level level, const juce::String& message)
{
    auto line = "[" + getTimestamp() + "] [" + levelToString(level) + "] " + message;

    {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (logStream.is_open())
        {
            logStream << line.toStdString() << std::endl;
            logStream.flush();
        }
    }

    // Also write to debug output
    DBG(line);
}

void OrpheusLogger::logMessage(const juce::String& message)
{
    // JUCE's internal Logger callback — route to our log
    log(Level::Info, "[JUCE] " + message);
}

//==============================================================================
// Crash handler: writes a final log entry before the process dies
//==============================================================================
static void orpheusCrashSignalHandler(int signal)
{
    const char* sigName = "UNKNOWN";
    switch (signal)
    {
        case SIGSEGV: sigName = "SIGSEGV (Segmentation Fault)"; break;
        case SIGABRT: sigName = "SIGABRT (Abort)"; break;
        case SIGFPE:  sigName = "SIGFPE (Floating Point Exception)"; break;
        case SIGILL:  sigName = "SIGILL (Illegal Instruction)"; break;
#ifdef SIGBUS
        case SIGBUS:  sigName = "SIGBUS (Bus Error)"; break;
#endif
    }

    // Write directly to file — can't use std::string/juce in signal handler safely
    auto logFile = OrpheusLogger::getCurrentLogFile();
    FILE* f = fopen(logFile.getFullPathName().toRawUTF8(), "a");
    if (f)
    {
        fprintf(f, "\n!!! CRASH DETECTED !!!\n");
        fprintf(f, "Signal: %s (%d)\n", sigName, signal);
        fprintf(f, "This crash log was written by the signal handler.\n");
        fprintf(f, "Check the lines above for the last operation before the crash.\n");
        fprintf(f, "========================================\n\n");
        fflush(f);
        fclose(f);
    }

    // Re-raise to get default behaviour (core dump / abort)
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

void OrpheusLogger::installCrashHandler()
{
    std::signal(SIGSEGV, orpheusCrashSignalHandler);
    std::signal(SIGABRT, orpheusCrashSignalHandler);
    std::signal(SIGFPE,  orpheusCrashSignalHandler);
    std::signal(SIGILL,  orpheusCrashSignalHandler);
#ifdef SIGBUS
    std::signal(SIGBUS,  orpheusCrashSignalHandler);
#endif
}
