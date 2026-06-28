#include "SmartTrackFixerPanel.h"
#include <cmath>

//==============================================================================
SmartTrackFixerPanel::SmartTrackFixerPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    // ── Title ──
    addAndMakeVisible(titleLabel);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textPrimary());
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(subtitleLabel);
    subtitleLabel.setFont(juce::Font(13.0f));
    subtitleLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);

    // ── Analyze Button ──
    addAndMakeVisible(analyzeButton);
    analyzeButton.setColour(juce::TextButton::buttonColourId, OrpheusLookAndFeel::accentPrimary());
    analyzeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    analyzeButton.onClick = [this] { onAnalyzeAll(); };

    // ── Fix All Button ──
    addAndMakeVisible(fixAllButton);
    fixAllButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2ecc71));
    fixAllButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    fixAllButton.setEnabled(false);
    fixAllButton.onClick = [this] { onFixAll(); };

    // ── Intensity Slider ──
    addAndMakeVisible(intensityLabel);
    intensityLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    intensityLabel.setColour(juce::Label::textColourId, OrpheusLookAndFeel::textSecondary());

    addAndMakeVisible(intensitySlider);
    intensitySlider.setRange(0.0, 1.0, 0.01);
    intensitySlider.setValue(0.75);
    intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    intensitySlider.setColour(juce::Slider::thumbColourId, OrpheusLookAndFeel::accentPrimary());
    intensitySlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a4d));

    // ── Track List Viewport ──
    // Use a custom component that delegates paint to the panel
    class TrackListComponent : public juce::Component
    {
    public:
        SmartTrackFixerPanel& owner;
        TrackListComponent(SmartTrackFixerPanel& o) : owner(o) {}
        void paint(juce::Graphics& g) override
        {
            if (!owner.hasAnalyzed) return;
            int numTracks = (int)owner.lastResult.classifications.size();
            for (int i = 0; i < numTracks; ++i)
            {
                auto rowBounds = juce::Rectangle<int>(0, i * trackRowHeight, getWidth(), trackRowHeight);
                owner.paintTrackRow(g, rowBounds, i, (i % 2 == 1));
            }
        }
    };

    trackListContent = std::make_unique<TrackListComponent>(*this);
    trackListViewport.setViewedComponent(trackListContent.get(), false);
    trackListViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackListViewport);

    // ── Conflict Viewport ──
    class ConflictListComponent : public juce::Component
    {
    public:
        SmartTrackFixerPanel& owner;
        ConflictListComponent(SmartTrackFixerPanel& o) : owner(o) {}
        void paint(juce::Graphics& g) override
        {
            if (!owner.hasAnalyzed) return;
            for (int i = 0; i < (int)owner.lastResult.conflicts.size(); ++i)
            {
                auto cardBounds = juce::Rectangle<int>(0, i * conflictCardHeight,
                                                        getWidth(), conflictCardHeight);
                owner.paintConflictCard(g, cardBounds, owner.lastResult.conflicts[(size_t)i], i);
            }
        }
    };

    conflictContent = std::make_unique<ConflictListComponent>(*this);
    conflictViewport.setViewedComponent(conflictContent.get(), false);
    conflictViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(conflictViewport);

    startTimerHz(4); // Refresh at 4 Hz
}

SmartTrackFixerPanel::~SmartTrackFixerPanel()
{
    stopTimer();
}

