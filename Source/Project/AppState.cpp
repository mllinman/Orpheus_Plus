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

int AppState::addVocalTrack(const juce::String& name)
{
    auto trackName = name.isEmpty() ? "Vocal " + juce::String(getNumTracks() + 1) : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "vocal", nullptr);
    track.setProperty("name",  trackName, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);

    TrackInfo info;
    info.name   = trackName;
    info.type   = TrackInfo::Type::Vocal;
    info.colour = juce::Colour(0xfffd79a8); // pink
    tracks.push_back(info);

    markDirty();
    return getNumTracks() - 1;
}

int AppState::addInstrumentTrack(const juce::String& name)
{
    auto trackName = name.isEmpty() ? "Instrument " + juce::String(getNumTracks() + 1) : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "instrument", nullptr);
    track.setProperty("name",  trackName, nullptr);
    track.setProperty("vol",   1.0,     nullptr);
    track.setProperty("pan",   0.0,     nullptr);
    track.setProperty("mute",  false,   nullptr);
    track.setProperty("solo",  false,   nullptr);
    valueTree.addChild(track, -1, &undoManager);

    TrackInfo info;
    info.name   = trackName;
    info.type   = TrackInfo::Type::Instrument;
    info.colour = juce::Colour(0xff00cec9); // teal
    tracks.push_back(info);

    markDirty();
    return getNumTracks() - 1;
}

int AppState::addFolderTrack(const juce::String& name)
{
    auto trackName = name.isEmpty() ? "Folder " + juce::String(getNumTracks() + 1) : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "folder", nullptr);
    track.setProperty("name",  trackName, nullptr);
    track.setProperty("expanded", true,   nullptr);
    valueTree.addChild(track, -1, &undoManager);

    TrackInfo info;
    info.name   = trackName;
    info.type   = TrackInfo::Type::Folder;
    info.colour = juce::Colour(0xfff39c12); // orange
    tracks.push_back(info);

    markDirty();
    return getNumTracks() - 1;
}

int AppState::addArrangerTrack(const juce::String& name)
{
    auto trackName = name.isEmpty() ? "Arranger" : name;
    auto track = juce::ValueTree("Track");
    track.setProperty("type",  "arranger", nullptr);
    track.setProperty("name",  trackName, nullptr);
    valueTree.addChild(track, 0, &undoManager); // Arranger usually at top

    TrackInfo info;
    info.name   = trackName;
    info.type   = TrackInfo::Type::Arranger;
    info.colour = juce::Colours::white;
    info.height = 40;
    tracks.insert(tracks.begin(), info);

    markDirty();
    return 0; // Inserted at top
}

void AppState::removeTrack(int index)
{
    if (juce::isPositiveAndBelow(index, getNumTracks()))
    {
        auto node = getTrackNode(index);
        if (node.isValid())
            node.getParent().removeChild(node, &undoManager);
        
        // Let's just rebuild the tracks vector from XML to keep it simple and accurate
        auto xml = valueTree.createXml();
        fromXml(*xml); 
        
        if (selectedTrackIndex >= (int)tracks.size())
            selectedTrackIndex = (int)tracks.size() - 1;
        markDirty();
    }
}

juce::ValueTree AppState::getTrackNode(int flatIndex) const
{
    int current = 0;
    std::function<juce::ValueTree(juce::ValueTree)> findNode = [&](juce::ValueTree node) -> juce::ValueTree {
        if (node.hasType("Track")) {
            if (current == flatIndex) return node;
            current++;
        }
        for (int i = 0; i < node.getNumChildren(); ++i) {
            auto child = node.getChild(i);
            auto found = findNode(child);
            if (found.isValid()) return found;
        }
        return juce::ValueTree();
    };
    
    // valueTree itself is not "Track", its children are
    for (int i = 0; i < valueTree.getNumChildren(); ++i) {
        auto found = findNode(valueTree.getChild(i));
        if (found.isValid()) return found;
    }
    return juce::ValueTree();
}

int AppState::getNumTracks() const
{
    return (int)tracks.size();
}

void AppState::moveTrack(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex) return;
    if (!juce::isPositiveAndBelow(fromIndex, (int)tracks.size())) return;
    if (!juce::isPositiveAndBelow(toIndex, (int)tracks.size())) return;

    auto fromNode = getTrackNode(fromIndex);
    auto toNode = getTrackNode(toIndex);
    if (!fromNode.isValid() || !toNode.isValid()) return;
    
    // Reorder in DOM
    auto parent = fromNode.getParent();
    if (parent == toNode.getParent()) {
        parent.moveChild(parent.indexOf(fromNode), parent.indexOf(toNode), &undoManager);
    } else {
        // Between different folders
        fromNode.getParent().removeChild(fromNode, &undoManager);
        toNode.getParent().addChild(fromNode, toNode.getParent().indexOf(toNode), &undoManager);
    }

    if (selectedTrackIndex == fromIndex)
        selectedTrackIndex = toIndex;
        
    // Rebuild flat logic
    auto xml = valueTree.createXml();
    fromXml(*xml);
    markDirty();
}

void AppState::duplicateTrack(int index)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    auto nodeToCopy = getTrackNode(index);
    if (nodeToCopy.isValid()) {
        auto copy = nodeToCopy.createCopy();
        copy.setProperty("name", copy.getProperty("name").toString() + " (copy)", &undoManager);
        nodeToCopy.getParent().addChild(copy, nodeToCopy.getParent().indexOf(nodeToCopy) + 1, &undoManager);
    }
    
    auto xml = valueTree.createXml();
    fromXml(*xml);
    markDirty();
}

void AppState::renameTrack(int index, const juce::String& newName)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    tracks[(size_t)index].name = newName;
    auto node = getTrackNode(index);
    if (node.isValid()) node.setProperty("name", newName, &undoManager);
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

void AppState::setTrackExpanded(int index, bool expanded)
{
    if (!juce::isPositiveAndBelow(index, (int)tracks.size())) return;
    tracks[(size_t)index].expanded = expanded;
    auto node = getTrackNode(index);
    if (node.isValid()) node.setProperty("expanded", expanded, &undoManager);
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
    
    std::function<void(const juce::ValueTree&, int, bool)> parseNode = [&](const juce::ValueTree& node, int depth, bool parentVisible) {
        for (int i = 0; i < node.getNumChildren(); ++i) {
            auto child = node.getChild(i);
            if (child.hasType("Track")) {
                TrackInfo info;
                info.name = child.getProperty("name", "Track " + juce::String(tracks.size() + 1));
                auto typeStr = child.getProperty("type").toString();
                if (typeStr == "midi") info.type = TrackInfo::Type::Midi;
                else if (typeStr == "vocal") info.type = TrackInfo::Type::Vocal;
                else if (typeStr == "instrument") info.type = TrackInfo::Type::Instrument;
                else if (typeStr == "folder") info.type = TrackInfo::Type::Folder;
                else if (typeStr == "arranger") info.type = TrackInfo::Type::Arranger;
                else info.type = TrackInfo::Type::Audio;
                
                info.expanded = child.getProperty("expanded", true);
                info.depth = depth;
                info.visible = parentVisible;
                tracks.push_back(info);
                
                // Recursively parse children of this track (if any)
                parseNode(child, depth + 1, parentVisible && info.expanded);
            }
        }
    };
    
    parseNode(valueTree, 0, true);

    markClean();
    sendChangeMessage();
    return true;
}

