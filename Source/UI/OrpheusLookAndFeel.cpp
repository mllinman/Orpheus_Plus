#include "OrpheusLookAndFeel.h"

OrpheusLookAndFeel::OrpheusLookAndFeel()
    : defaultFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain))
{
    // ── Background colors ───────────────────────────────────────────────
    setColour(juce::ResizableWindow::backgroundColourId,           bgDarkest());
    setColour(juce::DocumentWindow::backgroundColourId,            bgDarkest());

    // ── Buttons ─────────────────────────────────────────────────────────
    setColour(juce::TextButton::buttonColourId,                    bgElevated());
    setColour(juce::TextButton::buttonOnColourId,                  accentPrimary());
    setColour(juce::TextButton::textColourOffId,                   textSecondary());
    setColour(juce::TextButton::textColourOnId,                    juce::Colours::white);

    // ── Sliders ─────────────────────────────────────────────────────────
    setColour(juce::Slider::backgroundColourId,                    bgDark());
    setColour(juce::Slider::thumbColourId,                         accentPrimary());
    setColour(juce::Slider::trackColourId,                         accentSecondary().withAlpha(0.6f));

    // ── Labels ──────────────────────────────────────────────────────────
    setColour(juce::Label::textColourId,                           textPrimary());

    // ── ComboBox ────────────────────────────────────────────────────────
    setColour(juce::ComboBox::backgroundColourId,                  bgInput());
    setColour(juce::ComboBox::textColourId,                        textPrimary());
    setColour(juce::ComboBox::arrowColourId,                       textMuted());
    setColour(juce::ComboBox::outlineColourId,                     borderDefault());

    // ── PopupMenu ───────────────────────────────────────────────────────
    setColour(juce::PopupMenu::backgroundColourId,                 bgElevated());
    setColour(juce::PopupMenu::textColourId,                       textSecondary());
    setColour(juce::PopupMenu::highlightedBackgroundColourId,      accentPrimary());
    setColour(juce::PopupMenu::highlightedTextColourId,            juce::Colours::white);
    setColour(juce::PopupMenu::headerTextColourId,                 textMuted());

    // ── ScrollBar ───────────────────────────────────────────────────────
    setColour(juce::ScrollBar::thumbColourId,                      bgActive());
    setColour(juce::ScrollBar::backgroundColourId,                 bgDarker());

    // ── ToggleButton ────────────────────────────────────────────────────
    setColour(juce::ToggleButton::textColourId,                    textPrimary());
    setColour(juce::ToggleButton::tickColourId,                    accentPrimary());
    setColour(juce::ToggleButton::tickDisabledColourId,            textMuted());

    // ── AlertWindow ─────────────────────────────────────────────────────
    setColour(juce::AlertWindow::backgroundColourId,               bgSurface());
    setColour(juce::AlertWindow::textColourId,                     textPrimary());
    setColour(juce::AlertWindow::outlineColourId,                  borderDefault());

    // ── TextEditor ──────────────────────────────────────────────────────
    setColour(juce::TextEditor::backgroundColourId,                bgInput());
    setColour(juce::TextEditor::textColourId,                      textPrimary());
    setColour(juce::TextEditor::highlightColourId,                 accentPrimary().withAlpha(0.4f));
    setColour(juce::TextEditor::outlineColourId,                   borderDefault());

    // ── Tooltip ─────────────────────────────────────────────────────────
    setColour(juce::TooltipWindow::backgroundColourId,             bgElevated());
    setColour(juce::TooltipWindow::textColourId,                   textPrimary());
    setColour(juce::TooltipWindow::outlineColourId,                borderDefault());
}