//==============================================================================
void SmartTrackFixerPanel::paint(juce::Graphics& g)
{
    // Glassmorphic background
    g.fillAll(juce::Colour(0xff0d0d14));

    auto bounds = getLocalBounds().toFloat();

    // Subtle gradient overlay
    juce::ColourGradient bgGrad(
        juce::Colour(0xff151520), bounds.getX(), bounds.getY(),
        juce::Colour(0xff0a0a12), bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(bounds.reduced(1.0f), 6.0f);

    // Border
    g.setColour(juce::Colour(0xff2a2a3d));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // ── Header accent bar ──
    auto headerBar = bounds.removeFromTop(4.0f);
    juce::ColourGradient accentGrad(
        juce::Colour(0xff6C5CE7), headerBar.getX(), headerBar.getY(),
        juce::Colour(0xff00D2FF), headerBar.getRight(), headerBar.getY(),
        false);
    g.setGradientFill(accentGrad);
    g.fillRect(headerBar);

    // ── Section labels ──
    if (hasAnalyzed)
    {
        auto trackSectionY = headerHeight + 5;
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText("TRACK ANALYSIS", 15, trackSectionY - 18, 200, 16, juce::Justification::centredLeft);

        // Column headers
        g.setFont(juce::Font(10.0f));
        auto headerArea = juce::Rectangle<int>(15, trackSectionY - 2, getWidth() - 30, 14);
        g.drawText("TRACK", headerArea.removeFromLeft(150), juce::Justification::centredLeft);
        g.drawText("TYPE", headerArea.removeFromLeft(120), juce::Justification::centredLeft);
        g.drawText("CONF", headerArea.removeFromLeft(60), juce::Justification::centred);
        g.drawText("CENTROID", headerArea.removeFromLeft(80), juce::Justification::centred);
        g.drawText("FIXES", headerArea, juce::Justification::centred);

        // Conflict section header
        if (!lastResult.conflicts.empty())
        {
            int conflictY = getHeight() - (int)lastResult.conflicts.size() * conflictCardHeight - 30;
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.setColour(juce::Colour(0xffE06C75));
            g.drawText("FREQUENCY CONFLICTS (" + juce::String((int)lastResult.conflicts.size()) + ")",
                        15, conflictY - 18, 300, 16, juce::Justification::centredLeft);
        }
    }
    else
    {
        // Empty state illustration
        auto centerBounds = getLocalBounds().reduced(40).withTrimmedTop(headerHeight);
        g.setColour(juce::Colour(0xff2a2a40));
        g.fillRoundedRectangle(centerBounds.toFloat().reduced(20.0f), 12.0f);

        g.setColour(OrpheusLookAndFeel::textSecondary().withAlpha(0.5f));
        g.setFont(juce::Font(16.0f));
        g.drawText("Click 'Analyze All Tracks' to detect instruments\nand generate intelligent corrections.",
                    centerBounds, juce::Justification::centred);
    }
}

//==============================================================================
void SmartTrackFixerPanel::resized()
{
    auto area = getLocalBounds().reduced(15);

    // ── Header Zone ──
    auto headerZone = area.removeFromTop(headerHeight - 20);

    titleLabel.setBounds(headerZone.removeFromTop(30));
    subtitleLabel.setBounds(headerZone.removeFromTop(20));

    headerZone.removeFromTop(10);

    // Button row
    auto buttonRow = headerZone.removeFromTop(36);
    analyzeButton.setBounds(buttonRow.removeFromLeft(180).reduced(2));
    buttonRow.removeFromLeft(10);
    fixAllButton.setBounds(buttonRow.removeFromLeft(150).reduced(2));

    headerZone.removeFromTop(10);

    // Intensity slider row
    auto sliderRow = headerZone.removeFromTop(28);
    intensityLabel.setBounds(sliderRow.removeFromLeft(100));
    intensitySlider.setBounds(sliderRow.reduced(2));

    area.removeFromTop(15); // Gap after header

    // ── Conflict Zone (bottom) ──
    int conflictHeight = 0;
    if (hasAnalyzed && !lastResult.conflicts.empty())
    {
        conflictHeight = juce::jmin((int)lastResult.conflicts.size() * conflictCardHeight + 25,
                                     getHeight() / 4);
        auto conflictArea = area.removeFromBottom(conflictHeight);
        conflictViewport.setBounds(conflictArea);
        conflictContent->setSize(conflictArea.getWidth() - 14,
                                  (int)lastResult.conflicts.size() * conflictCardHeight);
    }
    else
    {
        conflictViewport.setBounds(0, 0, 0, 0);
    }

    // ── Track List Zone (fills remaining) ──
    area.removeFromTop(15);
    trackListViewport.setBounds(area);

    int numTracks = hasAnalyzed ? (int)lastResult.classifications.size() : 0;
    int contentHeight = juce::jmax(numTracks * trackRowHeight, area.getHeight());
    trackListContent->setSize(area.getWidth() - 14, contentHeight);

    // ── Position per-track controls ──
    for (int i = 0; i < trackControls.size() && i < numTracks; ++i)
    {
        auto* ctrl = trackControls[i];
        int y = i * trackRowHeight;
        int rightX = trackListContent->getWidth() - 220;

        ctrl->eqToggle.setBounds(rightX, y + 8, 40, 20);
        ctrl->dynToggle.setBounds(rightX + 42, y + 8, 40, 20);
        ctrl->stereoToggle.setBounds(rightX + 84, y + 8, 50, 20);
        ctrl->transToggle.setBounds(rightX + 136, y + 8, 45, 20);
        ctrl->fixButton.setBounds(rightX + 183, y + 6, 35, 24);
    }
}

//==============================================================================
void SmartTrackFixerPanel::timerCallback()
{
    // Light refresh for live metering or status updates
    if (isProcessing)
        repaint();
}

//==============================================================================
void SmartTrackFixerPanel::onAnalyzeAll()
{
    isProcessing = true;
    analyzeButton.setEnabled(false);
    analyzeButton.setButtonText("Analyzing...");
    repaint();

    // Run analysis (runs synchronously for now — could be threaded)
    float intensity = (float)intensitySlider.getValue();
    lastResult = trackFixer.analyzeAndFixAll(audioEngine, 0.0f); // Analyze only, don't apply fixes yet

    hasAnalyzed = true;
    isProcessing = false;
    fixAllButton.setEnabled(true);
    analyzeButton.setEnabled(true);
    analyzeButton.setButtonText("Re-Analyze All Tracks");

    // ── Rebuild per-track controls ──
    trackControls.clear();
    int numTracks = (int)lastResult.classifications.size();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* ctrl = new TrackFixControls();

        ctrl->eqToggle.setToggleState(true, juce::dontSendNotification);
        ctrl->dynToggle.setToggleState(true, juce::dontSendNotification);
        ctrl->stereoToggle.setToggleState(true, juce::dontSendNotification);
        ctrl->transToggle.setToggleState(true, juce::dontSendNotification);

        // Style the toggles
        for (auto* toggle : { &ctrl->eqToggle, &ctrl->dynToggle, &ctrl->stereoToggle, &ctrl->transToggle })
        {
            toggle->setColour(juce::ToggleButton::textColourId, OrpheusLookAndFeel::textSecondary());
            toggle->setColour(juce::ToggleButton::tickColourId, OrpheusLookAndFeel::accentPrimary());
            trackListContent->addAndMakeVisible(toggle);
        }

        ctrl->fixButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2ecc71));
        ctrl->fixButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        ctrl->fixButton.onClick = [this, i] { onFixTrack(i); };
        trackListContent->addAndMakeVisible(ctrl->fixButton);

        trackControls.add(ctrl);
    }

    resized();
    repaint();
}

