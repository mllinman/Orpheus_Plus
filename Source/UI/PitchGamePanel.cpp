#include "PitchGamePanel.h"

// ─── Note name tables ────────────────────────────────────────────────────────
static const char* noteNamesSharp[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
static const char* scaleNames[] = { "Chromatic", "Major", "Natural Minor", "Harmonic Minor", "Pentatonic Maj", "Pentatonic Min", "Blues", "Dorian", "Mixolydian" };

// ─── Scale intervals ─────────────────────────────────────────────────────────
static const std::vector<int> scaleIntervals[] = {
    { 0,1,2,3,4,5,6,7,8,9,10,11 },  // 0: Chromatic
    { 0,2,4,5,7,9,11 },              // 1: Major (Ionian)
    { 0,2,3,5,7,8,10 },              // 2: Natural Minor (Aeolian)
    { 0,2,3,5,7,8,11 },              // 3: Harmonic Minor
    { 0,2,4,7,9 },                   // 4: Pentatonic Major
    { 0,3,5,7,10 },                  // 5: Pentatonic Minor
    { 0,3,5,6,7,10 },                // 6: Blues
    { 0,2,3,5,7,9,10 },              // 7: Dorian
    { 0,2,4,5,7,9,10 },              // 8: Mixolydian
};
static constexpr int NUM_SCALES = 9;

// Scale degree roman numerals for display
static const char* romanNumerals[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII" };

// =============================================================================
PitchGamePanel::PitchGamePanel(AudioEngine& engine)
    : audioEngine(engine)
{
    pitchHistory.fill({});

    // ─── Key selector ─────────────────────────────────────────────────────
    keyLabel.setFont(juce::Font(10.0f).boldened());
    keyLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
    keyLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(keyLabel);

    for (int i = 0; i < 12; ++i)
        keyCombo.addItem(noteNamesSharp[i], i + 1);
    keyCombo.setSelectedId(1, juce::dontSendNotification); // C
    keyCombo.onChange = [this] {
        currentKey = keyCombo.getSelectedId() - 1;
        buildScaleNotes();
        // Sync to the AutoTune processor if available
        if (processor != nullptr) processor->setKey(currentKey);
        repaint();
    };
    addAndMakeVisible(keyCombo);

    // ─── Scale selector ───────────────────────────────────────────────────
    scaleLabel.setFont(juce::Font(10.0f).boldened());
    scaleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
    scaleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(scaleLabel);

    for (int i = 0; i < NUM_SCALES; ++i)
        scaleCombo.addItem(scaleNames[i], i + 1);
    scaleCombo.setSelectedId(2, juce::dontSendNotification); // Major
    scaleCombo.onChange = [this] {
        currentScale = scaleCombo.getSelectedId() - 1;
        buildScaleNotes();
        if (processor != nullptr) processor->setScale(currentScale);
        repaint();
    };
    addAndMakeVisible(scaleCombo);

    buildScaleNotes();
    startTimerHz(60);
}

PitchGamePanel::~PitchGamePanel()
{
    stopTimer();
}

// =============================================================================
// Build the set of valid scale notes for the current key
// =============================================================================
void PitchGamePanel::buildScaleNotes()
{
    scaleNoteList.clear();
    int scaleIdx = juce::jlimit(0, NUM_SCALES - 1, currentScale);
    const auto& intervals = scaleIntervals[scaleIdx];
    for (int interval : intervals)
        scaleNoteList.push_back((currentKey + interval) % 12);
}

// =============================================================================
// Utility Functions
// =============================================================================
juce::String PitchGamePanel::midiNoteToName(int midiNote)
{
    if (midiNote < 0) return "-";
    int octave = (midiNote / 12) - 1;
    int note = midiNote % 12;
    return juce::String(noteNamesSharp[note]) + juce::String(octave);
}

juce::String PitchGamePanel::noteNameOnly(int noteInOctave)
{
    return juce::String(noteNamesSharp[noteInOctave % 12]);
}

float PitchGamePanel::hzToMidiNote(float hz)
{
    if (hz <= 0.0f) return -1.0f;
    return 69.0f + 12.0f * std::log2(hz / 440.0f);
}

bool PitchGamePanel::isNoteInScale(int noteInOctave) const
{
    for (int n : scaleNoteList)
        if (n == (noteInOctave % 12))
            return true;
    return false;
}

int PitchGamePanel::getScaleDegree(int noteInOctave) const
{
    int relative = ((noteInOctave - currentKey) % 12 + 12) % 12;
    int scaleIdx = juce::jlimit(0, NUM_SCALES - 1, currentScale);
    const auto& intervals = scaleIntervals[scaleIdx];
    for (int i = 0; i < (int)intervals.size(); ++i)
        if (intervals[(size_t)i] == relative)
            return i;
    return -1;
}

float PitchGamePanel::getClosestTargetNote(float detectedMidi) const
{
    if (detectedMidi < 0.0f) return -1.0f;

    int noteBelow = (int)std::floor(detectedMidi);
    float bestDist = 999.0f;
    float bestNote = detectedMidi;

    for (int n = noteBelow - 2; n <= noteBelow + 2; ++n)
    {
        if (n < 0 || n > 127) continue;
        if (isNoteInScale(n % 12))
        {
            float dist = std::abs(detectedMidi - (float)n);
            if (dist < bestDist) { bestDist = dist; bestNote = (float)n; }
        }
    }
    return bestNote;
}

// =============================================================================
// Timer — poll pitch data, update history, compute scoring
// =============================================================================
void PitchGamePanel::timerCallback()
{
    if (!gameActive) { repaint(); return; }

    float detectedHz = 0.0f;
    if (processor != nullptr)
        detectedHz = processor->getDetectedPitch();

    float detectedMidi = hzToMidiNote(detectedHz);
    bool  hasVoice     = detectedMidi > 20.0f && detectedMidi < 100.0f;

    float targetMidi = hasVoice ? getClosestTargetNote(detectedMidi) : -1.0f;
    float centError  = hasVoice ? (detectedMidi - targetMidi) * 100.0f : 0.0f;

    float smoothK = 0.15f;
    if (hasVoice)
    {
        smoothedDetectedMidi += smoothK * (detectedMidi - smoothedDetectedMidi);
        smoothedCentError    += smoothK * (centError - smoothedCentError);
    }

    float absError = std::abs(centError);
    float accuracy = hasVoice ? juce::jmax(0.0f, 1.0f - absError / 50.0f) : 0.0f;
    smoothedAccuracy += 0.1f * (accuracy - smoothedAccuracy);

    // Session scoring
    if (hasVoice)
    {
        sessionTotalSamples++;
        sessionAccumAccuracy += accuracy;
        sessionScore = sessionAccumAccuracy / (float)sessionTotalSamples;

        // Streak: on-pitch if within 15 cents
        if (absError < 15.0f)
        {
            streakCount++;
            if (streakCount > bestStreak) bestStreak = streakCount;
        }
        else
        {
            streakCount = 0;
        }
    }

    // Record to ring buffer
    auto& sample       = pitchHistory[(size_t)historyWriteIndex];
    sample.detectedMidi = detectedMidi;
    sample.targetMidi   = targetMidi;
    sample.centError    = centError;
    sample.hasVoice     = hasVoice;
    historyWriteIndex   = (historyWriteIndex + 1) % HISTORY_SIZE;

    ledGlow += 0.08f * ((gameActive ? 1.0f : 0.0f) - ledGlow);
    indicatorGlow += 0.12f * ((hasVoice ? accuracy : 0.0f) - indicatorGlow);

    repaint();
}

// =============================================================================
// Paint
// =============================================================================
void PitchGamePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    paintBackground(g, bounds);

    auto header = bounds.removeFromTop(48);
    paintHeaderBar(g, header);

    if (!gameActive)
    {
        // Show key info and inactive message
        auto infoBar = bounds.removeFromTop(60);
        paintKeyInfoBar(g, infoBar);

        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(14.0f);
        g.drawText("Toggle the switch above to start pitch training",
                   bounds, juce::Justification::centred);
        return;
    }

    // Active layout
    auto keyInfoBar  = bounds.removeFromTop(60);
    auto pianoBar    = bounds.removeFromTop(50);
    auto scoreBar    = bounds.removeFromBottom(36);
    auto accuracyCol = bounds.removeFromRight(80);
    auto noteCol     = bounds.removeFromLeft(60);

    paintKeyInfoBar(g, keyInfoBar);
    paintScalePiano(g, pianoBar);
    paintNoteLabels(g, noteCol);
    paintPitchLadder(g, bounds);
    paintPitchIndicator(g, bounds);
    paintAccuracyMeter(g, accuracyCol);
    paintSessionScore(g, scoreBar);
}

// =============================================================================
// Background
// =============================================================================
void PitchGamePanel::paintBackground(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(OrpheusLookAndFeel::bgDarkest());
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x08ffffff), 0, 0,
        juce::Colours::transparentBlack, 0, (float)bounds.getHeight(), false));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
}

