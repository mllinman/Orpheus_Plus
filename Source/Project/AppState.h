#pragma once
#include <JuceHeader.h>

//==============================================================================
// Central state store for the project — tracks undo history and dirty flag
class AppState : public juce::ChangeBroadcaster
{
public:
    AppState();
    ~AppState() override;

    //── Project state ────────────────────────────────────────────────────────
    bool isDirty() const { return dirty; }
    void markDirty()     { dirty = true; sendChangeMessage(); }
    void markClean()     { dirty = false; }

    juce::String getProjectName() const        { return projectName; }
    void setProjectName(const juce::String& n) { projectName = n; markDirty(); }

    double getBpm() const            { return bpm; }
    void   setBpm(double b)          { bpm = b; markDirty(); }

    int  getTimeSigNum() const       { return timeSigNum; }
    void setTimeSigNum(int n)        { timeSigNum = n; markDirty(); }
    int  getTimeSigDen() const       { return timeSigDen; }
    void setTimeSigDen(int d)        { timeSigDen = d; markDirty(); }

    int  getSampleRate() const       { return sampleRate; }
    void setSampleRate(int sr)       { sampleRate = sr; markDirty(); }

    //── Track management ─────────────────────────────────────────────────────
    int  addAudioTrack(const juce::String& name = {});
    int  addMidiTrack(const juce::String& name = {});
    void removeTrack(int index);
    int  getNumTracks() const;

    //── Undo / Redo ──────────────────────────────────────────────────────────
    void undo();
    void redo();
    bool canUndo() const { return undoManager.canUndo(); }
    bool canRedo() const { return undoManager.canRedo(); }

    juce::UndoManager& getUndoManager() { return undoManager; }

    //── Serialisation ────────────────────────────────────────────────────────
    std::unique_ptr<juce::XmlElement> toXml() const;
    bool fromXml(const juce::XmlElement& xml);

    juce::ValueTree getValueTree() { return valueTree; }

private:
    bool         dirty       = false;
    juce::String projectName = "Untitled Project";
    double       bpm         = 120.0;
    int          timeSigNum  = 4;
    int          timeSigDen  = 4;
    int          sampleRate  = 48000;

    juce::UndoManager undoManager { 100 };
    juce::ValueTree   valueTree   { "OrpheusProject" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AppState)
};
