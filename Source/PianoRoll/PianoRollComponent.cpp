#include "PianoRollComponent.h"
#include "../UI/OrpheusLookAndFeel.h"
#include "../Audio/ChordGeneratorProcessor.h"

PianoRollComponent::PianoRollComponent(AppState& s, AudioEngine& e)
    : appState(s), audioEngine(e)
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
    
    liveNoteState.fill(false);
    liveNoteVelocity.fill(0);

    // Enable MIDI input from all available devices
    enableMidiInput();

    // Setup Toolbar
    addAndMakeVisible(toolBar);
    
    toolBar.addAndMakeVisible(btnAIChords);
    btnAIChords.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff533483));
    btnAIChords.onClick = [this] { generateAIChords(); };

    toolBar.addAndMakeVisible(btnAIMelody);
    btnAIMelody.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00BCD4));
    btnAIMelody.onClick = [this] { generateAIMelody(); };

    toolBar.addAndMakeVisible(btnArpeggiate);
    btnArpeggiate.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d44));
    btnArpeggiate.onClick = [this] { arpeggiate(); };

    toolBar.addAndMakeVisible(btnHumanize);
    btnHumanize.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d44));
    btnHumanize.onClick = [this] { humanize(); };

    horizontalLayout.setItemLayout(0, 30, 200, keyboardWidth); // Keyboard keys (fixed area)
    horizontalLayout.setItemLayout(1, 8, 8, 8);               // Resizer (fixed 8px)
    horizontalLayout.setItemLayout(2, -0.1, -1.0, -0.7);      // Note Grid (flexible)

    resizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(&horizontalLayout, 1, false);
    addAndMakeVisible(resizerBar.get());

    startTimerHz(30);
}

PianoRollComponent::~PianoRollComponent()
{
    stopTimer();
    // Disable MIDI input on all devices
    auto& dm = audioEngine.getDeviceManager();
    for (auto& dev : juce::MidiInput::getAvailableDevices())
        dm.removeMidiInputDeviceCallback(dev.identifier, this);
}

void PianoRollComponent::enableMidiInput()
{
    auto& dm = audioEngine.getDeviceManager();
    auto devices = juce::MidiInput::getAvailableDevices();

    for (auto& dev : devices)
    {
        if (!dm.isMidiInputDeviceEnabled(dev.identifier))
            dm.setMidiInputDeviceEnabled(dev.identifier, true);

        dm.addMidiInputDeviceCallback(dev.identifier, this);
    }
}

//==============================================================================
// MIDI Input Callback — called on the MIDI thread
//==============================================================================
void PianoRollComponent::handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message)
{
    if (message.isNoteOn())
    {
        const juce::ScopedLock sl(midiStateLock);
        int note = message.getNoteNumber();
        if (note >= 0 && note < 128)
        {
            liveNoteState[note] = true;
            liveNoteVelocity[note] = message.getVelocity();
        }
    }
    else if (message.isNoteOff())
    {
        const juce::ScopedLock sl(midiStateLock);
        int note = message.getNoteNumber();
        if (note >= 0 && note < 128)
        {
            liveNoteState[note] = false;
            liveNoteVelocity[note] = 0;
        }
    }
}

void PianoRollComponent::resized()
{
    auto bounds = getLocalBounds();
    
    toolBar.setBounds(bounds.removeFromTop(40));
    
    auto toolBarBounds = toolBar.getLocalBounds().reduced(4);
    btnAIChords.setBounds(toolBarBounds.removeFromLeft(100).reduced(2));
    btnAIMelody.setBounds(toolBarBounds.removeFromLeft(100).reduced(2));
    btnArpeggiate.setBounds(toolBarBounds.removeFromLeft(100).reduced(2));
    btnHumanize.setBounds(toolBarBounds.removeFromLeft(100).reduced(2));

    juce::Component dummyLeft, dummyRight;
    juce::Component* hComps[] = { &dummyLeft, resizerBar.get(), &dummyRight };
    
    horizontalLayout.layOutComponents(hComps, 3, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false, true);
    
    // We don't actually move any child components because we paint them directly,
    // but we need to update the dynamic `keyboardWidth` property based on the resizer's calculation.
    keyboardWidth = dummyLeft.getWidth();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(OrpheusLookAndFeel::bgPanel());

    auto pianoArea = bounds.removeFromLeft(keyboardWidth);
    
    // Account for resizer bar width in visual painting
    bounds.removeFromLeft(8);

    auto noteArea = bounds;
    noteArea.removeFromBottom(VELOCITY_LANE_HEIGHT);

    paintPianoKeys(g, pianoArea);
    paintGrid(g, noteArea);
    paintNotes(g, noteArea);

    auto velArea = bounds.removeFromBottom(VELOCITY_LANE_HEIGHT);
    paintVelocityLane(g, velArea);

    paintPlayhead(g);

    if (isLassoDragging)
    {
        g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f));
        g.fillRect(selectionLasso);
        g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.8f));
        g.drawRect(selectionLasso, 1);
    }
}