// =============================================================================
// Header Bar — Title, LED, Toggle Switch
// =============================================================================
void PitchGamePanel::paintHeaderBar(juce::Graphics& g, juce::Rectangle<int> header)
{
    // Gradient accent stripe
    g.setGradientFill(juce::ColourGradient(
        OrpheusLookAndFeel::accentPrimary().withAlpha(0.12f), 0, (float)header.getY(),
        juce::Colours::transparentBlack, 0, (float)header.getBottom(), false));
    g.fillRect(header);

    // Title
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText("  PITCH TRAINER", header.reduced(8, 0), juce::Justification::centredLeft);

    // ─── Switch ──────────────────────────────────────────────────────────
    int sw = 48, sh = 24;
    int sx = header.getRight() - sw - 16;
    int sy = header.getCentreY() - sh / 2;
    switchBounds = { sx, sy, sw, sh };

    float cornerR = (float)sh / 2.0f;
    auto trackCol = gameActive ? OrpheusLookAndFeel::accentSuccess().withAlpha(0.6f)
                               : OrpheusLookAndFeel::bgDark();
    g.setColour(trackCol);
    g.fillRoundedRectangle(switchBounds.toFloat(), cornerR);
    g.setColour(gameActive ? OrpheusLookAndFeel::accentSuccess().withAlpha(0.8f)
                           : OrpheusLookAndFeel::borderDefault());
    g.drawRoundedRectangle(switchBounds.toFloat().reduced(0.5f), cornerR, 1.0f);

    float thumbX = gameActive ? (float)(sx + sw - sh + 2) : (float)(sx + 2);
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX, (float)(sy + 2), (float)(sh - 4), (float)(sh - 4));

    // ─── LED ─────────────────────────────────────────────────────────────
    int ledX = sx - 26, ledY = header.getCentreY() - 6, ledD = 12;
    auto ledCol = gameActive ? OrpheusLookAndFeel::accentSuccess() : OrpheusLookAndFeel::bgDark();
    g.setColour(ledCol.withAlpha(0.3f * ledGlow));
    g.fillEllipse((float)(ledX - 4), (float)(ledY - 4), (float)(ledD + 8), (float)(ledD + 8));
    g.setColour(ledCol);
    g.fillEllipse((float)ledX, (float)ledY, (float)ledD, (float)ledD);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.fillEllipse((float)(ledX + 2), (float)(ledY + 2), (float)(ledD - 6), (float)(ledD - 6));

    // Separator
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(header.getBottom() - 1, (float)header.getX(), (float)header.getRight());
}