void SmartTrackFixerPanel::onFixAll()
{
    float intensity = (float)intensitySlider.getValue();

    // Regenerate profiles with current intensity and apply
    for (int i = 0; i < (int)lastResult.classifications.size(); ++i)
    {
        auto profile = trackFixer.generateFixProfile(lastResult.classifications[(size_t)i], intensity);
        trackFixer.applyFixToTrack(audioEngine, i, profile);
        lastResult.profiles[(size_t)i] = profile;
    }

    repaint();
}

void SmartTrackFixerPanel::onFixTrack(int trackIndex)
{
    if (trackIndex < 0 || trackIndex >= (int)lastResult.classifications.size())
        return;

    float intensity = (float)intensitySlider.getValue();
    auto profile = trackFixer.generateFixProfile(
        lastResult.classifications[(size_t)trackIndex], intensity);

    trackFixer.applyFixToTrack(audioEngine, trackIndex, profile);
    lastResult.profiles[(size_t)trackIndex] = profile;

    repaint();
}

//==============================================================================
// Drawing Helpers
//==============================================================================

void SmartTrackFixerPanel::paintTrackRow(juce::Graphics& g, juce::Rectangle<int> bounds,
                                          int trackIndex, bool isAlternate)
{
    // Alternate row background
    if (isAlternate)
    {
        g.setColour(juce::Colour(0xff18182a));
        g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    }

    if (trackIndex >= (int)lastResult.classifications.size()) return;

    auto& classification = lastResult.classifications[(size_t)trackIndex];
    auto* track = audioEngine.getTrack(trackIndex);
    juce::String trackName = track ? track->name : "Track " + juce::String(trackIndex + 1);

    auto row = bounds.reduced(4, 2);

    // ── Instrument icon (color dot) ──
    auto iconBounds = row.removeFromLeft(30).withSizeKeepingCentre(24, 24);
    paintInstrumentIcon(g, iconBounds, classification.type);

    row.removeFromLeft(6);

    // ── Track name ──
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(trackName, row.removeFromLeft(120), juce::Justification::centredLeft);

    // ── Instrument type ──
    auto typeColor = TrackElementClassifier::instrumentColour(classification.type);
    g.setColour(typeColor);
    g.setFont(juce::Font(12.0f));
    auto typeName = TrackElementClassifier::instrumentTypeToString(classification.type);
    if (classification.subType.isNotEmpty() && classification.subType != typeName)
        typeName += " (" + classification.subType + ")";
    g.drawText(typeName, row.removeFromLeft(120), juce::Justification::centredLeft);

    // ── Confidence badge ──
    auto confBounds = row.removeFromLeft(60).withSizeKeepingCentre(50, 20);
    paintConfidenceBadge(g, confBounds, classification.confidence);

    // ── Spectral centroid ──
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(11.0f));
    g.drawText(juce::String((int)classification.spectralCentroid) + " Hz",
               row.removeFromLeft(80), juce::Justification::centred);

    // (Toggle controls are positioned as child components in resized())
}