void PianoRollComponent::paintPianoKeys(juce::Graphics& g, juce::Rectangle<int> area)
{
    static const bool blackKeys[12] = { false,true,false,true,false,false,true,false,true,false,true,false };

    // Snapshot the live state (thread-safe copy)
    std::array<bool, 128> activeKeys;
    std::array<uint8_t, 128> activeVel;
    {
        const juce::ScopedLock sl(midiStateLock);
        activeKeys = liveNoteState;
        activeVel  = liveNoteVelocity;
    }

    for (int note = 0; note < NUM_NOTES; ++note)
    {
        int   y   = pitchToPixel(note) - (int)verticalOffset;
        bool  isBlack = blackKeys[note % 12];
        float keyW = isBlack ? area.getWidth() * 0.6f : (float)area.getWidth();

        bool isActive = activeKeys[note];

        if (isActive)
        {
            // Glowing highlight when key is pressed
            float intensity = activeVel[note] / 127.0f;
            juce::Colour glow = OrpheusLookAndFeel::accentPrimary().interpolatedWith(
                OrpheusLookAndFeel::accentSecondary(), 0.3f);
            g.setColour(glow.withAlpha(0.6f + intensity * 0.4f));
        }
        else
        {
            g.setColour(isBlack ? juce::Colour(0xff222233) : juce::Colour(0xffddeeff));
        }
        g.fillRect(area.getX(), y, (int)keyW, NOTE_HEIGHT);

        // Key border
        g.setColour(juce::Colour(0xff000000).withAlpha(isActive ? 0.2f : 1.0f));
        g.drawRect(area.getX(), y, (int)keyW, NOTE_HEIGHT);

        // Glow outline on active keys
        if (isActive)
        {
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.7f));
            g.drawRect(area.getX(), y, (int)keyW, NOTE_HEIGHT, 2);
        }

        // Note name at C notes
        if (note % 12 == 0)
        {
            g.setColour(isActive ? juce::Colours::white : juce::Colours::grey);
            g.setFont(juce::Font(9.0f));
            g.drawText("C" + juce::String(note / 12 - 1),
                       area.getX() + 2, y, area.getWidth() - 4, NOTE_HEIGHT,
                       juce::Justification::centredLeft);
        }
    }
}

void PianoRollComponent::paintGrid(juce::Graphics& g, juce::Rectangle<int> area)
{
    double bpm           = audioEngine.getBpm();
    double secondsPerBeat = 60.0 / bpm;
    int    totalBeats    = 64;

    // Snapshot active keys for lane highlighting
    std::array<bool, 128> activeKeys;
    {
        const juce::ScopedLock sl(midiStateLock);
        activeKeys = liveNoteState;
    }

    // Horizontal lane lines
    for (int note = 0; note < NUM_NOTES; ++note)
    {
        int y = area.getY() + pitchToPixel(note) - (int)verticalOffset;
        bool isBlack = (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 ||
                        note % 12 == 8 || note % 12 == 10);

        if (activeKeys[note])
        {
            // Highlight the entire lane when key is pressed
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.12f));
        }
        else
        {
            g.setColour(isBlack ? juce::Colour(0xff0f0f1f) : OrpheusLookAndFeel::bgSurface());
        }
        g.fillRect(area.getX(), y, area.getWidth(), NOTE_HEIGHT);

        if (note % 12 == 0)
        {
            g.setColour(OrpheusLookAndFeel::borderDefault());
            g.drawHorizontalLine(y + NOTE_HEIGHT, (float)area.getX(), (float)area.getRight());
        }
    }

    // Vertical beat lines
    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        int x = area.getX() + beatToPixel(beat) - (int)horizontalOffset;
        if (x < area.getX() || x > area.getRight()) continue;

        bool isBar = (beat % 4 == 0);
        g.setColour(isBar ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.35f) 
                          : OrpheusLookAndFeel::borderDefault());
        g.drawVerticalLine(x, (float)area.getY(), (float)area.getBottom());
    }

    // Quantize grid lines
    double subDiv = quantizeDivision;
    for (double beat = 0; beat <= totalBeats; beat += subDiv)
    {
        int x = area.getX() + beatToPixel(beat) - (int)horizontalOffset;
        if (x < area.getX() || x > area.getRight()) continue;
        g.setColour(juce::Colour(0x22ffffff));
        g.drawVerticalLine(x, (float)area.getY(), (float)area.getBottom());
    }
}

