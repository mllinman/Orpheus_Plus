#pragma once
#include <JuceHeader.h>
#include "../AI/SmartTrackFixer.h"
#include "../AI/TrackElementClassifier.h"
#include "../Audio/AudioEngine.h"
#include "OrpheusLookAndFeel.h"

/**
 * SmartTrackFixerPanel
 *
 * A dedicated UI panel for the AI Smart Track Fixer feature.
 * Displays each track with its detected instrument type, confidence badge,
 * and instrument-colored icon. Provides controls for analyzing all tracks,
 * applying fixes individually or globally, adjusting fix intensity,
 * and viewing frequency conflict warnings.
 */
class SmartTrackFixerPanel : public juce::Component,
                              private juce::Timer
{
public:
    SmartTrackFixerPanel(AudioEngine& engine);
    ~SmartTrackFixerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    // Actions
    void onAnalyzeAll();
    void onFixAll();
    void onFixTrack(int trackIndex);

    // Drawing helpers
    void paintTrackRow(juce::Graphics& g, juce::Rectangle<int> bounds,
                       int trackIndex, bool isAlternate);
    void paintConflictCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                           const SmartTrackFixer::FrequencyConflict& conflict, int index);
    void paintConfidenceBadge(juce::Graphics& g, juce::Rectangle<int> bounds,
                              float confidence);
    void paintInstrumentIcon(juce::Graphics& g, juce::Rectangle<int> bounds,
                             TrackElementClassifier::InstrumentType type);

    // ── Engine reference ──
    AudioEngine& audioEngine;

    // ── AI modules ──
    SmartTrackFixer trackFixer;

    // ── State ──
    SmartTrackFixer::FullFixResult lastResult;
    bool hasAnalyzed = false;
    bool isProcessing = false;

    // ── UI Elements ──
    juce::Label titleLabel      { "title",     "AI Smart Track Fixer" };
    juce::Label subtitleLabel   { "subtitle",  "Instrument-aware track corrections" };

    juce::TextButton analyzeButton  { "Analyze All Tracks" };
    juce::TextButton fixAllButton   { "Fix All Tracks" };

    juce::Label intensityLabel  { "intLabel",  "FIX INTENSITY" };
    juce::Slider intensitySlider;

    // Per-track fix toggles
    struct TrackFixControls
    {
        juce::ToggleButton eqToggle      { "EQ" };
        juce::ToggleButton dynToggle     { "Dyn" };
        juce::ToggleButton stereoToggle  { "Stereo" };
        juce::ToggleButton transToggle   { "Trans" };
        juce::TextButton   fixButton     { "Fix" };
    };
    juce::OwnedArray<TrackFixControls> trackControls;

    // Scroll support
    juce::Viewport trackListViewport;
    std::unique_ptr<juce::Component> trackListContent;
    juce::Viewport conflictViewport;
    std::unique_ptr<juce::Component> conflictContent;

    static constexpr int trackRowHeight = 56;
    static constexpr int conflictCardHeight = 70;
    static constexpr int headerHeight = 160;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartTrackFixerPanel)
};
