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
    auto trackName = name.isEmpty() ? "Audio " + juce::String(getNumTracks() + 1) : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "audio", nullptr);
    track.setProperty("name",  trackName, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);

    TrackInfo info;
    info.name = trackName;
    info.type = TrackInfo::Type::Audio;
    tracks.push_back(info);

    markDirty();
    return getNumTracks() - 1;
}

int AppState::addMidiTrack(const juce::String& name)
{
    auto trackName = name.isEmpty() ? "MIDI " + juce::String(getNumTracks() + 1) : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "midi",  nullptr);
    track.setProperty("name",  trackName, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);

    TrackInfo info;
    info.name = trackName;
    info.type = TrackInfo::Type::Midi;
    tracks.push_back(info);

    markDirty();
    return getNumTracks() - 1;
}

void AppState::removeTrack(int index)
{
    if (juce::isPositiveAndBelow(index, getNumTracks()))
    {
        valueTree.removeChild(valueTree.getChild(index), &undoManager);
        if (index < (int)tracks.size())
            tracks.erase(tracks.begin() + index);
        if (selectedTrackIndex >= (int)tracks.size())
            selectedTrackIndex = (int)tracks.size() - 1;
        markDirty();
    }
}

int AppState::getNumTracks() const
{
    return valueTree.getNumChildren();
}

void AppState::moveTrack(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex) return;
    if (!juce::isPositiveAndBelow(fromIndex, (int)tracks.size())) return;
    if (!juce::isPositiveAndBelow(toIndex, (int)tracks.size())) return;

    auto info = tracks[(size_t)fromIndex];
    tracks.erase(tracks.begin() + fromIndex);
    tracks.insert(tracks.begin() + toIndex, info);

    valueTree.moveChild(fromIndex, toIndex, &undoManager);
    if (selectedTrackIndex == fromIndex)
        selectedTrackIndex = toIndex;
    markDirty();
}

void AppState::duplicateTrack(int index)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    auto& src = tracks[(size_t)index];
    TrackInfo dup = src;
    dup.name = src.name + " (copy)";
    tracks.insert(tracks.begin() + index + 1, dup);

    auto srcVT = valueTree.getChild(index);
    auto dupVT = srcVT.createCopy();
    dupVT.setProperty("name", dup.name, nullptr);
    valueTree.addChild(dupVT, index + 1, &undoManager);
    markDirty();
}

void AppState::renameTrack(int index, const juce::String& newName)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    tracks[(size_t)index].name = newName;
    valueTree.getChild(index).setProperty("name", newName, &undoManager);
    markDirty();
}

void AppState::setTrackColor(int index, juce::Colour col)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    tracks[(size_t)index].colour = col;
    markDirty();
}

void AppState::setTrackHeight(int index, int h)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    tracks[(size_t)index].height = juce::jlimit(40, 300, h);
    markDirty();
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

    // Rebuild tracks vector from value tree
    tracks.clear();
    for (int i = 0; i < valueTree.getNumChildren(); ++i) {
        auto child = valueTree.getChild(i);
        TrackInfo info;
        info.name = child.getProperty("name", "Track " + juce::String(i + 1));
        info.type = child.getProperty("type").toString() == "midi" ? TrackInfo::Type::Midi : TrackInfo::Type::Audio;
        tracks.push_back(info);
    }

    markClean();
    sendChangeMessage();
    return true;
}