//==============================================================================
// Rotary Slider — Skeuomorphic Glassy Knob
//==============================================================================
void OrpheusLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    float cx = x + w * 0.5f, cy = y + h * 0.5f;
    float radius = juce::jmin(w, h) * 0.38f;
    float currentAngle = startAngle + sliderPos * (endAngle - startAngle);

    // Drop Shadow
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillEllipse(cx - radius, cy - radius + 2.0f, radius * 2, radius * 2);

    // Knob Base (Dark Metallic/Glass)
    juce::ColourGradient baseGrad(juce::Colour(0xff2a2a3e), cx, cy - radius,
                                  juce::Colour(0xff12121e), cx, cy + radius, false);
    g.setGradientFill(baseGrad);
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

    // Inner Bevel / Rim
    juce::ColourGradient rimGrad(juce::Colours::white.withAlpha(0.15f), cx, cy - radius,
                                 juce::Colours::black.withAlpha(0.4f), cx, cy + radius, false);
    g.setGradientFill(rimGrad);
    g.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2, 1.5f);

    // LED Ring / Arc Track (Deep groove)
    float arcRadius = radius + 6.0f;
    juce::Path trackArc;
    trackArc.addArc(cx - arcRadius, cy - arcRadius, arcRadius * 2, arcRadius * 2, startAngle, endAngle, true);
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.strokePath(trackArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // LED Value Arc (Glowing)
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addArc(cx - arcRadius, cy - arcRadius, arcRadius * 2, arcRadius * 2, startAngle, currentAngle, true);
        
        g.setColour(accentPrimary().withAlpha(0.3f));
        g.strokePath(valueArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(accentPrimary());
        g.strokePath(valueArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Specular Highlight (Glass reflection on top edge)
    juce::Path highlight;
    highlight.addArc(cx - radius + 2, cy - radius + 2, radius * 2 - 4, radius * 2 - 4, 
                     startAngle - 0.5f, startAngle + 1.5f, true);
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.strokePath(highlight, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Indicator Line (Skeuomorphic indent with glow)
    float indLength = radius * 0.6f;
    float indX = cx + std::sin(currentAngle) * indLength;
    float indY = cy - std::cos(currentAngle) * indLength;
    
    // Indent shadow
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawLine(cx, cy, indX, indY + 1.0f, 3.0f);
    
    // Indicator fill
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.drawLine(cx, cy, indX, indY, 2.0f);
}

//==============================================================================
// Linear Slider — horizontal track or vertical fader
//==============================================================================
//==============================================================================
// Linear Slider — Skeuomorphic Fader
//==============================================================================
void OrpheusLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float minPos, float maxPos, juce::Slider::SliderStyle style,
    juce::Slider& slider)
{
    if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearBar)
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
        float trackY = bounds.getCentreY();
        float trackH = 6.0f;

        // Track Groove (Inset shadow)
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(bounds.getX(), trackY - trackH / 2, bounds.getWidth(), trackH, 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(bounds.getX(), trackY - trackH / 2, bounds.getWidth(), trackH, 3.0f, 1.0f);

        // Track fill (LED strip)
        float fillW = sliderPos - bounds.getX();
        if (fillW > 0)
        {
            auto fillBounds = juce::Rectangle<float>(bounds.getX() + 1, trackY - trackH / 2 + 1, fillW - 2, trackH - 2);
            g.setGradientFill(juce::ColourGradient(accentPrimary(), fillBounds.getX(), 0,
                                                   accentSecondary(), fillBounds.getRight(), 0, false));
            g.fillRoundedRectangle(fillBounds, 2.0f);
        }

        // Thumb Cap (3D Glassy)
        auto thumbBounds = juce::Rectangle<float>(sliderPos - 10, trackY - 12, 20, 24);
        
        // Shadow
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(thumbBounds.translated(0, 2), 4.0f);

        // Gradient body
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a4e), 0, thumbBounds.getY(),
                                               juce::Colour(0xff1f1f2e), 0, thumbBounds.getBottom(), false));
        g.fillRoundedRectangle(thumbBounds, 4.0f);

        // Highlight
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawRoundedRectangle(thumbBounds, 4.0f, 1.0f);
        
        // Indicator Line
        g.setColour(juce::Colours::white);
        g.fillRect(sliderPos - 1.0f, thumbBounds.getY() + 4.0f, 2.0f, thumbBounds.getHeight() - 8.0f);
    }
    else if (style == juce::Slider::LinearVertical)
    {
        float trackX = x + w * 0.5f;
        float trackW = 8.0f;

        // Fader slot groove (deep shadow)
        g.setColour(juce::Colours::black.withAlpha(0.9f));
        g.fillRoundedRectangle(trackX - trackW / 2, (float)y, trackW, (float)h, 4.0f);
        // Inner rim light
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(trackX - trackW / 2, (float)y, trackW, (float)h, 4.0f, 1.0f);

        // Fader Cap (Skeuomorphic console style)
        float capW = 26.0f;
        float capH = 42.0f;
        juce::Rectangle<float> capBounds(trackX - capW / 2, sliderPos - capH / 2, capW, capH);
        
        // Drop shadow
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(capBounds.translated(0, 3.0f), 3.0f);
        
        // Gradient Body
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4a5e), capBounds.getX(), capBounds.getY(),
                                               juce::Colour(0xff12121e), capBounds.getRight(), capBounds.getY(), false));
        g.fillRoundedRectangle(capBounds, 3.0f);
        
        // Concave finger indent
        auto indentBounds = capBounds.reduced(4.0f, 8.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff12121e), indentBounds.getX(), indentBounds.getY(),
                                               juce::Colour(0xff2a2a3e), indentBounds.getX(), indentBounds.getBottom(), false));
        g.fillRoundedRectangle(indentBounds, 2.0f);

        // Horizontal indicator line (glowing LED strip)
        g.setColour(juce::Colours::white);
        g.fillRect(capBounds.getX() + 2, capBounds.getCentreY() - 1.0f, capW - 4, 2.0f);
        
        g.setColour(accentPrimary().withAlpha(0.5f));
        g.fillRect(capBounds.getX() + 2, capBounds.getCentreY() - 2.0f, capW - 4, 4.0f);
    }
    else
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minPos, maxPos, style, slider);
    }
}

