#include "OrpheusLookAndFeel.h"

OrpheusLookAndFeel::OrpheusLookAndFeel()
    : defaultFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain))
{
    setColour(juce::ResizableWindow::backgroundColourId,           backgroundDark());
    setColour(juce::DocumentWindow::backgroundColourId,            backgroundDark());
    setColour(juce::TextButton::buttonColourId,                    backgroundLight());
    setColour(juce::TextButton::buttonOnColourId,                  accent());
    setColour(juce::TextButton::textColourOffId,                   textPrimary());
    setColour(juce::TextButton::textColourOnId,                    juce::Colours::white);
    setColour(juce::Slider::backgroundColourId,                    backgroundMid());
    setColour(juce::Slider::thumbColourId,                         accent());
    setColour(juce::Slider::trackColourId,                         accentBlue().withAlpha(0.6f));
    setColour(juce::Label::textColourId,                           textPrimary());
    setColour(juce::ComboBox::backgroundColourId,                  backgroundLight());
    setColour(juce::ComboBox::textColourId,                        textPrimary());
    setColour(juce::ComboBox::arrowColourId,                       textSecondary());
    setColour(juce::ComboBox::outlineColourId,                     backgroundLight().brighter(0.1f));
    setColour(juce::PopupMenu::backgroundColourId,                 backgroundMid());
    setColour(juce::PopupMenu::textColourId,                       textPrimary());
    setColour(juce::PopupMenu::highlightedBackgroundColourId,      accent().withAlpha(0.8f));
    setColour(juce::PopupMenu::highlightedTextColourId,            juce::Colours::white);
    setColour(juce::ScrollBar::thumbColourId,                      textSecondary().withAlpha(0.5f));
    setColour(juce::ScrollBar::backgroundColourId,                 backgroundDark());
    setColour(juce::ToggleButton::textColourId,                    textPrimary());
    setColour(juce::ToggleButton::tickColourId,                    accentBlue());
    setColour(juce::ToggleButton::tickDisabledColourId,            textSecondary());
    setColour(juce::AlertWindow::backgroundColourId,               backgroundMid());
    setColour(juce::AlertWindow::textColourId,                     textPrimary());
    setColour(juce::TextEditor::backgroundColourId,                backgroundDark());
    setColour(juce::TextEditor::textColourId,                      textPrimary());
    setColour(juce::TextEditor::highlightColourId,                 accent().withAlpha(0.4f));
    setColour(juce::TextEditor::outlineColourId,                   backgroundLight().brighter(0.15f));
}

void OrpheusLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float radius = juce::jmin(w, h) * 0.38f;

    // Track arc
    juce::Path trackArc;
    trackArc.addArc(cx - radius, cy - radius, radius * 2, radius * 2,
                    startAngle, endAngle, true);
    g.setColour(backgroundDark().withAlpha(0.5f));
    g.strokePath(trackArc, juce::PathStrokeType(4.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    float currentAngle = startAngle + sliderPos * (endAngle - startAngle);
    if (sliderPos > 0.0f) {
        juce::Path valueArc;
        valueArc.addArc(cx - radius, cy - radius, radius * 2, radius * 2,
                        startAngle, currentAngle, true);
        
        // Add a slight glow effect
        auto glowParams = juce::DropShadow(accentBlue(), 4, {0, 0});
        g.setColour(accentBlue().withAlpha(0.2f));
        g.strokePath(valueArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(accentBlue());
        g.strokePath(valueArc, juce::PathStrokeType(4.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Thumb dot
    float thumbX = cx + std::sin(currentAngle) * (radius * 0.85f);
    float thumbY = cy - std::cos(currentAngle) * (radius * 0.85f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbX - 3.5f, thumbY - 3.5f, 7.0f, 7.0f);
}

void OrpheusLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float minPos, float maxPos, juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        float trackY = y + h * 0.5f;
        float trackH = 4.0f;

        // Track background
        g.setColour(backgroundDark());
        g.fillRoundedRectangle((float)x, trackY - trackH / 2, (float)w, trackH, 2.0f);

        // Track fill
        g.setColour(accentBlue().withAlpha(0.7f));
        g.fillRoundedRectangle((float)x, trackY - trackH / 2, sliderPos - x, trackH, 2.0f);

        // Thumb
        g.setColour(accent());
        g.fillEllipse(sliderPos - 7, trackY - 7, 14, 14);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillEllipse(sliderPos - 4, trackY - 4, 8, 8);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        float trackX = x + w * 0.5f;
        float trackW = 6.0f;

        // Fader slot groove (Dark inset)
        g.setColour(backgroundDark().darker());
        g.fillRoundedRectangle(trackX - trackW / 2, (float)y, trackW, (float)h, 3.0f);
        
        // Draw 0dB line (assuming slider range knows about it, usually default handled via lookandfeel ticks)
        // We can draw a subtle tick mark at the center or near top for "0dB" if we assume typical mixer layout.
        // For now, simple sleek cap.
        
        // Fader Cap (Sleek dark grey rectangle with horizontal line)
        float capW = 20.0f;
        float capH = 36.0f;
        juce::Rectangle<float> capBounds(trackX - capW / 2, sliderPos - capH / 2, capW, capH);
        
        g.setColour(juce::Colour(0xff2d2d3e)); // Grey cap body
        g.fillRoundedRectangle(capBounds, 2.0f);
        
        g.setColour(backgroundDark());
        g.drawRoundedRectangle(capBounds, 2.0f, 1.0f);
        
        // Horizontal indicator line on cap
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRect(capBounds.getX() + 2, capBounds.getCentreY() - 1.0f, capW - 4, 2.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos, style, slider);
    }
}

void OrpheusLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool isToggle = button.getToggleState();

    juce::Colour bc = down ? backgroundColour.darker(0.2f)
                           : highlighted ? backgroundColour.brighter(0.1f)
                                         : backgroundColour;
    
    // Provide a subtle glowing outline if toggled
    if (isToggle) {
        g.setColour(accent().withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.expanded(1.0f), 4.0f);
        bc = accent().withAlpha(0.6f);
    }

    g.setColour(bc);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(highlighted || isToggle ? accent().withAlpha(0.8f) : backgroundColour.brighter(0.2f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void OrpheusLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool highlighted, bool /*down*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool toggled = button.getToggleState();

    // LED style button
    g.setColour(backgroundDark().brighter(0.1f));
    g.fillRoundedRectangle(bounds, 4.0f);

    if (toggled)
    {
        // Inner glowing LED depending on text (fallback to accent)
        juce::Colour ledColour = accent();
        if (button.getButtonText().equalsIgnoreCase("S")) ledColour = juce::Colour(0xffffd54f); // Solo Yellow
        if (button.getButtonText().equalsIgnoreCase("M")) ledColour = juce::Colour(0xffe94560); // Mute Red
        if (button.getButtonText().equalsIgnoreCase("R") || button.getButtonText().equalsIgnoreCase("Arm")) ledColour = juce::Colours::red; // Record Red

        // Glow
        g.setColour(ledColour.withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.reduced(1.0f), 3.0f);
        
        g.setColour(ledColour);
        g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.6f));
    }
    else
    {
        g.setColour(backgroundLight());
        g.fillRoundedRectangle(bounds.reduced(2.0f), 3.0f);
        g.setColour(textSecondary());
    }

    g.setFont(defaultFont.withHeight(10.5f).boldened());
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void OrpheusLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
    int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h);
    g.setColour(findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

    // Arrow
    auto arrowZone = bounds.removeFromRight(h).reduced(4);
    juce::Path arrow;
    arrow.startNewSubPath(arrowZone.getCentreX() - 4, arrowZone.getCentreY() - 2);
    arrow.lineTo(arrowZone.getCentreX() + 4, arrowZone.getCentreY() - 2);
    arrow.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 3);
    arrow.closeSubPath();
    g.setColour(findColour(juce::ComboBox::arrowColourId));
    g.fillPath(arrow);
}

void OrpheusLookAndFeel::drawMenuBarBackground(juce::Graphics& g, int w, int h,
    bool, juce::MenuBarComponent&)
{
    g.setColour(backgroundDark());
    g.fillRect(0, 0, w, h);
    g.setColour(backgroundLight());
    g.drawHorizontalLine(h - 1, 0, (float)w);
}

void OrpheusLookAndFeel::drawMenuBarItem(juce::Graphics& g, int w, int h, int,
    const juce::String& text, bool isMouseOver, bool isMenuOpen, bool, juce::MenuBarComponent&)
{
    if (isMouseOver || isMenuOpen)
    {
        g.setColour(accent().withAlpha(0.2f));
        g.fillRoundedRectangle(2, 2, (float)(w - 4), (float)(h - 4), 3.0f);
    }
    g.setColour(isMouseOver || isMenuOpen ? juce::Colours::white : textPrimary());
    g.setFont(defaultFont.withHeight(12.0f));
    g.drawText(text, 0, 0, w, h, juce::Justification::centred);
}

juce::Font OrpheusLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonH)
{
    return defaultFont.withHeight(juce::jmin(12.0f, (float)buttonH * 0.7f));
}

juce::Font OrpheusLookAndFeel::getLabelFont(juce::Label&)
{
    return defaultFont;
}
