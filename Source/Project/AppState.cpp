#include "AppState.h"

AppState::AppState()
{
    valueTree.setProperty("bpm",        bpm,        nullptr);
    valueTree.setProperty("timeSigNum", timeSigNum, nullptr);
    valueTree.setProperty("timeSigDen", timeSigDen, nullptr);
    valueTree.setProperty("sampleRate", sampleRate, nullptr);
    valueTree.setProperty("name",       projectName, nullptr);
}

AppState::~AppState() {}

int AppState::addAudioTrack(const juce::String& name)
{
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "audio", nullptr);
    track.setProperty("name",  name.isEmpty() ? "Audio " + juce::String(getNumTracks() + 1) : name, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);
    markDirty();
    return getNumTracks() - 1;
}

int AppState::addMidiTrack(const juce::String& name)
{
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "midi",  nullptr);
    track.setProperty("name",  name.isEmpty() ? "MIDI " + juce::String(getNumTracks() + 1) : name, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);
    markDirty();
    return getNumTracks() - 1;
}

void AppState::removeTrack(int index)
{
    if (juce::isPositiveAndBelow(index, getNumTracks()))
    {
        valueTree.removeChild(valueTree.getChild(index), &undoManager);
        markDirty();
    }
}

int AppState::getNumTracks() const
{
    return valueTree.getNumChildren();
}

void AppState::undo() { undoManager.undo(); }
void AppState::redo() { undoManager.redo(); }

std::unique_ptr<juce::XmlElement> AppState::toXml() const
{
    return valueTree.createXml();
}

bool AppState::fromXml(const juce::XmlElement& xml)
{
    auto tree = juce::ValueTree::fromXml(xml);
    if (!tree.isValid()) return false;

    valueTree  = tree;
    bpm        = valueTree.getProperty("bpm",         120.0);
    timeSigNum = valueTree.getProperty("timeSigNum",  4);
    timeSigDen = valueTree.getProperty("timeSigDen",  4);
    sampleRate = valueTree.getProperty("sampleRate",  48000);
    projectName = valueTree.getProperty("name",       "Untitled");

    markClean();
    sendChangeMessage();
    return true;
}