//==============================================================================
// Button Background — Glassy/3D Buttons
//==============================================================================
void OrpheusLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour, bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    bool isToggle = button.getToggleState();
    float cornerSize = 4.0f;

    // Drop Shadow
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(bounds.translated(0, 1.5f), cornerSize);

    juce::Colour baseColor = isToggle ? accentPrimary().darker(0.1f) : backgroundColour;
    if (down) baseColor = baseColor.darker(0.3f);
    else if (highlighted) baseColor = baseColor.brighter(0.15f);

    // Main Gradient (Glassy Convex)
    juce::ColourGradient grad(baseColor.brighter(0.2f), 0, bounds.getY(),
                              baseColor.darker(0.3f), 0, bounds.getBottom(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, cornerSize);

    // Inner top highlight (Glass reflection)
    if (!down)
    {
        g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.15f), 0, bounds.getY(),
                                               juce::Colours::transparentWhite, 0, bounds.getY() + bounds.getHeight() * 0.4f, false));
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    // Border
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    
    // Bottom Rim highlight
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerSize - 1.0f, 1.0f);
}

//==============================================================================
// Toggle Button — 3D Hardware LED Button (M/S/R)
//==============================================================================
void OrpheusLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
    bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    bool toggled = button.getToggleState();

    juce::Colour ledColour = accentPrimary();
    auto text = button.getButtonText().toLowerCase();
    if (text == "s")    ledColour = juce::Colour(0xffffca28); // Vibrant yellow
    if (text == "m")    ledColour = juce::Colour(0xfff44336); // Red
    if (text == "r" || text == "arm") ledColour = juce::Colour(0xffd32f2f);

    // Drop Shadow
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(bounds.translated(0, 1.0f), 3.0f);

    // Button Body
    juce::Colour bodyColour = down ? bgDarkest() : (highlighted ? bgDarker().brighter(0.1f) : bgDarker());
    g.setGradientFill(juce::ColourGradient(bodyColour.brighter(0.1f), 0, bounds.getY(),
                                           bodyColour.darker(0.2f), 0, bounds.getBottom(), false));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Outer Bezel
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // LED State
    if (toggled)
    {
        // Bright LED glow
        g.setColour(ledColour.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.reduced(1.5f), 2.0f);
        
        g.setColour(ledColour);
        g.drawRoundedRectangle(bounds.reduced(1.5f), 2.0f, 1.5f);
        g.setColour(juce::Colours::black.withAlpha(0.8f)); // Text dark inside bright LED
    }
    else
    {
        // Off State
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.reduced(1.5f), 2.0f);
        g.setColour(textMuted()); // Text dim
    }

    g.setFont(defaultFont.withHeight(11.0f).boldened());
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

