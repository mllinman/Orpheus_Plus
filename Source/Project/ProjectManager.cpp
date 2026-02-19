#include "ProjectManager.h"
#include "../Audio/AudioEngine.h"

ProjectManager::ProjectManager(AppState& s, AudioEngine& e)
    : appState(s), audioEngine(e)
{}

ProjectManager::~ProjectManager() {}

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
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.orpheus");

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
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.orpheus");

    chooser->launchAsync(juce::FileBrowserComponent::saveMode |
                         juce::FileBrowserComponent::warnAboutOverwriting,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file != juce::File{})
            {
                auto f = file.withFileExtension("orpheus");
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