void PianoRollComponent::paintNotes(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Define an array of distinct colors for multi-track rendering
    const juce::Colour trackColours[] = {
        juce::Colour(0xff6c5ce7), // Legacy accent purple
        juce::Colour(0xff00b894), // Green
        juce::Colour(0xff0984e3), // Blue
        juce::Colour(0xfffdcb6e), // Yellow/Orange
        juce::Colour(0xffe17055), // Red/Orange
        juce::Colour(0xffd63031)  // Red
    };

    auto drawNotes = [&](const std::vector<MidiNote*>& notesList, juce::Colour baseColour) {
        for (auto* note : notesList)
        {
            auto bounds = getNoteBounds(*note);
            bounds.translate((float)area.getX(), (float)area.getY());
            bounds.translate(0.0f, -(float)verticalOffset);
            bounds.translate(-(float)horizontalOffset, 0.0f);

            if (bounds.getRight() < area.getX() || bounds.getX() > area.getRight()) continue;

            juce::Colour col = note->selected ? baseColour.brighter(0.4f) : baseColour;
            g.setColour(col.withAlpha(0.85f));
            g.fillRoundedRectangle(bounds, 2.0f);

            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        }
    };

    if (!activeClips.empty())
    {
        // Render unified key editor (multiple clips)
        for (size_t i = 0; i < activeClips.size(); ++i)
        {
            auto* clip = activeClips[i];
            juce::Colour col = trackColours[i % 6];
            
            std::vector<MidiNote*> clipNotes;
            for (auto* n : clip->notes) clipNotes.push_back(n);
            
            drawNotes(clipNotes, col);
        }
    }
    else
    {
        // Fallback to local notes array
        std::vector<MidiNote*> localNotes;
        for (auto* n : notes) localNotes.push_back(n);
        drawNotes(localNotes, noteColour);
    }
    
    // Create clip bounds to prevent notes drawing outside grid
    g.reduceClipRegion(area);
}

void PianoRollComponent::paintVelocityLane(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xff151525));
    g.fillRect(area);

    g.setColour(OrpheusLookAndFeel::borderDefault());
    g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());
    
    // Draw stems for all notes
    for (auto* note : notes)
    {
        auto bounds = getNoteBounds(*note);
        int centerX = bounds.getX() + bounds.getWidth() / 2;
        int x = area.getX() + centerX - (int)horizontalOffset;

        if (x < area.getX() || x > area.getRight()) continue;

        int stemH = (int)(area.getHeight() * (note->velocity / 127.0f));
        int stemY = area.getBottom() - stemH;

        juce::Colour col = note->selected ? noteColour.brighter(0.4f) : noteColour;
        g.setColour(col.withAlpha(0.8f));
        
        g.drawVerticalLine(x, (float)stemY, (float)area.getBottom());
        g.fillEllipse((float)x - 2.5f, (float)stemY - 2.5f, 5.0f, 5.0f);
    }
}

