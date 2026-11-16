#include "ZojoLookAndFeel.h"
#include "BinaryData.h"

namespace
{
juce::Typeface::Ptr tfMono, tfMonoBold, tfSans;
void loadTypefaces()
{
    if (tfMono != nullptr) return;
    tfMono = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
    tfMonoBold = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    tfSans = juce::Typeface::createSystemTypefaceFor (BinaryData::SpaceGrotesk_ttf, BinaryData::SpaceGrotesk_ttfSize);
}
}

juce::Font zc::mono (float size, bool bold)
{
    loadTypefaces();
    return juce::Font (juce::FontOptions (bold ? tfMonoBold : tfMono).withHeight (size));
}
juce::Font zc::sans (float size)
{
    loadTypefaces();
    return juce::Font (juce::FontOptions (tfSans).withHeight (size));
}

void zc::drawDotGrid (juce::Graphics& g, juce::Rectangle<float> a, float s, float r, juce::Colour c)
{
    g.setColour (c);
    for (float y = a.getY() + s * 0.5f; y < a.getBottom(); y += s)
        for (float x = a.getX() + s * 0.5f; x < a.getRight(); x += s)
            g.fillEllipse (x - r, y - r, r * 2, r * 2);
}

void zc::drawModule (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    g.setGradientFill (juce::ColourGradient (module2.withAlpha (0.95f), r.getX(), r.getY(), module.withAlpha (0.95f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setColour (hair); g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f)); g.drawLine (r.getX() + corner, r.getY() + 1, r.getRight() - corner, r.getY() + 1, 1.0f);
}

void zc::drawDotMatrixText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, float fontHeight,
                            juce::Colour colour, float spacing, juce::Justification just, bool glow)
{
    const int W = (int) std::ceil (area.getWidth()), H = (int) std::ceil (area.getHeight());
    if (W <= 0 || H <= 0) return;
    juce::Image img (juce::Image::SingleChannel, W, H, true);
    {
        juce::Graphics ig (img);
        ig.setColour (juce::Colours::white);
        ig.setFont (mono (fontHeight, true).withExtraKerningFactor (-0.04f));
        ig.drawText (text, 0, 0, W, H, just, false);
    }
    const float r = spacing * 0.38f;
    juce::Path dots;
    for (float y = spacing * 0.5f; y < (float) H; y += spacing)
        for (float x = spacing * 0.5f; x < (float) W; x += spacing)
            if (img.getPixelAt ((int) x, (int) y).getAlpha() > 110)
                dots.addEllipse (area.getX() + x - r, area.getY() + y - r, r * 2, r * 2);
    if (glow)
    {
        g.setColour (colour.withAlpha (0.18f));
        g.fillPath (dots, juce::AffineTransform::translation (0, 0));
        juce::DropShadow (colour.withAlpha (0.45f), (int) (spacing * 3), {}).drawForPath (g, dots);
    }
    g.setColour (colour); g.fillPath (dots);
}

//==============================================================================
void zc::setPalette (const juce::String& name)
{
    if (name == "neon")
    {
        paper = juce::Colour (0xff0a0820); module = juce::Colour (0xff141039); module2 = juce::Colour (0xff181343);
        ink = juce::Colour (0xffeafcff); sub = juce::Colour (0xff8f8ad0); hair = juce::Colour (0xff2e2868);
        key = juce::Colour (0xff221c56); dotOff = juce::Colour (0xff2c2668); grid = juce::Colour (0xff1c1848);
        red = juce::Colour (0xffff2e93); blue = juce::Colour (0xff5b6bff); green = juce::Colour (0xff22f0cf);
        orange = juce::Colour (0xffff6ad5); yellow = juce::Colour (0xfffff15c); white = juce::Colour (0xfff2fbff);
        cyan = juce::Colour (0xff00f0ff); violet = juce::Colour (0xffb65cff);
    }
    else
    {
        paper = juce::Colour (0xff09090a); module = juce::Colour (0xff131315); module2 = juce::Colour (0xff161618);
        ink = juce::Colour (0xffecece6); sub = juce::Colour (0xff7b7b76); hair = juce::Colour (0xff26262a);
        key = juce::Colour (0xff1f1f23); dotOff = juce::Colour (0xff29292d); grid = juce::Colour (0xff1a1a1d);
        red = juce::Colour (0xffff3b30); blue = juce::Colour (0xff3d7bff); green = juce::Colour (0xff2fd47a);
        orange = juce::Colour (0xffff7a2e); yellow = juce::Colour (0xffffd23f); white = juce::Colour (0xfff4f4ef);
        cyan = juce::Colour (0xff2ee6d6); violet = juce::Colour (0xffa17dff);
    }
}

