#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace zc
{
// The active palette. Presets can switch it (see setPalette); the editor rebuilds its UI when it changes.
inline juce::Colour paper { 0xff09090a }, module { 0xff131315 }, module2 { 0xff161618 }, ink { 0xffecece6 }, sub { 0xff7b7b76 },
                    hair { 0xff26262a }, key { 0xff1f1f23 }, dotOff { 0xff29292d }, red { 0xffff3b30 }, blue { 0xff3d7bff },
                    green { 0xff2fd47a }, orange { 0xffff7a2e }, yellow { 0xffffd23f }, white { 0xfff4f4ef }, cyan { 0xff2ee6d6 },
                    violet { 0xffa17dff }, black { 0xff050505 }, grid { 0xff1a1a1d };
// "default" or "neon" (cyberpunk: teal, hot pink, electric violet on deep indigo)
void setPalette (const juce::String& name);
juce::Font mono (float size, bool bold = false);
juce::Font sans (float size);
inline float textWidth (const juce::Font& f, const juce::String& t) { return juce::GlyphArrangement::getStringWidth (f, t); }

// Draw text as a field of dots (rendered offscreen, then sampled on a grid)
void drawDotMatrixText (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, float fontHeight,
                        juce::Colour colour, float spacing = 3.4f, juce::Justification just = juce::Justification::centredLeft, bool glow = true);
void drawDotGrid (juce::Graphics&, juce::Rectangle<float> area, float spacing, float radius, juce::Colour colour);
void drawModule (juce::Graphics&, juce::Rectangle<float> r, float corner = 16.0f);
}

class ZojoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ZojoLookAndFeel();
    void applyPalette();   // re-read zc:: colours into the JUCE colour IDs
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return zc::mono (10.0f, true); }
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return zc::sans (13.0f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override { return zc::sans (14.0f); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
};
