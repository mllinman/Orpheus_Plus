#include "ProjectManager.h"
#include "../Audio/AudioEngine.h"
#include "../Timeline/AudioClip.h"

ProjectManager::ProjectManager(AppState& s, AudioEngine& e)
    : appState(s), audioEngine(e)
{
    loadSettings();
}

ProjectManager::~ProjectManager()
{
    saveSettings();
}

void ProjectManager::loadSettings()
{
    auto props = juce::PropertiesFile::Options().getDefaultFile().getParentDirectory()
                      .getChildFile("OrpheusPlus/settings.xml");
    
    if (props.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(props))
        {
            defaultProjectDir = juce::File(xml->getStringAttribute("DefaultProjectDirectory", 
                juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Orpheus Projects").getFullPathName()));
            copyAudioOnSave = xml->getBoolAttribute("CopyAudioOnSave", true);
            return;
        }
    }
    
    defaultProjectDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Orpheus Projects");
    copyAudioOnSave = true;
}

void ProjectManager::saveSettings()
{
    auto propsFile = juce::PropertiesFile::Options().getDefaultFile().getParentDirectory()
                      .getChildFile("OrpheusPlus/settings.xml");
                      
    propsFile.getParentDirectory().createDirectory();

    auto xml = std::make_unique<juce::XmlElement>("OrpheusSettings");
    if (propsFile.existsAsFile())
    {
        if (auto existing = juce::XmlDocument::parse(propsFile))
            xml = std::move(existing);
    }
    
    xml->setAttribute("DefaultProjectDirectory", defaultProjectDir.getFullPathName());
    xml->setAttribute("CopyAudioOnSave", copyAudioOnSave);
    
    xml->writeTo(propsFile);
}

void ProjectManager::setDefaultProjectDirectory(const juce::File& dir)
{
    defaultProjectDir = dir;
    saveSettings();
}

void ProjectManager::setCopyAudioOnSave(bool shouldCopy)
{
    copyAudioOnSave = shouldCopy;
    saveSettings();
}

void ProjectManager::newProject()
{
    if (appState.isDirty())
    {
        juce::AlertWindow::showOkCancelBox(
            juce::MessageBoxIconType::QuestionIcon,
            "New Project",
            "Discard unsaved changes?",
            "Discard", "Cancel",
            nullptr,
            juce::ModalCallbackFunction::create([this](int result) {
                if (result == 1)
                {
                    currentFile = juce::File{};
                    appState.fromXml(*juce::XmlDocument::parse("<OrpheusProject/>"));
                    appState.markClean();
                }
            }));
        return;
    }

    currentFile = juce::File{};
    appState.markClean();
}

void ProjectManager::openProject()
{
    auto chooser = std::make_shared<juce::FileChooser>("Open Project",
        defaultProjectDir,
        "*.orph");

    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                         juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
                loadProjectFile(file);
        });
}

void ProjectManager::saveProject()
{
    if (currentFile.existsAsFile())
        saveProjectFile(currentFile);
    else
        saveProjectAs();
}

void ProjectManager::saveProjectAs()
{
    auto chooser = std::make_shared<juce::FileChooser>("Save Project As",
        defaultProjectDir,
        "*.orph");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File{})
            {
                // Ensure it has .orph extension
                auto f = file.withFileExtension("orph");
                
                // If it's not inside a project folder of the same name, we should ideally create one.
                juce::String projectName = f.getFileNameWithoutExtension();
                juce::File projectFolder = f.getParentDirectory();
                
                // If the user selected a directory that isn't named after the project, create it
                if (projectFolder.getFileName() != projectName)
                {
                    projectFolder = projectFolder.getChildFile(projectName);
                    projectFolder.createDirectory();
                    f = projectFolder.getChildFile(f.getFileName());
                }
                
                appState.setProjectName(projectName);
                
                if (saveProjectFile(f))
                    currentFile = f;
            }
        });
}

bool ProjectManager::loadProjectFile(const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse(file))
    {
        if (appState.fromXml(*xml))
        {
            currentFile = file;
            addRecentFile(file);
            return true;
        }
    }
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
        "Load Error", "Could not load project file: " + file.getFullPathName());
    return false;
}

bool ProjectManager::saveProjectFile(const juce::File& file)
{
    // Make sure project directory exists
    file.getParentDirectory().createDirectory();

    // Copy audio files if needed
    if (copyAudioOnSave)
    {
        copyAudioFilesToProjectFolder(file.getParentDirectory());
    }

    if (auto xml = appState.toXml())
    {
        if (xml->writeTo(file))
        {
            appState.markClean();
            addRecentFile(file);
            return true;
        }
    }
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
        "Save Error", "Could not save project to: " + file.getFullPathName());
    return false;
}

void ProjectManager::copyAudioFilesToProjectFolder(const juce::File& projectFolder)
{
    juce::File audioFolder = projectFolder.getChildFile("Audio");
    audioFolder.createDirectory();

    auto& tracks = appState.getTracks();
    auto stateTree = appState.getValueTree();
    
    // We would iterate through tracks and clips. Since we don't have direct access to AudioClip objects here,
    // we need to search the ValueTree for audio clips and copy their source files.
    
    for (auto trackNode : stateTree.getChildWithName("Tracks"))
    {
        for (auto clipNode : trackNode.getChildWithName("Clips"))
        {
            if (clipNode.getType().toString() == "AudioClip")
            {
                juce::File sourceFile(clipNode.getProperty("sourceFile").toString());
                if (sourceFile.existsAsFile())
                {
                    // Copy to Audio folder
                    juce::File destFile = audioFolder.getChildFile(sourceFile.getFileName());
                    
                    // Only copy if it's not already in the Audio folder
                    if (sourceFile.getParentDirectory() != audioFolder)
                    {
                        if (sourceFile.copyFileTo(destFile))
                        {
                            // Update the ValueTree to point to the new relative/absolute location
                            clipNode.setProperty("sourceFile", destFile.getFullPathName(), nullptr);
                        }
                    }
                }
            }
        }
    }
}

juce::StringArray ProjectManager::getRecentFiles() const
{
    auto props = juce::PropertiesFile::Options().getDefaultFile().getParentDirectory()
                      .getChildFile("OrpheusPlus/settings.xml").getParentDirectory();
    (void)props;
    // TODO: load from application properties
    return {};
}

void ProjectManager::addRecentFile(const juce::File& file)
{
    // TODO: persist to application properties
    juce::ignoreUnused(file);
}
