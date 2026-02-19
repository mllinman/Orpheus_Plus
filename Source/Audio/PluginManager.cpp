#include "PluginManager.h"
#include "AudioEngine.h"

PluginManager::PluginManager(AudioEngine& e) : engine(e)
{
    formatManager.addFormat(new juce::VST3PluginFormat());
   #if JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
   #endif

    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("OrpheusPlus/plugins.xml");
    loadPluginList(pluginListFile);
}

PluginManager::~PluginManager()
{
    auto pluginListFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                            .getChildFile("OrpheusPlus/plugins.xml");
    savePluginList(pluginListFile);
}

void PluginManager::scanForPlugins()
{
    if (scanning.load()) return;
    scanning.store(true);

    juce::Thread::launch([this]
    {
        juce::FileSearchPath paths;
        paths.addPath(getDefaultVST3Paths());
       #if JUCE_MAC
        paths.addPath(getDefaultAUPaths());
       #endif

        auto deadMansPedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                               .getChildFile("OrpheusPlus/deadmanspedal.txt");

        if (formatManager.getNumFormats() > 0)
        {
            scanner.reset(new juce::PluginDirectoryScanner(
                knownPlugins, *formatManager.getFormat(0), paths, true, deadMansPedal));
        }

        juce::String currentPluginName;
        while (scanner->scanNextFile(true, currentPluginName))
        {
            float progress = scanner->getProgress();
            juce::MessageManager::callAsync([this, progress, currentPluginName]
            {
                listeners.call(&Listener::scanProgress, progress, currentPluginName);
            });

            if (!scanning.load()) break;
        }

        scanning.store(false);
        juce::MessageManager::callAsync([this]
        {
            listeners.call(&Listener::scanComplete);
            listeners.call(&Listener::pluginListChanged);
        });
    });
}

void PluginManager::cancelScan()
{
    scanning.store(false);
}

std::unique_ptr<juce::AudioPluginInstance>
PluginManager::loadPlugin(const juce::PluginDescription& desc, juce::String& errorMessage)
{
    return formatManager.createPluginInstance(
        desc, engine.getDeviceManager().getCurrentAudioDevice()
                ? engine.getDeviceManager().getCurrentAudioDevice()->getCurrentSampleRate() : 44100.0,
        512, errorMessage);
}

void PluginManager::addPluginToTrack(int /*trackIndex*/, const juce::PluginDescription& desc)
{
    juce::String err;
    auto instance = loadPlugin(desc, err);
    if (instance)
    {
        // TODO: add to track's plugin chain in AudioProcessorGraph
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
            "Plugin Load Error", err);
    }
}

void PluginManager::removePluginFromTrack(int /*trackIndex*/, int /*pluginSlot*/)
{
    // TODO: remove from graph
}

void PluginManager::openPluginEditor(int /*trackIndex*/, int /*pluginSlot*/)
{
    // TODO: create floating plugin editor window
}

void PluginManager::savePluginList(const juce::File& file)
{
    if (auto xml = knownPlugins.createXml())
    {
        file.getParentDirectory().createDirectory();
        xml->writeTo(file);
    }
}

void PluginManager::loadPluginList(const juce::File& file)
{
    if (file.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(file))
            knownPlugins.recreateFromXml(*xml);
    }
}

void PluginManager::addSearchPath(const juce::File& path)
{
    // Stored for next scan
}

juce::FileSearchPath PluginManager::getDefaultVST3Paths()
{
    juce::FileSearchPath paths;
   #if JUCE_WINDOWS
    paths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    paths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
   #elif JUCE_MAC
    paths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    paths.add(juce::File("~/Library/Audio/Plug-Ins/VST3"));
   #elif JUCE_LINUX
    paths.add(juce::File("/usr/lib/vst3"));
    paths.add(juce::File("/usr/local/lib/vst3"));
    paths.add(juce::File("~/.vst3"));
   #endif
    return paths;
}

juce::FileSearchPath PluginManager::getDefaultAUPaths()
{
    juce::FileSearchPath paths;
   #if JUCE_MAC
    paths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
    paths.add(juce::File("~/Library/Audio/Plug-Ins/Components"));
   #endif
    return paths;
}