// =============================================================================
// Key Info Bar — shows current key, scale name, and all notes in the key
// =============================================================================
void PitchGamePanel::paintKeyInfoBar(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Dark strip background
    g.setColour(OrpheusLookAndFeel::bgDarker().withAlpha(0.7f));
    g.fillRect(area);

    auto left = area.removeFromLeft(180); // Key/Scale combos are overlaid here by resized()

    // ─── "KEY OF" display ─────────────────────────────────────────────────
    auto keyDisplay = area.removeFromLeft(120).reduced(4, 8);
    g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.15f));
    g.fillRoundedRectangle(keyDisplay.toFloat(), 8.0f);
    g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.5f));
    g.drawRoundedRectangle(keyDisplay.toFloat(), 8.0f, 1.0f);

    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("KEY OF", keyDisplay.removeFromTop(14), juce::Justification::centred);

    g.setColour(OrpheusLookAndFeel::accentPrimary());
    g.setFont(juce::Font(22.0f).boldened());
    juce::String keyName = noteNameOnly(currentKey);
    int scaleIdx = juce::jlimit(0, NUM_SCALES - 1, currentScale);
    juce::String shortScale = (scaleIdx == 1) ? "Major" : (scaleIdx == 2) ? "Minor" : juce::String(scaleNames[scaleIdx]);
    g.drawText(keyName + " " + shortScale, keyDisplay, juce::Justification::centred);

    // ─── Scale notes display ──────────────────────────────────────────────
    auto notesArea = area.reduced(8, 10);
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("SCALE NOTES", notesArea.removeFromTop(12), juce::Justification::centredLeft);

    float noteW = juce::jmin(32.0f, (float)notesArea.getWidth() / (float)scaleNoteList.size());
    float nx = (float)notesArea.getX();
    float ny = (float)notesArea.getY() + 2;

    for (size_t i = 0; i < scaleNoteList.size(); ++i)
    {
        int noteVal = scaleNoteList[i];
        bool isRoot = (noteVal == currentKey);

        // Pill background
        auto pill = juce::Rectangle<float>(nx, ny, noteW - 3, 22.0f);
        g.setColour(isRoot ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.25f)
                           : OrpheusLookAndFeel::bgDark());
        g.fillRoundedRectangle(pill, 4.0f);
        g.setColour(isRoot ? OrpheusLookAndFeel::accentPrimary()
                           : OrpheusLookAndFeel::borderDefault());
        g.drawRoundedRectangle(pill, 4.0f, 1.0f);

        // Note name
        g.setColour(isRoot ? OrpheusLookAndFeel::accentPrimary()
                           : OrpheusLookAndFeel::textPrimary());
        g.setFont(juce::Font(11.0f).boldened());
        g.drawText(noteNameOnly(noteVal), pill, juce::Justification::centred);

        // Scale degree beneath
        if (i < 12)
        {
            g.setColour(OrpheusLookAndFeel::textMuted());
            g.setFont(8.0f);
            g.drawText(romanNumerals[i],
                       (int)nx, (int)(ny + 23), (int)(noteW - 3), 10,
                       juce::Justification::centred);
        }

        nx += noteW;
    }

    // Separator
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(area.getBottom() + (int)notesArea.getHeight() / 2 + 16,
                         (float)getLocalBounds().getX(), (float)getLocalBounds().getRight());
}

