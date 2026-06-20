#include "SpatialPannerUI.h"

SpatialPannerUI::SpatialPannerUI(AudioEngine& engineToUse, int trackIdx)
    : engine(engineToUse), trackIndex(trackIdx)
{
    addAndMakeVisible(enableButton);
    addAndMakeVisible(elevationSlider);
    addAndMakeVisible(elevationLabel);

    elevationLabel.setJustificationType(juce::Justification::centred);
    elevationLabel.attachToComponent(&elevationSlider, false);

    elevationSlider.setRange(-90.0, 90.0, 1.0);
    elevationSlider.setValue(0.0);
    elevationSlider.onValueChange = [this]()
    {
        currentElevation = (float)elevationSlider.getValue();
        engine.setTrackSpatialPosition(trackIndex, currentAzimuth, currentElevation, currentDistance);
    };

    enableButton.onClick = [this]()
    {
        isSpatialEnabled = enableButton.getToggleState();
        engine.setTrackSpatialMode(trackIndex, isSpatialEnabled);
    };

    startTimerHz(30);
}

SpatialPannerUI::~SpatialPannerUI()
{
}

void SpatialPannerUI::paint(juce::Graphics& g)
{
    // Draw Glassmorphic Background
    auto bounds = getLocalBounds().toFloat();
    juce::Colour bgColour = juce::Colours::black.withAlpha(0.3f);
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 8.0f);

    // Draw grid for panner
    juce::Rectangle<float> panArea(10, 40, getWidth() - 100, getHeight() - 50);
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRoundedRectangle(panArea, 8.0f, 2.0f);
    
    // Draw center lines
    g.drawLine(panArea.getCentreX(), panArea.getY(), panArea.getCentreX(), panArea.getBottom(), 1.0f);
    g.drawLine(panArea.getX(), panArea.getCentreY(), panArea.getRight(), panArea.getCentreY(), 1.0f);

    // Draw sound source position
    float maxDist = 10.0f; // Max distance represented by the UI
    float displayDist = juce::jlimit(0.0f, maxDist, currentDistance);
    
    // Convert polar (azimuth, distance) to cartesian for UI
    // Azimuth 0 = straight ahead (top of panArea)
    // Azimuth 90 = right
    float angleRad = juce::MathConstants<float>::pi * (currentAzimuth - 90.0f) / 180.0f;
    float normalizedDist = displayDist / maxDist; // 0 to 1
    
    float radius = (panArea.getWidth() / 2.0f) * normalizedDist;
    float posX = panArea.getCentreX() + radius * std::cos(angleRad);
    float posY = panArea.getCentreY() + radius * std::sin(angleRad);

    g.setColour(isSpatialEnabled ? juce::Colours::cyan : juce::Colours::darkgrey);
    g.fillEllipse(posX - 8.0f, posY - 8.0f, 16.0f, 16.0f);
    
    // Glow effect
    if (isSpatialEnabled)
    {
        juce::ColourGradient glow(juce::Colours::cyan.withAlpha(0.6f), posX, posY,
                                  juce::Colours::cyan.withAlpha(0.0f), posX, posY + 20.0f, true);
        g.setGradientFill(glow);
        g.fillEllipse(posX - 20.0f, posY - 20.0f, 40.0f, 40.0f);
    }
}

void SpatialPannerUI::resized()
{
    auto area = getLocalBounds();
    enableButton.setBounds(area.removeFromTop(30).reduced(5));
    
    auto rightArea = area.removeFromRight(80);
    elevationSlider.setBounds(rightArea.reduced(5).withTrimmedTop(20));
}

void SpatialPannerUI::timerCallback()
{
    // Update UI from engine state
    if (auto* t = engine.getTrack(trackIndex))
    {
        if (t->spatialEnabled != enableButton.getToggleState())
        {
            enableButton.setToggleState(t->spatialEnabled, juce::dontSendNotification);
            isSpatialEnabled = t->spatialEnabled;
            repaint();
        }
        
        if (std::abs(t->panAzimuth - currentAzimuth) > 0.1f ||
            std::abs(t->panDistance - currentDistance) > 0.1f ||
            std::abs(t->panElevation - currentElevation) > 0.1f)
        {
            currentAzimuth = t->panAzimuth;
            currentDistance = t->panDistance;
            currentElevation = t->panElevation;
            elevationSlider.setValue(currentElevation, juce::dontSendNotification);
            repaint();
        }
    }
}

void SpatialPannerUI::mouseDown(const juce::MouseEvent& e)
{
    updateSpatialPositionFromMouse(e);
}

void SpatialPannerUI::mouseDrag(const juce::MouseEvent& e)
{
    updateSpatialPositionFromMouse(e);
}

void SpatialPannerUI::updateSpatialPositionFromMouse(const juce::MouseEvent& e)
{
    juce::Rectangle<float> panArea(10, 40, getWidth() - 100, getHeight() - 50);
    
    if (panArea.contains(e.position))
    {
        float dx = e.position.x - panArea.getCentreX();
        float dy = e.position.y - panArea.getCentreY();
        
        // Calculate Azimuth (-180 to 180)
        // atan2(dy, dx) returns angle from positive X axis (right). 
        // We want 0 to be top (negative Y), 90 to be right (positive X)
        float angleRad = std::atan2(dy, dx);
        currentAzimuth = (angleRad * 180.0f / juce::MathConstants<float>::pi) + 90.0f;
        
        if (currentAzimuth > 180.0f) currentAzimuth -= 360.0f;
        
        // Calculate Distance
        float maxRadius = panArea.getWidth() / 2.0f;
        float dist = std::sqrt(dx*dx + dy*dy);
        currentDistance = juce::jlimit(0.0f, 10.0f, (dist / maxRadius) * 10.0f);
        
        engine.setTrackSpatialPosition(trackIndex, currentAzimuth, currentElevation, currentDistance);
        repaint();
    }
}