void SmartTrackFixerPanel::paintConflictCard(juce::Graphics& g, juce::Rectangle<int> bounds,
                                               const SmartTrackFixer::FrequencyConflict& conflict,
                                               int /*index*/)
{
    auto card = bounds.reduced(4, 3);

    // Severity-based background color
    float sevAlpha = 0.15f + conflict.severity * 0.2f;
    g.setColour(juce::Colour(0xffE06C75).withAlpha(sevAlpha));
    g.fillRoundedRectangle(card.toFloat(), 6.0f);

    // Border
    g.setColour(juce::Colour(0xffE06C75).withAlpha(0.4f));
    g.drawRoundedRectangle(card.toFloat(), 6.0f, 1.0f);

    auto inner = card.reduced(10, 5);

    // Track names
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(conflict.trackNameA + "  vs  " + conflict.trackNameB,
               inner.removeFromTop(18), juce::Justification::centredLeft);

    // Severity indicator
    auto severityText = conflict.severity > 0.7f ? "HIGH" : (conflict.severity > 0.4f ? "MEDIUM" : "LOW");
    auto severityColor = conflict.severity > 0.7f ? juce::Colour(0xffE06C75)
                       : (conflict.severity > 0.4f ? juce::Colour(0xffE5C07B) : juce::Colour(0xff98C379));
    g.setColour(severityColor);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(severityText, card.removeFromRight(60).reduced(5), juce::Justification::centred);

    // Suggestion
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(juce::Font(11.0f));
    g.drawText(conflict.suggestion, inner, juce::Justification::centredLeft, true);
}

void SmartTrackFixerPanel::paintConfidenceBadge(juce::Graphics& g, juce::Rectangle<int> bounds,
                                                  float confidence)
{
    // Color gradient from red (low) to yellow (medium) to green (high)
    juce::Colour badgeColor;
    if (confidence > 0.7f)
        badgeColor = juce::Colour(0xff2ecc71); // Green
    else if (confidence > 0.4f)
        badgeColor = juce::Colour(0xffE5C07B); // Yellow
    else
        badgeColor = juce::Colour(0xffE06C75); // Red

    g.setColour(badgeColor.withAlpha(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 10.0f);

    g.setColour(badgeColor);
    g.drawRoundedRectangle(bounds.toFloat(), 10.0f, 1.0f);

    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(juce::String((int)(confidence * 100.0f)) + "%",
               bounds, juce::Justification::centred);
}

void SmartTrackFixerPanel::paintInstrumentIcon(juce::Graphics& g, juce::Rectangle<int> bounds,
                                                 TrackElementClassifier::InstrumentType type)
{
    auto color = TrackElementClassifier::instrumentColour(type);

    // Filled circle with instrument color
    g.setColour(color.withAlpha(0.25f));
    g.fillEllipse(bounds.toFloat());

    g.setColour(color);
    g.drawEllipse(bounds.toFloat().reduced(1.0f), 1.5f);

    // First letter of instrument type as icon
    auto typeName = TrackElementClassifier::instrumentTypeToString(type);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(typeName.substring(0, 1), bounds, juce::Justification::centred);
}