// =============================================================================
// Scale Piano — mini keyboard highlighting in-scale notes, with live note lit up
// =============================================================================
void PitchGamePanel::paintScalePiano(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto pianoArea = area.reduced(12, 4);
    int numKeys = 12;
    float keyW = (float)pianoArea.getWidth() / numKeys;
    float keyH = (float)pianoArea.getHeight();

    int detectedNoteInOctave = -1;
    if (smoothedDetectedMidi > 20.0f)
        detectedNoteInOctave = ((int)std::round(smoothedDetectedMidi)) % 12;

    for (int i = 0; i < numKeys; ++i)
    {
        int noteVal = (currentKey + i) % 12;
        bool inScale = isNoteInScale(noteVal);
        bool isRoot = (noteVal == currentKey);
        bool isActive = (noteVal == detectedNoteInOctave) && gameActive;

        float x = pianoArea.getX() + i * keyW;
        auto keyRect = juce::Rectangle<float>(x + 1, (float)pianoArea.getY(), keyW - 2, keyH);

        // Key fill
        if (isActive)
        {
            float err = std::abs(smoothedCentError);
            float t = juce::jmin(1.0f, err / 50.0f);
            auto col = OrpheusLookAndFeel::accentSuccess().interpolatedWith(
                OrpheusLookAndFeel::accentDanger(), t);
            g.setColour(col.withAlpha(0.7f));

            // Glow for active key
            g.fillRoundedRectangle(keyRect.expanded(2), 4.0f);
        }
        else if (isRoot)
        {
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.3f));
        }
        else if (inScale)
        {
            g.setColour(OrpheusLookAndFeel::bgElevated());
        }
        else
        {
            g.setColour(OrpheusLookAndFeel::bgDarkest());
        }
        g.fillRoundedRectangle(keyRect, 3.0f);

        // Border
        g.setColour(isRoot ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.6f)
                           : OrpheusLookAndFeel::borderSubtle());
        g.drawRoundedRectangle(keyRect, 3.0f, 1.0f);

        // Note label
        g.setColour(inScale ? OrpheusLookAndFeel::textPrimary()
                            : OrpheusLookAndFeel::textDisabled());
        g.setFont(juce::Font(9.0f).boldened());
        g.drawText(noteNameOnly(noteVal), keyRect, juce::Justification::centred);
    }

    // Separator
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(area.getBottom() - 1, (float)area.getX(), (float)area.getRight());
}

