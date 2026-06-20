#include "ScoreViewComponent.h"
#include "../UI/OrpheusLookAndFeel.h"

ScoreViewComponent::ScoreViewComponent()
{
}

ScoreViewComponent::~ScoreViewComponent()
{
}

void ScoreViewComponent::setMidiClip(MidiClip* clip)
{
    activeClip = clip;
    repaint();
}

void ScoreViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xfff5f5f5)); // Standard sheet music off-white

    auto bounds = getLocalBounds();
    int staffY = bounds.getHeight() / 2;

    drawStaff(g, staffY);

    if (activeClip)
    {
        // Simple mapping just to verify render
        int xPos = 50;
        for (auto* note : activeClip->notes)
        {
            drawNote(g, note, staffY, xPos);
            xPos += 40; // Spacing
        }
    }
}

void ScoreViewComponent::resized()
{
}

void ScoreViewComponent::drawStaff(juce::Graphics& g, int yCenter)
{
    g.setColour(juce::Colours::black);
    // Draw 5 staff lines
    for (int i = -2; i <= 2; ++i)
    {
        g.drawHorizontalLine(yCenter + (i * 10), 20.0f, (float)getWidth() - 20.0f);
    }
}

void ScoreViewComponent::drawNote(juce::Graphics& g, MidiNote* note, int staffY, int xPos)
{
    g.setColour(juce::Colours::black);
    
    // Very naive mapping of pitch to y offset
    int pitchOffset = (60 - note->pitch) * 5; 
    
    // Draw note head
    g.fillEllipse((float)xPos, (float)(staffY + pitchOffset), 12.0f, 10.0f);
    
    // Draw stem
    g.drawLine((float)(xPos + 11), (float)(staffY + pitchOffset + 5), (float)(xPos + 11), (float)(staffY + pitchOffset - 30), 1.5f);
}
