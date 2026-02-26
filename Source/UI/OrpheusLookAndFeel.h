#pragma once
#include <JuceHeader.h>

//==============================================================================
// Orpheus DAW Design Tokens — matching the legacy web CSS variables.css
//==============================================================================
class OrpheusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrpheusLookAndFeel();

    /** Load a named theme: "Dark" (default), "Light", or "Midnight". */
    void loadTheme(const juce::String& themeName)
    {
        currentThemeName_ = themeName;
        if (themeName == "Light")
        {
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xfff0f0f5));
            setColour(juce::TextButton::buttonColourId,          juce::Colour(0xffe0e0e8));
            setColour(juce::TextButton::textColourOffId,         juce::Colour(0xff1a1a2e));
            setColour(juce::Label::textColourId,                 juce::Colour(0xff1a1a2e));
        }
        else if (themeName == "Midnight")
        {
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff05050a));
            setColour(juce::TextButton::buttonColourId,          juce::Colour(0xff0a0a18));
            setColour(juce::TextButton::textColourOffId,         juce::Colour(0xff8888cc));
            setColour(juce::Label::textColourId,                 juce::Colour(0xff8888cc));
        }
        else // "Dark" (default)
        {
            setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0a0f));
            setColour(juce::TextButton::buttonColourId,          juce::Colour(0xff1a1a2e));
            setColour(juce::TextButton::textColourOffId,         juce::Colour(0xffe8e8f0));
            setColour(juce::Label::textColourId,                 juce::Colour(0xffe8e8f0));
        }
    }

    juce::String getCurrentTheme() const { return currentThemeName_; }
    //── Background Layers (from --bg-*) ──────────────────────────────────────
    static juce::Colour bgDarkest()    { return juce::Colour(0xff09090b); }
    static juce::Colour bgDarker()     { return juce::Colour(0xff18181b); }
    static juce::Colour bgDark()       { return juce::Colour(0xff27272a); }
    static juce::Colour bgSurface()    { return juce::Colour(0xff18181b); }
    static juce::Colour bgElevated()   { return juce::Colour(0xff27272a); }
    static juce::Colour bgHover()      { return juce::Colour(0xff3f3f46); }
    static juce::Colour bgActive()     { return juce::Colour(0xff52525b); }
    static juce::Colour bgPanel()      { return juce::Colour(0xff09090b); }
    static juce::Colour bgInput()      { return juce::Colour(0xff09090b); }
    static juce::Colour bgTrackOdd()   { return juce::Colour(0xff18181b); }
    static juce::Colour bgTrackEven()  { return juce::Colour(0xff27272a); }
    static juce::Colour bgClip()       { return juce::Colour(0xff3f3f46); }

    //── Accent Colors (from --accent-*) ──────────────────────────────────────
    static juce::Colour accentPrimary()   { return juce::Colour(0xff8b5cf6); }  // Violet 500
    static juce::Colour accentPrimaryL()  { return juce::Colour(0xffa78bfa); }  // Violet 400
    static juce::Colour accentSecondary() { return juce::Colour(0xff06b6d4); }  // Cyan 500
    static juce::Colour accentTertiary()  { return juce::Colour(0xffec4899); }  // Pink 500
    static juce::Colour accentWarning()   { return juce::Colour(0xfff59e0b); }  // Amber 500
    static juce::Colour accentDanger()    { return juce::Colour(0xffef4444); }  // Red 500
    static juce::Colour accentSuccess()   { return juce::Colour(0xff10b981); }  // Emerald 500
    static juce::Colour accentInfo()      { return juce::Colour(0xff3b82f6); }  // Blue 500

    //── Text Colors (from --text-*) ──────────────────────────────────────────
    static juce::Colour textPrimary()   { return juce::Colour(0xfffafafa); } // Zinc 50
    static juce::Colour textSecondary() { return juce::Colour(0xffa1a1aa); } // Zinc 400
    static juce::Colour textMuted()     { return juce::Colour(0xff71717a); } // Zinc 500
    static juce::Colour textDisabled()  { return juce::Colour(0xff52525b); } // Zinc 600

    //── Border Colors (from --border-*) ──────────────────────────────────────
    static juce::Colour borderSubtle()  { return juce::Colour(0xff27272a); }
    static juce::Colour borderDefault() { return juce::Colour(0xff3f3f46); }
    static juce::Colour borderStrong()  { return juce::Colour(0xff52525b); }

    //── Track Colors ─────────────────────────────────────────────────────────
    static juce::Colour trackBlue()    { return juce::Colour(0xff4a90d9); }
    static juce::Colour trackPurple()  { return juce::Colour(0xff9b59b6); }
    static juce::Colour trackGreen()   { return juce::Colour(0xff27ae60); }
    static juce::Colour trackOrange()  { return juce::Colour(0xffe67e22); }
    static juce::Colour trackRed()     { return juce::Colour(0xffe74c3c); }
    static juce::Colour trackCyan()    { return juce::Colour(0xff00bcd4); }
    static juce::Colour trackPink()    { return juce::Colour(0xffe91e8a); }
    static juce::Colour trackIndigo()  { return juce::Colour(0xff5c6bc0); }

    // Legacy API compat
    static juce::Colour backgroundDark()   { return bgDarkest(); }
    static juce::Colour backgroundMid()    { return bgSurface(); }
    static juce::Colour backgroundLight()  { return bgElevated(); }
    static juce::Colour accent()           { return accentPrimary(); }
    static juce::Colour accentBlue()       { return accentSecondary(); }
    static juce::Colour accentPurple()     { return accentPrimaryL(); }

    //── Override LookAndFeel methods ─────────────────────────────────────────
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override;

    void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics&, juce::ToggleButton&,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int w, int h, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox&) override;

    void drawMenuBarBackground(juce::Graphics&, int w, int h,
                                bool isMouseOverBar, juce::MenuBarComponent&) override;

    void drawMenuBarItem(juce::Graphics&, int w, int h, int itemIndex,
                         const juce::String& itemText, bool isMouseOverItem,
                         bool isMenuOpen, bool isMouseOverBar,
                         juce::MenuBarComponent&) override;

    void drawPopupMenuBackground(juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text, const juce::String& shortcutKeyText,
                           const juce::Drawable* icon, const juce::Colour* textColour) override;

    void drawScrollbar(juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h,
                       bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

    //── Custom Orpheus Methods ──────────────────────────────────────────────
    void drawTabButton(juce::Graphics& g, int w, int h, const juce::Colour& backgroundColour,
                       bool isMouseOver, bool isMouseDown, bool isFront,
                       const juce::String& text, int tabIndex);
    
    void drawToolbarBackground(juce::Graphics& g, int w, int h);
    void drawGlassBackground(juce::Graphics& g, const juce::Rectangle<float>& area, 
                             float cornerSize, float alpha = 1.0f);

    juce::Font getTextButtonFont(juce::TextButton&, int buttonH) override;
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getPopupMenuFont() override;

private:
    juce::Font defaultFont;
    juce::String currentThemeName_ = "Dark";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrpheusLookAndFeel)
};
