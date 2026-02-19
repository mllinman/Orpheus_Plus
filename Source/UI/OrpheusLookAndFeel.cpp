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
    float radius = juce::jmin(w, h) * 0.4f;

    // Background circle
    g.setColour(backgroundDark());
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

    // Track arc
    juce::Path trackArc;
    trackArc.addArc(cx - radius, cy - radius, radius * 2, radius * 2,
                    startAngle, endAngle, true);
    g.setColour(backgroundMid().brighter(0.3f));
    g.strokePath(trackArc, juce::PathStrokeType(3.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    float currentAngle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path valueArc;
    valueArc.addArc(cx - radius, cy - radius, radius * 2, radius * 2,
                    startAngle, currentAngle, true);
    g.setColour(accentBlue());
    g.strokePath(valueArc, juce::PathStrokeType(3.0f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Thumb dot
    float thumbX = cx + std::sin(currentAngle) * (radius * 0.75f);
    float thumbY = cy - std::cos(currentAngle) * (radius * 0.75f);
    g.setColour(accent());
    g.fillEllipse(thumbX - 4, thumbY - 4, 8, 8);

    // Centre dot
    g.setColour(backgroundLight());
    g.fillEllipse(cx - 3, cy - 3, 6, 6);
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
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos, style, slider);
    }
}

void OrpheusLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    juce::Colour bc = down ? backgroundColour.darker(0.2f)
                           : highlighted ? backgroundColour.brighter(0.1f)
                                         : backgroundColour;

    g.setColour(bc);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(highlighted ? accent().withAlpha(0.8f)
                             : backgroundColour.brighter(0.2f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void OrpheusLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool highlighted, bool /*down*/)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    bool toggled = button.getToggleState();

    g.setColour(toggled ? accent().withAlpha(0.85f) : backgroundLight());
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(toggled ? accent() : textSecondary());
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    g.setColour(toggled ? juce::Colours::white : textSecondary());
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