void PianoRollComponent::paintPlayhead(juce::Graphics& g)
{
    double beats = audioEngine.getPlayheadPosition() *
                   (audioEngine.getBpm() / 60.0);
    int x = keyboardWidth + 8 + beatToPixel(beats) - (int)horizontalOffset;

    // Playhead with glow
    g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f));
    g.drawVerticalLine(x - 1, 0.0f, (float)getHeight());
    g.drawVerticalLine(x + 1, 0.0f, (float)getHeight());
    g.setColour(OrpheusLookAndFeel::accentPrimary());
    g.drawVerticalLine(x, 0.0f, (float)getHeight());
}

void PianoRollComponent::timerCallback()
{
    // Always repaint to update live key highlights and playhead
    repaint();
}

//──────────────────────────────────────────────────────────────────────────────
// Mouse & Keyboard interaction
//──────────────────────────────────────────────────────────────────────────────
void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.x < keyboardWidth + 8) return;
    grabKeyboardFocus();

    int pitch = pixelToPitch(e.y + (int)verticalOffset);
    double beat = pixelToBeat(e.x - (keyboardWidth + 8) + (int)horizontalOffset);
    double qBeat = std::round(beat / quantizeDivision) * quantizeDivision;

    if (e.y >= getHeight() - VELOCITY_LANE_HEIGHT)
    {
        isEditingVelocity = true;
        mouseDrag(e);
        return;
    }

    auto* n = getNoteAt(beat, pitch);

    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        if (n) {
            notes.removeObject(n);
            repaint();
        }
    }
    else if (n != nullptr)
    {
        if (!e.mods.isShiftDown() && !n->selected)
            for (auto* other : notes) other->selected = false;
            
        n->selected = true;

        auto bounds = getNoteBounds(*n);
        float rightEdge = bounds.getRight() + keyboardWidth + 8 - (float)horizontalOffset;
        
        if (std::abs(e.x - rightEdge) < 10.0f) {
            resizingNote = n;
        } else {
            draggingNotes.clear();
            for (auto* note : notes) {
                if (note->selected) {
                    draggingNotes.push_back({note, note->startBeat, note->pitch});
                }
            }
            dragStartBeat = beat;
            dragStartPitch = pitch;
        }
        repaint();
    }
    else
    {
        if (!e.mods.isShiftDown())
            for (auto* other : notes) other->selected = false;

        if (!isDrawingMode || e.mods.isAltDown() || e.mods.isShiftDown())
        {
            isLassoDragging = true;
            selectionLasso.setPosition(e.getPosition());
            selectionLasso.setSize(0, 0);
        }
        else
        {
            auto* newNote       = notes.add(new MidiNote());
            newNote->pitch      = pitch;
            newNote->startBeat  = qBeat;
            newNote->duration   = quantizeDivision;
            newNote->velocity   = 100;
            newNote->selected   = true;
            
            draggingNotes.clear();
            draggingNotes.push_back({newNote, newNote->startBeat, newNote->pitch});
            dragStartBeat = beat;
            dragStartPitch = pitch;
        }
        repaint();
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.x < keyboardWidth + 8) return;

    if (isEditingVelocity)
    {
        double wBeat = pixelToBeat(e.x - (keyboardWidth + 8) + (int)horizontalOffset);
        float normalizedY = 1.0f - (float)(e.y - (getHeight() - VELOCITY_LANE_HEIGHT)) / VELOCITY_LANE_HEIGHT;
        int vel = juce::jlimit(0, 127, (int)(normalizedY * 127.0f));
        
        for (auto* note : notes) {
            if (wBeat >= note->startBeat && wBeat <= note->startBeat + note->duration) {
                note->velocity = vel;
            }
        }
        repaint();
        return;
    }

    int pitch = pixelToPitch(e.y + (int)verticalOffset);
    double beat = pixelToBeat(e.x - (keyboardWidth + 8) + (int)horizontalOffset);
    double qBeat = std::round(beat / quantizeDivision) * quantizeDivision;

    if (isLassoDragging)
    {
        selectionLasso = juce::Rectangle<int>(e.getMouseDownPosition(), e.getPosition());
        
        juce::Rectangle<float> lassoF((float)(selectionLasso.getX() - (keyboardWidth + 8) + horizontalOffset),
                                      (float)(selectionLasso.getY() + verticalOffset),
                                      (float)selectionLasso.getWidth(),
                                      (float)selectionLasso.getHeight());

        for (auto* note : notes) {
            auto bounds = getNoteBounds(*note);
            note->selected = bounds.intersects(lassoF);
        }
        repaint();
        return;
    }

    if (resizingNote)
    {
        double newDuration = qBeat - resizingNote->startBeat + quantizeDivision;
        resizingNote->duration = juce::jmax(quantizeDivision, newDuration);
        repaint();
    }
    else if (!draggingNotes.empty())
    {
        double beatDelta = qBeat - std::round(dragStartBeat / quantizeDivision) * quantizeDivision;
        int pitchDelta = pitch - dragStartPitch;

        for (auto& state : draggingNotes)
        {
            state.note->pitch = juce::jlimit(0, 127, state.originalPitch + pitchDelta);
            state.note->startBeat = juce::jmax(0.0, state.originalBeat + beatDelta);
        }
        repaint();
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&)
{
    if (!draggingNotes.empty() || resizingNote || isEditingVelocity)
        syncToClips();

    draggingNotes.clear();
    resizingNote = nullptr;
    isLassoDragging = false;
    isEditingVelocity = false;
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelected();
        syncToClips();
        return true;
    }
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectAll();
        return true;
    }
    if (key == juce::KeyPress('q', juce::ModifierKeys::commandModifier, 0))
    {
        quantizeSelected();
        syncToClips();
        return true;
    }

    return false;
}