// =============================================================================
// Note Labels — vertical strip
// =============================================================================
void PitchGamePanel::paintNoteLabels(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (smoothedDetectedMidi < 20.0f) return;

    float centerMidi = smoothedDetectedMidi;
    float rangeNotes = 12.0f;
    float pxPerNote = (float)bounds.getHeight() / rangeNotes;

    g.setFont(11.0f);

    for (int note = (int)(centerMidi - 7); note <= (int)(centerMidi + 7); ++note)
    {
        if (note < 0 || note > 127) continue;
        float yPos = bounds.getCentreY() - (note - centerMidi) * pxPerNote;
        if (yPos < bounds.getY() || yPos > bounds.getBottom()) continue;

        bool inScale = isNoteInScale(note % 12);
        bool isRoot  = (note % 12) == currentKey;

        g.setColour(isRoot  ? OrpheusLookAndFeel::accentPrimary() :
                    inScale ? OrpheusLookAndFeel::textPrimary().withAlpha(0.8f)
                            : OrpheusLookAndFeel::textMuted().withAlpha(0.25f));
        g.drawText(midiNoteToName(note),
                   bounds.getX(), (int)(yPos - 8), bounds.getWidth() - 4, 16,
                   juce::Justification::centredRight);
    }
}

// =============================================================================
// Pitch Ladder
// =============================================================================
void PitchGamePanel::paintPitchLadder(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (smoothedDetectedMidi < 20.0f)
    {
        g.setColour(OrpheusLookAndFeel::textMuted());
        g.setFont(13.0f);
        g.drawText("Sing into your microphone...", bounds, juce::Justification::centred);
        return;
    }

    float centerMidi = smoothedDetectedMidi;
    float rangeNotes = 12.0f;
    float pxPerNote  = (float)bounds.getHeight() / rangeNotes;

    // Grid lines
    for (int note = (int)(centerMidi - 7); note <= (int)(centerMidi + 7); ++note)
    {
        if (note < 0 || note > 127) continue;
        float yPos = (float)bounds.getCentreY() - (note - centerMidi) * pxPerNote;
        if (yPos < bounds.getY() || yPos > bounds.getBottom()) continue;

        bool inScale = isNoteInScale(note % 12);
        bool isRoot  = (note % 12) == currentKey;

        g.setColour(isRoot  ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.35f) :
                    inScale ? OrpheusLookAndFeel::accentPrimary().withAlpha(0.18f)
                            : OrpheusLookAndFeel::borderSubtle().withAlpha(0.1f));
        g.drawHorizontalLine((int)yPos, (float)bounds.getX(), (float)bounds.getRight());

        // Thick highlight for root notes
        if (isRoot)
        {
            g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.08f));
            g.fillRect((float)bounds.getX(), yPos - pxPerNote * 0.25f,
                       (float)bounds.getWidth(), pxPerNote * 0.5f);
        }
    }

    // Scrolling trail
    float sampleW = (float)bounds.getWidth() / (float)HISTORY_SIZE;
    for (int i = 0; i < HISTORY_SIZE; ++i)
    {
        int idx = (historyWriteIndex - HISTORY_SIZE + i + HISTORY_SIZE * 2) % HISTORY_SIZE;
        auto& s = pitchHistory[(size_t)idx];
        if (!s.hasVoice) continue;

        float yDet = (float)bounds.getCentreY() - (s.detectedMidi - centerMidi) * pxPerNote;
        if (yDet < bounds.getY() || yDet > bounds.getBottom()) continue;

        float absErr = std::abs(s.centError);
        float t = juce::jmin(1.0f, absErr / 50.0f);
        auto dotCol = OrpheusLookAndFeel::accentSuccess().interpolatedWith(
            OrpheusLookAndFeel::accentDanger(), t);

        float age = (float)i / (float)HISTORY_SIZE;
        dotCol = dotCol.withAlpha(age * age * 0.85f);

        float xPos = bounds.getX() + i * sampleW;

        if (age > 0.9f)
        {
            g.setColour(dotCol.withAlpha(0.18f));
            g.fillEllipse(xPos - 4.0f, yDet - 4.0f, sampleW + 8.0f, 10.0f);
        }
        g.setColour(dotCol);
        g.fillEllipse(xPos, yDet - 2.0f, sampleW + 1.0f, 5.0f);

        // Also draw the target note position as a faint line
        if (s.targetMidi > 0 && age > 0.5f)
        {
            float yTarget = (float)bounds.getCentreY() - (s.targetMidi - centerMidi) * pxPerNote;
            if (yTarget > bounds.getY() && yTarget < bounds.getBottom())
            {
                g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(age * 0.15f));
                g.fillRect(xPos, yTarget - 1.0f, sampleW + 1.0f, 2.0f);
            }
        }
    }
}

