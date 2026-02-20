#include "PianoRollComponent.h"

PianoRollComponent::PianoRollComponent(AppState& s, AudioEngine& e)
    : appState(s), audioEngine(e)
{
    setOpaque(true);
    setWantsKeyboardFocus(true);
    startTimerHz(30);
}

PianoRollComponent::~PianoRollComponent() { stopTimer(); }

void PianoRollComponent::resized() {}

void PianoRollComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    g.fillAll(juce::Colour(0xff12121e));

    auto pianoArea = bounds.removeFromLeft(PIANO_KEY_WIDTH);
    paintPianoKeys(g, pianoArea);
    paintGrid(g, bounds);
    paintNotes(g, bounds);
    paintPlayhead(g);
}

void PianoRollComponent::paintPianoKeys(juce::Graphics& g, juce::Rectangle<int> area)
{
    static const bool blackKeys[12] = { false,true,false,true,false,false,true,false,true,false,true,false };

    for (int note = 0; note < NUM_NOTES; ++note)
    {
        int   y   = pitchToPixel(note) - (int)verticalOffset;
        bool  isBlack = blackKeys[note % 12];
        float keyW = isBlack ? area.getWidth() * 0.6f : (float)area.getWidth();

        g.setColour(isBlack ? juce::Colour(0xff222233) : juce::Colour(0xffddeeff));
        g.fillRect(area.getX(), y, (int)keyW, NOTE_HEIGHT);
        g.setColour(juce::Colour(0xff000000));
        g.drawRect(area.getX(), y, (int)keyW, NOTE_HEIGHT);

        // Note name at C notes
        if (note % 12 == 0)
        {
            g.setColour(juce::Colours::grey);
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

    // Horizontal lane lines
    for (int note = 0; note < NUM_NOTES; ++note)
    {
        int y = area.getY() + pitchToPixel(note) - (int)verticalOffset;
        bool isBlack = (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 ||
                        note % 12 == 8 || note % 12 == 10);

        g.setColour(isBlack ? juce::Colour(0xff0f0f1f) : juce::Colour(0xff1a1a2e));
        g.fillRect(area.getX(), y, area.getWidth(), NOTE_HEIGHT);

        if (note % 12 == 0)
        {
            g.setColour(juce::Colour(0xff2a2a4a));
            g.drawHorizontalLine(y + NOTE_HEIGHT, (float)area.getX(), (float)area.getRight());
        }
    }

    // Vertical beat lines
    for (int beat = 0; beat <= totalBeats; ++beat)
    {
        int x = area.getX() + beatToPixel(beat) - (int)horizontalOffset;
        if (x < area.getX() || x > area.getRight()) continue;

        bool isBar = (beat % 4 == 0);
        g.setColour(isBar ? juce::Colour(0xff533483) : juce::Colour(0xff2a2a4a));
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
    for (auto* note : notes)
    {
        auto bounds = getNoteBounds(*note);
        bounds.translate((float)area.getX(), (float)area.getY());
        bounds.translate(0.0f, -(float)verticalOffset);
        bounds.translate(-(float)horizontalOffset, 0.0f);

        if (bounds.getRight() < area.getX() || bounds.getX() > area.getRight()) continue;

        juce::Colour col = note->selected ? noteColour.brighter(0.4f) : noteColour;
        g.setColour(col.withAlpha(0.85f));
        g.fillRoundedRectangle(bounds, 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);

        // Velocity indicator bar at bottom
        float velW = bounds.getWidth() * (note->velocity / 127.0f);
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.fillRect(bounds.getX(), bounds.getBottom() - 2.0f, velW, 2.0f);
    }
}

void PianoRollComponent::paintPlayhead(juce::Graphics& g)
{
    double beats = audioEngine.getPlayheadPosition() *
                   (audioEngine.getBpm() / 60.0);
    int x = PIANO_KEY_WIDTH + beatToPixel(beats) - (int)horizontalOffset;

    g.setColour(juce::Colour(0xffe94560));
    g.drawVerticalLine(x, 0.0f, (float)getHeight());
}

void PianoRollComponent::timerCallback()
{
    if (audioEngine.isPlaying()) repaint();
}

//──────────────────────────────────────────────────────────────────────────────
// Mouse & Keyboard interaction
//──────────────────────────────────────────────────────────────────────────────
void PianoRollComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.x < PIANO_KEY_WIDTH) return;
    grabKeyboardFocus();

    int pitch = pixelToPitch(e.y + (int)verticalOffset);
    double beat = pixelToBeat(e.x - PIANO_KEY_WIDTH + (int)horizontalOffset);
    double qBeat = std::round(beat / quantizeDivision) * quantizeDivision;

    auto* n = getNoteAt(beat, pitch);

    if (e.mods.isRightButtonDown() || e.mods.isCommandDown())
    {
        // Delete note under cursor
        if (n) {
            notes.removeObject(n);
            repaint();
        }
    }
    else if (n != nullptr)
    {
        // Clicked an existing note
        if (!e.mods.isShiftDown())
            for (auto* other : notes) other->selected = false;
            
        n->selected = true;

        // Check if we clicked the right edge for resizing
        auto bounds = getNoteBounds(*n);
        float rightEdge = bounds.getRight() + PIANO_KEY_WIDTH - (float)horizontalOffset;
        
        if (std::abs(e.x - rightEdge) < 10.0f) {
            resizingNote = n;
        } else {
            draggingNote = n;
        }
        repaint();
    }
    else
    {
        // Deselect all
        for (auto* other : notes) other->selected = false;

        // Add new note if double clicked or if we enforce drawing mode
        // For now, let's say single click on empty space creates a note
        auto* newNote       = notes.add(new MidiNote());
        newNote->pitch      = pitch;
        newNote->startBeat  = qBeat;
        newNote->duration   = quantizeDivision;
        newNote->velocity   = 100;
        newNote->selected   = true;
        draggingNote        = newNote;
        repaint();
    }
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (e.x < PIANO_KEY_WIDTH) return;

    int pitch = pixelToPitch(e.y + (int)verticalOffset);
    double beat = pixelToBeat(e.x - PIANO_KEY_WIDTH + (int)horizontalOffset);
    double qBeat = std::round(beat / quantizeDivision) * quantizeDivision;

    if (resizingNote)
    {
        double newDuration = qBeat - resizingNote->startBeat + quantizeDivision;
        resizingNote->duration = juce::jmax(quantizeDivision, newDuration);
        repaint();
    }
    else if (draggingNote)
    {
        // Simple move (doesn't handle multi-selection move yet)
        draggingNote->pitch = pitch;
        draggingNote->startBeat = qBeat;
        repaint();
    }
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&)
{
    if (draggingNote || resizingNote)
        syncToClip();

    draggingNote = nullptr;
    resizingNote = nullptr;
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelected();
        syncToClip();
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
        syncToClip();
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

void PianoRollComponent::setActiveClip(MidiClip* clip)
{
    activeClip = clip;
    if (activeClip)
        loadMidiSequence(activeClip->midiData);
    else
        notes.clear();
}

void PianoRollComponent::syncToClip()
{
    if (activeClip)
        activeClip->midiData = getMidiSequence();
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