//==============================================================================
// ComboBox
//==============================================================================
void OrpheusLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
    int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(0.5f);
    float cornerSize = bounds.getHeight() * 0.5f; // Pill-shaped

    drawGlassBackground(g, bounds, cornerSize);
    
    g.setColour(borderDefault().withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

    // Arrow
    auto arrowZone = bounds.removeFromRight(h).reduced(h * 0.3f);
    juce::Path arrow;
    arrow.startNewSubPath(arrowZone.getCentreX() - 3.5f, arrowZone.getCentreY() - 2);
    arrow.lineTo(arrowZone.getCentreX() + 3.5f, arrowZone.getCentreY() - 2);
    arrow.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 3);
    arrow.closeSubPath();
    g.setColour(textSecondary());
    g.fillPath(arrow);
}

//==============================================================================
// Menu Bar — gradient from bgSurface to bgPanel
//==============================================================================
void OrpheusLookAndFeel::drawMenuBarBackground(juce::Graphics& g, int w, int h,
    bool, juce::MenuBarComponent&)
{
    // Gradient matching .top-menu-bar: linear-gradient(180deg, #1a1a2e 0%, #12121e 100%)
    g.setGradientFill(juce::ColourGradient(bgSurface(), 0, 0, bgPanel(), 0, (float)h, false));
    g.fillRect(0, 0, w, h);
    g.setColour(borderSubtle());
    g.drawHorizontalLine(h - 1, 0, (float)w);
}

void OrpheusLookAndFeel::drawMenuBarItem(juce::Graphics& g, int w, int h, int,
    const juce::String& text, bool isMouseOver, bool isMenuOpen, bool, juce::MenuBarComponent&)
{
    if (isMouseOver || isMenuOpen)
    {
        g.setColour(bgHover());
        g.fillRoundedRectangle(2.0f, 2.0f, (float)(w - 4), (float)(h - 4), 4.0f);
    }
    g.setColour(isMouseOver || isMenuOpen ? textPrimary() : textSecondary());
    g.setFont(defaultFont.withHeight(12.0f));
    g.drawText(text, 0, 0, w, h, juce::Justification::centred);
}

//==============================================================================
// Popup Menu — matching .dropdown-menu
//==============================================================================
void OrpheusLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h)
{
    g.setColour(bgElevated());
    g.fillRoundedRectangle(0, 0, (float)w, (float)h, 6.0f);
    g.setColour(borderDefault());
    g.drawRoundedRectangle(0.5f, 0.5f, w - 1.0f, h - 1.0f, 6.0f, 1.0f);
}

void OrpheusLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
    bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu,
    const juce::String& text, const juce::String& shortcutKeyText,
    const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(borderDefault());
        g.fillRect(area.reduced(8, 0).withHeight(1).withCentre(area.getCentre()));
        return;
    }

    if (isHighlighted && isActive)
    {
        g.setColour(accentPrimary());
        g.fillRoundedRectangle(area.toFloat().reduced(4, 1), 4.0f);
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(isActive ? textSecondary() : textDisabled());
    }

    auto textArea = area.reduced(12, 0);
    g.setFont(defaultFont.withHeight(12.5f));
    g.drawText(text, textArea, juce::Justification::centredLeft);

    if (shortcutKeyText.isNotEmpty())
    {
        g.setColour(isHighlighted ? juce::Colours::white.withAlpha(0.6f) : textMuted());
        g.setFont(defaultFont.withHeight(11.0f));
        g.drawText(shortcutKeyText, textArea, juce::Justification::centredRight);
    }

    if (isTicked)
    {
        auto tickArea = area.withWidth(24);
        g.setColour(isHighlighted ? juce::Colours::white : accentPrimary());
        g.setFont(defaultFont.withHeight(14.0f));
        g.drawText(juce::String::charToString(0x2713), tickArea, juce::Justification::centred);
    }

    if (hasSubMenu)
    {
        auto arrowArea = area.withLeft(area.getRight() - 16).reduced(4);
        juce::Path arrow;
        arrow.startNewSubPath((float)arrowArea.getX(), (float)arrowArea.getY());
        arrow.lineTo((float)arrowArea.getRight(), (float)arrowArea.getCentreY());
        arrow.lineTo((float)arrowArea.getX(), (float)arrowArea.getBottom());
        arrow.closeSubPath();
        g.setColour(isHighlighted ? juce::Colours::white : textMuted());
        g.fillPath(arrow);
    }
}

//==============================================================================
// Scrollbar — thin and sleek
//==============================================================================
void OrpheusLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar&,
    int x, int y, int w, int h, bool isScrollbarVertical,
    int thumbStart, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    g.setColour(bgDarker());
    g.fillRect(x, y, w, h);

    juce::Colour thumbC = isMouseDown ? borderStrong()
                        : isMouseOver ? borderDefault()
                        : bgActive();
    
    if (isScrollbarVertical)
    {
        int thumbW = juce::jmax(4, w - 4);
        g.setColour(thumbC);
        g.fillRoundedRectangle((float)(x + (w - thumbW) / 2), (float)(y + thumbStart),
                                (float)thumbW, (float)thumbSize, 4.0f);
    }
    else
    {
        int thumbH = juce::jmax(4, h - 4);
        g.setColour(thumbC);
        g.fillRoundedRectangle((float)(x + thumbStart), (float)(y + (h - thumbH) / 2),
                                (float)thumbSize, (float)thumbH, 4.0f);
    }
}

//==============================================================================
// Fonts
//==============================================================================
juce::Font OrpheusLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonH)
{
    return defaultFont.withHeight(juce::jmin(12.0f, (float)buttonH * 0.65f));
}

juce::Font OrpheusLookAndFeel::getLabelFont(juce::Label&)
{
    return defaultFont;
}

juce::Font OrpheusLookAndFeel::getPopupMenuFont()
{
    return defaultFont.withHeight(12.5f);
}

//==============================================================================
// Glassmorphism & Custom Components
//==============================================================================

void OrpheusLookAndFeel::drawGlassBackground(juce::Graphics& g, const juce::Rectangle<float>& area, 
                                             float cornerSize, float alpha)
{
    // High-end glassmorphism effect
    g.setColour(bgSurface().withAlpha(0.4f * alpha));
    g.fillRoundedRectangle(area, cornerSize);
    
    // Diagonal glass reflection gradient
    g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(0.15f * alpha), area.getX(), area.getY(),
                                           juce::Colours::transparentWhite, area.getBottomRight().getX(), area.getBottomRight().getY(), false));
    g.fillRoundedRectangle(area, cornerSize);
    
    // Inner white rim (thin frosted glass edge)
    g.setColour(juce::Colours::white.withAlpha(0.08f * alpha));
    g.drawRoundedRectangle(area.reduced(0.5f), cornerSize, 1.0f);
    
    // Outer drop shadow rim (dark)
    g.setColour(juce::Colours::black.withAlpha(0.3f * alpha));
    g.drawRoundedRectangle(area.expanded(0.5f), cornerSize, 1.0f);
}

