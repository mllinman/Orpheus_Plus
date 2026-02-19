#pragma once
#include <JuceHeader.h>

//==============================================================================
class OrpheusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrpheusLookAndFeel();

    //── Colours ──────────────────────────────────────────────────────────────
    static juce::Colour backgroundDark()   { return juce::Colour(0xff0d0d1a); }
    static juce::Colour backgroundMid()    { return juce::Colour(0xff1a1a2e); }
    static juce::Colour backgroundLight()  { return juce::Colour(0xff16213e); }
    static juce::Colour accent()           { return juce::Colour(0xffe94560); }
    static juce::Colour accentBlue()       { return juce::Colour(0xff4fc3f7); }
    static juce::Colour accentPurple()     { return juce::Colour(0xff7b2d8b); }
    static juce::Colour textPrimary()      { return juce::Colour(0xffeeeeff); }
    static juce::Colour textSecondary()    { return juce::Colour(0xffaaaacc); }

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

    juce::Font getTextButtonFont(juce::TextButton&, int buttonH) override;
    juce::Font getLabelFont(juce::Label&) override;

private:
    juce::Font defaultFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrpheusLookAndFeel)
};