void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& e,
                                         const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
        pixelsPerBeat = juce::jlimit(20.0, 400.0, pixelsPerBeat * (1.0 + wheel.deltaY * 0.3));
    else if (e.mods.isShiftDown())
        horizontalOffset = juce::jmax(0.0, horizontalOffset - wheel.deltaY * 50.0);
    else
        verticalOffset = juce::jlimit(0.0, (double)(NUM_NOTES * NOTE_HEIGHT - getHeight()),
                                      verticalOffset - wheel.deltaY * 30.0);
    repaint();
}

//──────────────────────────────────────────────────────────────────────────────
// Helpers
//──────────────────────────────────────────────────────────────────────────────
MidiNote* PianoRollComponent::getNoteAt(double beat, int pitch)
{
    for (auto* n : notes)
        if (n->pitch == pitch && beat >= n->startBeat && beat < n->startBeat + n->duration)
            return n;
    return nullptr;
}

juce::Rectangle<float> PianoRollComponent::getNoteBounds(const MidiNote& note) const
{
    float x = (float)beatToPixel(note.startBeat);
    float y = (float)pitchToPixel(note.pitch);
    float w = (float)(note.duration * pixelsPerBeat);
    return { x, y, juce::jmax(4.0f, w - 1.0f), (float)(NOTE_HEIGHT - 1) };
}

int PianoRollComponent::pixelToPitch(int y) const
{
    return juce::jlimit(0, 127, NUM_NOTES - 1 - y / NOTE_HEIGHT);
}

double PianoRollComponent::pixelToBeat(int x) const
{
    return x / pixelsPerBeat;
}

int PianoRollComponent::pitchToPixel(int pitch) const
{
    return (NUM_NOTES - 1 - pitch) * NOTE_HEIGHT;
}

int PianoRollComponent::beatToPixel(double beat) const
{
    return (int)(beat * pixelsPerBeat);
}

void PianoRollComponent::selectAll()
{
    for (auto* n : notes) n->selected = true;
    repaint();
}

void PianoRollComponent::deleteSelected()
{
    for (int i = notes.size() - 1; i >= 0; --i)
        if (notes[i]->selected) notes.remove(i);
    repaint();
}

void PianoRollComponent::quantizeSelected()
{
    for (auto* n : notes)
        if (n->selected)
            n->startBeat = std::round(n->startBeat / quantizeDivision) * quantizeDivision;
    repaint();
}