// =============================================================================
// Current Pitch Indicator — the live orb
// =============================================================================
void PitchGamePanel::paintPitchIndicator(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (smoothedDetectedMidi < 20.0f) return;

    float centerMidi = smoothedDetectedMidi;
    float rangeNotes = 12.0f;
    float pxPerNote  = (float)bounds.getHeight() / rangeNotes;
    float yPos       = (float)bounds.getCentreY();

    // Target note line
    float targetMidi = getClosestTargetNote(centerMidi);
    float yTarget = (float)bounds.getCentreY() - (targetMidi - centerMidi) * pxPerNote;
    if (yTarget > bounds.getY() && yTarget < bounds.getBottom())
    {
        g.setColour(OrpheusLookAndFeel::accentPrimary().withAlpha(0.5f));
        g.drawHorizontalLine((int)yTarget, (float)bounds.getX(), (float)bounds.getRight());

        // Target label
        int targetNote = (int)std::round(targetMidi);
        g.setColour(OrpheusLookAndFeel::accentPrimary());
        g.setFont(juce::Font(10.0f).boldened());
        g.drawText("TARGET: " + midiNoteToName(targetNote),
                   bounds.getX() + 4, (int)(yTarget - 16), 120, 14,
                   juce::Justification::centredLeft);

        // Arrow marker
        juce::Path arrow;
        arrow.addTriangle((float)bounds.getRight() - 12, yTarget - 6,
                          (float)bounds.getRight() - 12, yTarget + 6,
                          (float)bounds.getRight() - 2,  yTarget);
        g.fillPath(arrow);
    }

    // Live orb
    float absErr = std::abs(smoothedCentError);
    float t = juce::jmin(1.0f, absErr / 50.0f);
    auto orbCol = OrpheusLookAndFeel::accentSuccess().interpolatedWith(
        OrpheusLookAndFeel::accentDanger(), t);

    float orbX = (float)bounds.getRight() - 36.0f;
    float orbR = 12.0f;

    // Glow
    g.setColour(orbCol.withAlpha(0.2f * indicatorGlow));
    g.fillEllipse(orbX - orbR * 2, yPos - orbR * 2, orbR * 4, orbR * 4);

    // Orb
    g.setColour(orbCol);
    g.fillEllipse(orbX - orbR, yPos - orbR, orbR * 2, orbR * 2);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(orbX - orbR + 3, yPos - orbR + 3, orbR - 2, orbR - 2);

    // Cent deviation
    g.setColour(orbCol);
    g.setFont(juce::Font(13.0f).boldened());
    juce::String centText = (smoothedCentError >= 0 ? "+" : "") +
                            juce::String(smoothedCentError, 0) + " ct";
    g.drawText(centText, (int)(orbX - 40), (int)(yPos + orbR + 4), 80, 16,
               juce::Justification::centred);

    // Detected note name
    int currentNote = (int)std::round(smoothedDetectedMidi);
    bool noteInScale = isNoteInScale(currentNote % 12);
    g.setColour(noteInScale ? OrpheusLookAndFeel::accentSuccess()
                            : OrpheusLookAndFeel::accentWarning());
    g.setFont(juce::Font(18.0f).boldened());
    g.drawText(midiNoteToName(currentNote),
               (int)(orbX - 40), (int)(yPos - orbR - 24), 80, 20,
               juce::Justification::centred);

    // "IN KEY" / "OUT OF KEY" badge
    auto badgeRect = juce::Rectangle<int>((int)(orbX - 30), (int)(yPos - orbR - 42), 60, 14);
    g.setColour(noteInScale ? OrpheusLookAndFeel::accentSuccess().withAlpha(0.2f)
                            : OrpheusLookAndFeel::accentDanger().withAlpha(0.2f));
    g.fillRoundedRectangle(badgeRect.toFloat(), 3.0f);
    g.setColour(noteInScale ? OrpheusLookAndFeel::accentSuccess()
                            : OrpheusLookAndFeel::accentDanger());
    g.setFont(juce::Font(8.0f).boldened());
    g.drawText(noteInScale ? "IN KEY" : "OUT OF KEY", badgeRect, juce::Justification::centred);
}

