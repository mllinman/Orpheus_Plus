#include "SessionViewPanel.h"
#include "OrpheusLookAndFeel.h"

SessionViewPanel::SessionViewPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    audioEngine.addListener(this);
    
    // Default 8 scenes
    for (int i = 0; i < 8; ++i)
        scenes.push_back({ juce::String("Scene ") + juce::String(i + 1) });
        
    trackListChanged();
}

SessionViewPanel::~SessionViewPanel()
{
    audioEngine.removeListener(this);
}

void SessionViewPanel::trackListChanged()
{
    int numTracks = audioEngine.getNumTracks();
    clipGrid.resize(numTracks);
    for (auto& col : clipGrid)
    {
        col.resize(scenes.size());
    }
    repaint();
}

void SessionViewPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
    drawGrid(g);
}

void SessionViewPanel::drawGrid(juce::Graphics& g)
{
    int numTracks = audioEngine.getNumTracks();
    if (numTracks == 0)
    {
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.drawText("No tracks available for Session View", getLocalBounds(), juce::Justification::centred);
        return;
    }

    int cellWidth = 100;
    int cellHeight = 60;
    int headerHeight = 30;
    int sceneHeaderWidth = 80;

    // Draw Track Headers
    for (int t = 0; t < numTracks; ++t)
    {
        juce::Rectangle<int> headerRect(t * cellWidth, 0, cellWidth, headerHeight);
        g.setColour(OrpheusLookAndFeel::bgPanel());
        g.fillRect(headerRect.reduced(1));
        
        g.setColour(OrpheusLookAndFeel::textPrimary());
        g.setFont(12.0f);
        g.drawText(audioEngine.getTrackInfo(t).name, headerRect, juce::Justification::centred);
    }

    // Draw Scene Headers (Right Side)
    for (size_t s = 0; s < scenes.size(); ++s)
    {
        juce::Rectangle<int> sceneRect(numTracks * cellWidth, headerHeight + (int)s * cellHeight, sceneHeaderWidth, cellHeight);
        g.setColour(OrpheusLookAndFeel::bgPanel().darker(0.2f));
        g.fillRect(sceneRect.reduced(1));
        
        // Play button triangle
        juce::Path playBtn;
        float cx = sceneRect.getX() + 20.0f;
        float cy = sceneRect.getCentreY();
        playBtn.addTriangle(cx - 5, cy - 6, cx + 7, cy, cx - 5, cy + 6);
        g.setColour(OrpheusLookAndFeel::accent());
        g.fillPath(playBtn);
        
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.drawText(scenes[s].name, sceneRect.withTrimmedLeft(40), juce::Justification::centredLeft);
    }

    // Draw Clip Grid
    for (int t = 0; t < numTracks; ++t)
    {
        for (size_t s = 0; s < scenes.size(); ++s)
        {
            juce::Rectangle<int> cellRect(t * cellWidth, headerHeight + (int)s * cellHeight, cellWidth, cellHeight);
            drawClipCell(g, t, (int)s, cellRect);
        }
    }
}

void SessionViewPanel::drawClipCell(juce::Graphics& g, int trackIdx, int sceneIdx, juce::Rectangle<int> bounds)
{
    bounds = bounds.reduced(1);
    
    // Draw empty cell slot
    g.setColour(OrpheusLookAndFeel::bgPanel().darker(0.5f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    if (trackIdx < clipGrid.size() && sceneIdx < clipGrid[trackIdx].size())
    {
        auto* clip = clipGrid[trackIdx][sceneIdx].get();
        if (clip)
        {
            // Draw filled clip
            g.setColour(clip->colour);
            g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            
            // Draw play status
            if (clip->isPlaying)
            {
                g.setColour(juce::Colours::white.withAlpha(0.3f));
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
                
                // Playhead progress
                float progress = (float)(std::fmod(clip->playheadPosition, 1.0));
                g.setColour(juce::Colours::white.withAlpha(0.8f));
                g.drawVerticalLine(bounds.getX() + (int)(bounds.getWidth() * progress), bounds.getY(), bounds.getBottom());
            }
            else if (clip->isQueued)
            {
                // Pulsing green to indicate queued
                g.setColour(juce::Colours::green.withAlpha(0.5f + 0.3f * std::sin(juce::Time::getMillisecondCounter() * 0.01f)));
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            }
            
            g.setColour(juce::Colours::black);
            g.setFont(12.0f);
            g.drawText(clip->name, bounds.reduced(4, 2), juce::Justification::centredLeft, true);
        }
        else
        {
            // Empty slot outline
            g.setColour(OrpheusLookAndFeel::textSecondary().withAlpha(0.1f));
            g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 1.0f);
        }
    }
}

void SessionViewPanel::resized()
{
}