ZojoLookAndFeel::ZojoLookAndFeel() { applyPalette(); }

void ZojoLookAndFeel::applyPalette()
{
    setColour (juce::ResizableWindow::backgroundColourId, zc::paper);
    setColour (juce::PopupMenu::backgroundColourId, zc::module);
    setColour (juce::PopupMenu::textColourId, zc::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, zc::key);
    setColour (juce::PopupMenu::highlightedTextColourId, zc::white);
    setColour (juce::PopupMenu::headerTextColourId, zc::red);
    setColour (juce::ComboBox::textColourId, zc::ink);
    setColour (juce::ComboBox::backgroundColourId, zc::key);
    setColour (juce::ComboBox::outlineColourId, zc::hair);
    setColour (juce::ComboBox::arrowColourId, zc::ink);
    setColour (juce::TextButton::textColourOffId, zc::sub);
    setColour (juce::TextButton::textColourOnId, zc::paper);
    setColour (juce::Label::textColourId, zc::ink);
    setColour (juce::AlertWindow::backgroundColourId, zc::module);
    setColour (juce::AlertWindow::textColourId, zc::ink);
    setColour (juce::AlertWindow::outlineColourId, zc::hair);
    setColour (juce::TextEditor::backgroundColourId, zc::paper);
    setColour (juce::TextEditor::textColourId, zc::ink);
    setColour (juce::TextEditor::outlineColourId, zc::hair);
    setColour (juce::TextEditor::focusedOutlineColourId, zc::red);
    setColour (juce::ScrollBar::thumbColourId, zc::hair);
    setColour (juce::DocumentWindow::backgroundColourId, zc::paper);
}

juce::Typeface::Ptr ZojoLookAndFeel::getTypefaceForFont (const juce::Font& f)
{
    loadTypefaces();
    if (f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName()) return tfSans;
    if (f.getTypefaceName() == juce::Font::getDefaultMonospacedFontName()) return tfMono;
    return LookAndFeel_V4::getTypefaceForFont (f);
}

void ZojoLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider& s)
{
    const auto cap = juce::Colour ((juce::uint32) (juce::int64) s.getProperties().getWithDefault ("cap", (juce::int64) zc::white.getARGB()));
    const auto ind = juce::Colour ((juce::uint32) (juce::int64) s.getProperties().getWithDefault ("ind", (juce::int64) 0xff0b0b0c));
    const auto off = juce::Colour ((juce::uint32) (juce::int64) s.getProperties().getWithDefault ("off", (juce::int64) zc::dotOff.getARGB()));
    const bool bipolar = s.getProperties().getWithDefault ("bipolar", false);
    const float glowAmt = (float) s.getProperties().getWithDefault ("glow", 1.0);

    const float size = (float) std::min (w, h), cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
    const float ringR = size * 0.46f, capR = size * 0.31f, dotR = size * 0.03f;
    constexpr int N = 23;
    const float a0 = -juce::MathConstants<float>::pi * 0.75f, span = juce::MathConstants<float>::pi * 1.5f;

    juce::Path lit, unlit;
    for (int i = 0; i < N; ++i)
    {
        const float t = (float) i / (N - 1), a = a0 + t * span;
        const float px = cx + ringR * std::sin (a), py = cy - ringR * std::cos (a);
        const bool on = bipolar ? (t >= std::min (pos, 0.5f) - 1e-4f && t <= std::max (pos, 0.5f) + 1e-4f) : (t <= pos + 1e-4f);
        (on ? lit : unlit).addEllipse (px - dotR, py - dotR, dotR * 2, dotR * 2);
    }
    g.setColour (off); g.fillPath (unlit);
    if (glowAmt > 0) juce::DropShadow (cap.withAlpha (0.75f * glowAmt), (int) (size * 0.09f), {}).drawForPath (g, lit);
    g.setColour (cap.withMultipliedAlpha (0.55f + 0.45f * glowAmt)); g.fillPath (lit);

    juce::Path capPath; capPath.addEllipse (cx - capR, cy - capR, capR * 2, capR * 2);
    if (glowAmt > 0) juce::DropShadow (cap.withAlpha (0.55f * glowAmt), (int) (size * 0.16f), {}).drawForPath (g, capPath);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), (int) (size * 0.05f), { 0, (int) (size * 0.03f) }).drawForPath (g, capPath);
    g.setColour (cap); g.fillPath (capPath);
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.drawEllipse (cx - capR + 1, cy - capR + 1, capR * 2 - 2, capR * 2 - 2, 1.0f);

    const float ang = a0 + pos * span;
    juce::Path line; line.startNewSubPath (0, -capR * 0.3f); line.lineTo (0, -capR * 0.88f);
    g.setColour (ind);
    g.strokePath (line, juce::PathStrokeType (size * 0.045f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::rotation (ang).translated (cx, cy));
    if (s.hasKeyboardFocus (false)) { g.setColour (zc::white.withAlpha (0.6f)); g.drawEllipse (cx - capR, cy - capR, capR * 2, capR * 2, 1.2f); }
}

void ZojoLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool chip = b.getProperties().getWithDefault ("chip", false);
    const auto accent = juce::Colour ((juce::uint32) (juce::int64) b.getProperties().getWithDefault ("accent", (juce::int64) zc::ink.getARGB()));
    const float corner = chip ? r.getHeight() * 0.5f : 9.0f;
    if (down) r.translate (0, 1);

    if (on && (bool) b.getProperties().getWithDefault ("softOn", false))
    {
        juce::Path p; p.addRoundedRectangle (r, corner);
        juce::DropShadow (accent.withAlpha (0.45f), 16, {}).drawForPath (g, p);
        g.setColour (juce::Colour (0xff141416)); g.fillPath (p);
        g.setColour (accent.withAlpha (0.16f)); g.fillPath (p);
        g.setColour (accent.withAlpha (0.8f)); g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    else if (on)
    {
        juce::Path p; p.addRoundedRectangle (r, corner);
        juce::DropShadow (accent.withAlpha (0.55f), 14, {}).drawForPath (g, p);
        g.setColour (accent); g.fillPath (p);
    }
    else if (chip)
    {
        g.setColour (hi ? zc::key.brighter (0.15f) : juce::Colours::transparentBlack); g.fillRoundedRectangle (r, corner);
        g.setColour (zc::hair.brighter (hi ? 0.4f : 0.1f)); g.drawRoundedRectangle (r, corner, 1.0f);
    }
    else
    {
        g.setColour (hi ? zc::key.brighter (0.12f) : zc::key); g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.withTop (r.getBottom() - (down ? 1.5f : 3.5f)), corner * 0.6f);
        g.setColour (juce::Colours::white.withAlpha (0.05f)); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + corner, r.getRight() - corner);
    }
}

void ZojoLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool hi, bool)
{
    const bool on = b.getToggleState();
    const auto onText = juce::Colour ((juce::uint32) (juce::int64) b.getProperties().getWithDefault ("onText", (juce::int64) zc::paper.getARGB()));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (on ? onText : (hi ? zc::ink : zc::sub));
    const juce::String t = b.getButtonText().toUpperCase();
    g.drawText (t, b.getLocalBounds().reduced (4, 0), juce::Justification::centred, true);
}

void ZojoLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (zc::key); g.fillRoundedRectangle (r, 8.0f);
    g.setColour (box.hasKeyboardFocus (true) ? zc::red.withAlpha (0.6f) : zc::hair); g.drawRoundedRectangle (r, 8.0f, 1.0f);
    juce::Path arrow; const float ax = (float) w - 16, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (zc::ink); g.fillPath (arrow);
}

void ZojoLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& l)
{
    l.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
    l.setFont (getComboBoxFont (box));
}

void ZojoLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (zc::module);
    g.setColour (zc::hair); g.drawRect (0, 0, w, h);
}