// =============================================================================
// Accuracy Meter — vertical bar
// =============================================================================
void PitchGamePanel::paintAccuracyMeter(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto area = bounds.reduced(16, 16);

    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("ACCURACY", area.removeFromTop(14), juce::Justification::centred);

    area = area.reduced(4, 4);

    g.setColour(OrpheusLookAndFeel::bgDark());
    g.fillRoundedRectangle(area.toFloat(), 4.0f);

    int fillH = (int)(area.getHeight() * smoothedAccuracy);
    auto fillRect = area.withTop(area.getBottom() - fillH);
    auto fillCol = OrpheusLookAndFeel::accentSuccess().interpolatedWith(
        OrpheusLookAndFeel::accentDanger(), 1.0f - smoothedAccuracy);

    g.setGradientFill(juce::ColourGradient(
        fillCol.brighter(0.2f), (float)fillRect.getX(), (float)fillRect.getY(),
        fillCol.darker(0.3f),   (float)fillRect.getRight(), (float)fillRect.getBottom(), false));
    g.fillRoundedRectangle(fillRect.toFloat(), 4.0f);

    g.setColour(OrpheusLookAndFeel::borderDefault());
    g.drawRoundedRectangle(area.toFloat(), 4.0f, 1.0f);

    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(13.0f).boldened());
    g.drawText(juce::String((int)(smoothedAccuracy * 100.0f)) + "%",
               area.getX(), area.getBottom() + 2, area.getWidth(), 18,
               juce::Justification::centred);
}

// =============================================================================
// Session Score Bar — streak, average, best streak
// =============================================================================
void PitchGamePanel::paintSessionScore(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(OrpheusLookAndFeel::bgDarker().withAlpha(0.8f));
    g.fillRect(area);
    g.setColour(OrpheusLookAndFeel::borderSubtle());
    g.drawHorizontalLine(area.getY(), (float)area.getX(), (float)area.getRight());

    auto row = area.reduced(12, 4);
    int colW = row.getWidth() / 3;

    // Session avg accuracy
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("SESSION AVG", row.getX(), row.getY(), colW, 12, juce::Justification::centred);
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText(juce::String((int)(sessionScore * 100.0f)) + "%",
               row.getX(), row.getY() + 12, colW, 18, juce::Justification::centred);

    // Current streak
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("STREAK", row.getX() + colW, row.getY(), colW, 12, juce::Justification::centred);
    g.setColour(streakCount > 5 ? OrpheusLookAndFeel::accentSuccess()
                                : OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText(juce::String(streakCount),
               row.getX() + colW, row.getY() + 12, colW, 18, juce::Justification::centred);

    // Best streak
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(9.0f).boldened());
    g.drawText("BEST STREAK", row.getX() + colW * 2, row.getY(), colW, 12, juce::Justification::centred);
    g.setColour(OrpheusLookAndFeel::accentWarning());
    g.setFont(juce::Font(14.0f).boldened());
    g.drawText(juce::String(bestStreak),
               row.getX() + colW * 2, row.getY() + 12, colW, 18, juce::Justification::centred);
}

// =============================================================================
// Resized — layout child components (combo boxes)
// =============================================================================
void PitchGamePanel::resized()
{
    // Key/Scale combos live in the key info bar area
    int comboY = 48 + 10; // below header
    int leftPad = 12;

    keyLabel.setBounds(leftPad, comboY, 30, 16);
    keyCombo.setBounds(leftPad + 30, comboY, 60, 20);

    scaleLabel.setBounds(leftPad, comboY + 24, 40, 16);
    scaleCombo.setBounds(leftPad + 40, comboY + 24, 120, 20);
}

// =============================================================================
// Mouse Handling — toggle switch
// =============================================================================
void PitchGamePanel::mouseDown(const juce::MouseEvent& e) { (void)e; }

void PitchGamePanel::mouseUp(const juce::MouseEvent& e)
{
    if (switchBounds.contains(e.getPosition()))
    {
        gameActive = !gameActive;
        if (!gameActive)
        {
            smoothedAccuracy = 0.0f;
            pitchHistory.fill({});
        }
        else
        {
            // Reset session scores on activation
            sessionTotalSamples = 0;
            sessionAccumAccuracy = 0.0f;
            streakCount = 0;
            bestStreak = 0;
            sessionScore = 0.0f;
        }
        repaint();
    }
}