void OrpheusLookAndFeel::drawToolbarBackground(juce::Graphics& g, int w, int h)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h);
    
    g.setGradientFill(juce::ColourGradient(bgSurface(), 0, 0, 
                                           bgDarkest(), 0, (float)h, false));
    g.fillRect(bounds);
    
    g.setColour(borderSubtle());
    g.drawHorizontalLine(h - 1, 0, (float)w);
}

void OrpheusLookAndFeel::drawTabButton(juce::Graphics& g, int w, int h, const juce::Colour& backgroundColour,
                                       bool isMouseOver, bool isMouseDown, bool isFront,
                                       const juce::String& text, int /*tabIndex*/)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h).reduced(1, 0);
    float cornerSize = 4.0f;

    // Use backgroundColour as category accent (passed via addTab)
    juce::Colour catColour = backgroundColour.getBrightness() > 0.15f
                           ? backgroundColour
                           : accentPrimary();

    if (isFront)
    {
        // Active tab — dark background with coloured underline
        g.setColour(bgSurface());
        g.fillRoundedRectangle(bounds, cornerSize);

        // Coloured indicator line at bottom
        g.setColour(catColour);
        g.fillRect(bounds.getX() + 3, bounds.getBottom() - 2.5f, bounds.getWidth() - 6, 2.5f);

        // Subtle category glow
        g.setColour(catColour.withAlpha(0.08f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    else if (isMouseOver || isMouseDown)
    {
        g.setColour(bgElevated().withAlpha(0.5f));
        g.fillRoundedRectangle(bounds, cornerSize);

        // Subtle colour hint on hover
        g.setColour(catColour.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    // Tab text — coloured when active, muted otherwise
    float textEndX = (float)w;

    // Close × button (shown on hover or active)
    if (isFront || isMouseOver)
    {
        float closeSize = 12.0f;
        float closeX = bounds.getRight() - closeSize - 4.0f;
        float closeY = bounds.getCentreY() - closeSize / 2.0f;
        auto closeArea = juce::Rectangle<float>(closeX, closeY, closeSize, closeSize);

        // Draw × symbol
        g.setColour((isMouseOver && !isFront) ? textMuted() : catColour.withAlpha(0.5f));
        g.setFont(defaultFont.withHeight(10.0f));
        g.drawText(juce::String::charToString(0x2715), closeArea, juce::Justification::centred);

        textEndX = closeX - 2.0f;
    }

    if (isFront)
    {
        g.setColour(catColour.brighter(0.3f));
        g.setFont(defaultFont.withHeight(11.5f).boldened());
    }
    else if (isMouseOver)
    {
        g.setColour(textPrimary());
        g.setFont(defaultFont.withHeight(11.5f));
    }
    else
    {
        g.setColour(textMuted());
        g.setFont(defaultFont.withHeight(11.0f));
    }

    // Small coloured dot before text for category identification
    if (isFront || isMouseOver)
    {
        float dotR = 3.0f;
        float dotX = bounds.getX() + 6.0f;
        float dotY = bounds.getCentreY() - dotR;
        g.setColour(catColour);
        g.fillEllipse(dotX, dotY, dotR * 2, dotR * 2);

        g.drawText(text, juce::Rectangle<float>(dotX + dotR * 2 + 3, 0, textEndX - dotX - dotR * 2 - 6, (float)h),
                   juce::Justification::centredLeft, true);
    }
    else
    {
        g.drawText(text, 0, 0, w, h, juce::Justification::centred, true);
    }
}