void PianoRollComponent::loadMidiSequence(const juce::MidiMessageSequence& seq)
{
    notes.clear();
    double bpm = audioEngine.getBpm();

    for (int i = 0; i < seq.getNumEvents(); ++i)
    {
        auto* e = seq.getEventPointer(i);
        if (e->message.isNoteOn())
        {
            auto* n = notes.add(new MidiNote());
            n->pitch     = e->message.getNoteNumber();
            n->velocity  = e->message.getVelocity();
            n->startBeat = e->message.getTimeStamp() * (bpm / 60.0);

            // Find corresponding note-off
            if (e->noteOffObject)
                n->duration = (e->noteOffObject->message.getTimeStamp() -
                               e->message.getTimeStamp()) * (bpm / 60.0);
        }
    }
    repaint();
}

void PianoRollComponent::setActiveClips(const std::vector<MidiClip*>& clips)
{
    activeClips = clips;
    notes.clear();
    
    if (!activeClips.empty())
    {
        // For simplicity, just load the first clip's data for editing
        // A true unified editor would load all, differentiated by color
        loadMidiSequence(activeClips[0]->midiData);
    }
}

void PianoRollComponent::syncToClips()
{
    if (!activeClips.empty())
        activeClips[0]->midiData = getMidiSequence();
}

juce::MidiMessageSequence PianoRollComponent::getMidiSequence() const
{
    juce::MidiMessageSequence seq;
    double bpm = audioEngine.getBpm();

    for (auto* n : notes)
    {
        double tOn  = n->startBeat / (bpm / 60.0);
        double tOff = (n->startBeat + n->duration) / (bpm / 60.0);

        seq.addEvent(juce::MidiMessage::noteOn(1, n->pitch, (uint8_t)n->velocity), tOn);
        seq.addEvent(juce::MidiMessage::noteOff(1, n->pitch), tOff);
    }
    seq.updateMatchedPairs();
    return seq;
}

//==============================================================================
// AI Generative Tools
//==============================================================================

void PianoRollComponent::generateAIChords()
{
    notes.clear();
    
    // Generate I-VI-IV-V progression in C Major
    auto generated = ChordGeneratorProcessor::generateChords(60, ChordGeneratorProcessor::ScaleType::Major, 4, 0.0, 4.0);
    
    for (const auto& gn : generated)
    {
        auto* n = notes.add(new MidiNote());
        n->pitch = gn.pitch;
        n->velocity = gn.velocity;
        n->startBeat = gn.beat;
        n->duration = gn.duration;
    }

    syncToClips();
    repaint();
}

void PianoRollComponent::generateAIMelody()
{
    notes.clear();

    // Generate 4 bars of melody in C Major
    auto generated = ChordGeneratorProcessor::generateMelody(60, ChordGeneratorProcessor::ScaleType::Major, 0.0, 16.0);
    
    for (const auto& gn : generated)
    {
        auto* n = notes.add(new MidiNote());
        n->pitch = gn.pitch;
        n->velocity = gn.velocity;
        n->startBeat = gn.beat;
        n->duration = gn.duration;
    }

    syncToClips();
    repaint();
}

void PianoRollComponent::arpeggiate()
{
    if (notes.isEmpty()) return;

    struct NoteSorter {
        int compareElements(const MidiNote* a, const MidiNote* b) const {
            if (std::abs(a->startBeat - b->startBeat) > 0.01)
                return a->startBeat < b->startBeat ? -1 : 1;
            return a->pitch < b->pitch ? -1 : 1;
        }
    };
    NoteSorter sorter;
    notes.sort(sorter);

    double currentChordBeat = notes.getFirst()->startBeat;
    double offset = 0.0;

    for (auto* n : notes)
    {
        if (std::abs(n->startBeat - currentChordBeat) > 0.1) // New chord group
        {
            currentChordBeat = n->startBeat;
            offset = 0.0;
        }

        n->startBeat += offset;
        n->duration = 0.5; // Shorten to eighth note
        offset += 0.5;
    }

    syncToClips();
    repaint();
}

void PianoRollComponent::humanize()
{
    auto& rand = juce::Random::getSystemRandom();
    for (auto* n : notes)
    {
        // Randomize timing +/- 0.05 beats
        double shift = (rand.nextFloat() * 0.1) - 0.05;
        n->startBeat = juce::jmax(0.0, n->startBeat + shift);

        // Randomize velocity +/- 10
        int velShift = rand.nextInt(21) - 10;
        n->velocity = juce::jlimit(10, 127, n->velocity + velShift);
    }

    syncToClips();
    repaint();
}
