#include "ModulationMatrixPanel.h"
#include "OrpheusLookAndFeel.h"

ModulationMatrixPanel::ModulationMatrixPanel(AudioEngine& engine)
    : audioEngine(engine)
{
    addAndMakeVisible(addLfoButton);
    addLfoButton.onClick = [this] {
        audioEngine.addLFO();
        repaint();
    };
    
    startTimerHz(30); // 30fps UI refresh for LFO visuals
}

ModulationMatrixPanel::~ModulationMatrixPanel()
{
    stopTimer();
}

void ModulationMatrixPanel::timerCallback()
{
    repaint();
}

void ModulationMatrixPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    addLfoButton.setBounds(bounds.removeFromTop(30).withWidth(100));
}

void ModulationMatrixPanel::paint(juce::Graphics& g)
{
    g.fillAll(OrpheusLookAndFeel::bgDarkest());
    
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(40); // space for button
    
    auto lfoArea = bounds.removeFromTop(150);
    
    auto& lfos = audioEngine.getLFOs();
    if (lfos.empty())
    {
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.drawText("No LFOs active. Click 'Add LFO' to create one.", lfoArea, juce::Justification::centred);
    }
    else
    {
        int boxWidth = 120;
        int spacing = 10;
        for (int i = 0; i < (int)lfos.size(); ++i)
        {
            juce::Rectangle<int> boxRect(lfoArea.getX() + i * (boxWidth + spacing), lfoArea.getY(), boxWidth, lfoArea.getHeight());
            drawLFOBox(g, boxRect, lfos[i], i);
        }
    }
    
    bounds.removeFromTop(20);
    drawMappingsList(g, bounds);
}

void ModulationMatrixPanel::drawLFOBox(juce::Graphics& g, juce::Rectangle<int> bounds, const LFOSource& lfo, int index)
{
    g.setColour(OrpheusLookAndFeel::bgPanel());
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(OrpheusLookAndFeel::textSecondary().withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);
    
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(14.0f);
    g.drawText("LFO " + juce::String(index + 1), bounds.removeFromTop(25), juce::Justification::centred);
    
    // Draw the waveform (mocked visual based on state)
    // LFOSource tick() returns a value in [-depth, +depth]. 
    // We don't want to advance it here, but we can draw a static sine wave 
    // and a dot for its current phase. Since we don't have a getPhase() exposed,
    // we just draw a static sine wave.
    auto waveArea = bounds.reduced(10);
    g.setColour(OrpheusLookAndFeel::accent());
    
    juce::Path wave;
    for (float x = 0; x < waveArea.getWidth(); ++x)
    {
        float phase = x / waveArea.getWidth();
        float y = std::sin(phase * juce::MathConstants<float>::twoPi);
        float pixelY = waveArea.getCentreY() - y * (waveArea.getHeight() * 0.4f);
        
        if (x == 0) wave.startNewSubPath(waveArea.getX() + x, pixelY);
        else wave.lineTo(waveArea.getX() + x, pixelY);
    }
    g.strokePath(wave, juce::PathStrokeType(2.0f));
    
    g.setColour(OrpheusLookAndFeel::textSecondary());
    g.setFont(10.0f);
    g.drawText(juce::String(lfo.getRate(), 1) + " Hz", bounds.removeFromBottom(20), juce::Justification::centred);
}

void ModulationMatrixPanel::drawMappingsList(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(OrpheusLookAndFeel::textPrimary());
    g.setFont(16.0f);
    g.drawText("Modulation Matrix Mappings", bounds.removeFromTop(20), juce::Justification::topLeft);
    
    g.setColour(OrpheusLookAndFeel::bgPanel().darker(0.2f));
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    auto& mappings = audioEngine.getModMappings();
    if (mappings.empty())
    {
        g.setColour(OrpheusLookAndFeel::textSecondary());
        g.setFont(12.0f);
        g.drawText("Drag an LFO to a parameter to create a mapping.", bounds, juce::Justification::centred);
    }
    else
    {
        int rowHeight = 30;
        for (int i = 0; i < (int)mappings.size(); ++i)
        {
            auto rowBounds = bounds.removeFromTop(rowHeight).reduced(4, 2);
            g.setColour(i % 2 == 0 ? juce::Colours::transparentWhite : juce::Colours::white.withAlpha(0.05f));
            g.fillRect(rowBounds);
            
            g.setColour(OrpheusLookAndFeel::textPrimary());
            g.drawText("LFO " + juce::String(mappings[i].sourceId + 1) + 
                       "  ->  Track " + juce::String(mappings[i].trackIndex) + 
                       " [" + mappings[i].targetParam + "]   " + 
                       juce::String(mappings[i].depth * 100.0f, 0) + "%", 
                       rowBounds.withTrimmedLeft(10), juce::Justification::centredLeft);
        }
    }
}
