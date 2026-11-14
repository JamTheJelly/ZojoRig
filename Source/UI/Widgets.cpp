#include "Widgets.h"
#include "../PluginProcessor.h"

//==============================================================================
ZKnob::ZKnob (APVTS& s, const juce::String& id, const juce::String& l, juce::Colour cap, juce::Colour ind, Format f)
    : label (l), format (f)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setMouseDragSensitivity (160);
    slider.setVelocityBasedMode (false);
    slider.setScrollWheelEnabled (true);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    slider.getProperties().set ("cap", (juce::int64) cap.getARGB());
    slider.getProperties().set ("ind", (juce::int64) ind.getARGB());
    slider.getProperties().set ("bipolar", f == Format::decibels);
    slider.onValueChange = [this] { repaint(); };
    addAndMakeVisible (slider);
    attachment = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void ZKnob::resized()
{
    const int sz = std::min (getWidth(), getHeight() - 26);
    slider.setBounds ((getWidth() - sz) / 2, 0, sz, sz);
}

void ZKnob::paint (juce::Graphics& g)
{
    const float v = (float) slider.getValue();
    juce::String val;
    if (format == Format::decibels) { const float d = (v - 5.0f) * 2.4f; val = (d > 0 ? "+" : "") + juce::String (d, 1); }
    else val = juce::String (v, 1);
    const int top = slider.getBottom() - 2;
    g.setColour (labelColour); g.setFont (zc::mono (9.5f, true));
    g.drawText (label.toUpperCase(), 0, top, getWidth(), 12, juce::Justification::centred);
    g.setColour (valueColour); g.setFont (zc::mono (9.5f));
    g.drawText (val, 0, top + 12, getWidth(), 12, juce::Justification::centred);
}

//==============================================================================
Pedal::Pedal (APVTS& s, int i, const juce::String& c, const juce::String& n, juce::Colour col, const juce::String& onId,
              std::vector<KnobSpec> specs, juce::Colour indicator)
    : code (c), name (n), index (i), colour (col)
{
    for (auto& k : specs)
    {
        knobs.push_back (std::make_unique<ZKnob> (s, k.id, k.label, colour, indicator, k.fmt));
        addAndMakeVisible (*knobs.back());
    }
    foot.setClickingTogglesState (true);
    foot.getProperties().set ("accent", (juce::int64) colour.getARGB());
    foot.getProperties().set ("softOn", true);
    foot.getProperties().set ("onText", (juce::int64) colour.brighter (0.3f).getARGB());
    foot.onStateChange = [this] { refresh(); };
    addAndMakeVisible (foot);
    footAttachment = std::make_unique<APVTS::ButtonAttachment> (s, onId, foot);
    refresh();
}

void Pedal::refresh()
{
    const bool on = foot.getToggleState();
    foot.setButtonText (on ? "ON" : "BYPASS");
    for (auto& k : knobs) k->setGlow (on ? 1.0f : 0.35f);
    if (on != lastOn) { lastOn = on; repaint(); }
}

void Pedal::resized()
{
    auto r = getLocalBounds().reduced (12);
    r.removeFromTop (46);
    auto footRow = r.removeFromBottom (36);
    foot.setBounds (footRow.withTrimmedLeft (20));
    const int n = (int) knobs.size(), kw = 58, kh = 78;
    const int perRow = n <= 3 ? n : (n == 4 ? 2 : n);
    const int rows = (n + perRow - 1) / perRow;
    for (int i = 0; i < n; ++i)
    {
        const int row = i / perRow, inRow = std::min (perRow, n - row * perRow), col = i % perRow;
        const int x = r.getX() + (r.getWidth() - inRow * kw) / 2 + col * kw;
        const int y = r.getY() + row * kh + std::max (0, (r.getHeight() - rows * kh) / 2 - 4);
        knobs[(size_t) i]->setBounds (x, y, kw, kh);
    }
}

void Pedal::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    const bool on = foot.getToggleState();
    if (on)
    {
        juce::Path p; p.addRoundedRectangle (r, 16);
        juce::DropShadow (colour.withAlpha (0.35f), 26, {}).drawForPath (g, p);
    }
    zc::drawModule (g, r);
    if (on) { g.setColour (colour.withAlpha (0.45f)); g.drawRoundedRectangle (r.reduced (0.5f), 16, 1.2f); }

    g.setFont (zc::mono (9.0f)); g.setColour (zc::sub);
    g.drawText (juce::String (index).paddedLeft ('0', 2) + "  " + code, 12, 10, getWidth() - 40, 12, juce::Justification::centredLeft);
    juce::Path dot; dot.addEllipse ((float) getWidth() - 24, 10, 11, 11);
    juce::DropShadow (colour.withAlpha (0.8f), 8, {}).drawForPath (g, dot);
    g.setColour (colour); g.fillPath (dot);
    g.setColour (zc::ink); g.setFont (zc::mono (17.0f, true));
    g.drawText (name.toUpperCase(), 12, 24, getWidth() - 24, 22, juce::Justification::centredLeft);

    // LED next to the footswitch
    const auto fb = foot.getBounds().toFloat();
    juce::Rectangle<float> led (fb.getX() - 15, fb.getCentreY() - 4.5f, 9, 9);
    if (on) { juce::Path lp; lp.addEllipse (led); juce::DropShadow (zc::red, 10, {}).drawForPath (g, lp); }
    g.setColour (on ? zc::red : juce::Colour (0xff2a2a2e)); g.fillEllipse (led);
}

//==============================================================================
void DotMeter::setLevel (float peak)
{
    const float l = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (peak, -60.0f) + 60.0f) / 60.0f);
    level = std::max (l, level - 0.035f);
    if (peak > 0.98f) clipTime = juce::Time::getMillisecondCounter();
    repaint();
}

void DotMeter::paint (juce::Graphics& g)
{
    const float s = 8.0f, r = 2.6f;
    const int cols = (int) ((float) getWidth() / s), rows = std::max (1, (int) ((float) getHeight() / s));
    for (int c = 0; c < cols; ++c)
    {
        const float t = (float) c / (float) std::max (1, cols - 1);
        const bool lit = t <= level && level > 0.001f;
        const auto col = ! lit ? zc::dotOff : t < 0.62f ? zc::green : t < 0.82f ? zc::ink : zc::red;
        g.setColour (col);
        for (int rr = 0; rr < rows; ++rr) g.fillEllipse (c * s + s * 0.5f - r, rr * s + s * 0.5f - r, r * 2, r * 2);
    }
}

//==============================================================================
TunerOverlay::TunerOverlay (ZojoRigProcessor& p) : proc (p)
{
    buf.resize (4096);
    close.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible (close);
    setVisible (false);
}

void TunerOverlay::show (bool s)
{
    setVisible (s);
    proc.tunerMute = s;
    if (s) { startTimerHz (20); toFront (true); } else stopTimer();
}

void TunerOverlay::mouseDown (const juce::MouseEvent& e) { if (! box.contains (e.position) && onClose) onClose(); }

void TunerOverlay::resized()
{
    box = getLocalBounds().toFloat().withSizeKeepingCentre (460, 330);
    close.setBounds ((int) box.getCentreX() - 50, (int) box.getBottom() - 54, 100, 32);
}

void TunerOverlay::timerCallback()
{
    const double sr = proc.copyTunerSamples (buf.data(), (int) buf.size());
    const float f = zojo::detectPitch (buf.data(), (int) buf.size(), sr);
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const auto now = juce::Time::getMillisecondCounter();
    if (f > 0)
    {
        lastNoteTime = now;
        const float midi = 69.0f + 12.0f * std::log2 (f / 440.0f);
        const int rn = (int) std::lround (midi);
        cents = (midi - (float) rn) * 100.0f;
        note = juce::String (names[((rn % 12) + 12) % 12]) + juce::String (rn / 12 - 1);
        inTune = std::abs (cents) < 4.0f; hasNote = true;
        freqText = juce::String (f, 1) + " Hz   " + (cents >= 0 ? "+" : "") + juce::String ((int) std::lround (cents)) + " cents";
    }
    else if (now - lastNoteTime > 1500)
    {
        hasNote = false; inTune = false; cents = 0; note = "-"; freqText = "play a single string";
    }
    repaint();
}

void TunerOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xcc040405));
    juce::Path bp; bp.addRoundedRectangle (box, 22);
    juce::DropShadow (juce::Colours::black, 40, { 0, 20 }).drawForPath (g, bp);
    g.setColour (juce::Colour (0xff0e0e10)); g.fillPath (bp);
    g.setColour (zc::hair); g.strokePath (bp, juce::PathStrokeType (1));

    auto r = box.reduced (26, 22);
    g.setFont (zc::mono (9.0f)); g.setColour (zc::sub);
    g.drawText ("TUNER  -  A4 = 440 HZ", r.removeFromTop (14), juce::Justification::centredLeft);
    g.drawText ("output muted", box.reduced (26, 22).removeFromTop (14), juce::Justification::centredRight);
    auto noteArea = r.removeFromTop (150);
    zc::drawDotMatrixText (g, note, noteArea, 140.0f, inTune ? zc::white : juce::Colour (0xff4a4a47), 6.5f, juce::Justification::centred, inTune);

    auto scale = r.removeFromTop (34).reduced (0, 6);
    zc::drawDotGrid (g, scale, 8.0f, 2.6f, juce::Colour (0xff3a3a37));
    g.setColour (juce::Colour (0xff555555)); g.fillRect (scale.getCentreX() - 1, scale.getY() - 6, 2.0f, scale.getHeight() + 12);
    if (hasNote)
    {
        const float x = scale.getX() + scale.getWidth() * juce::jlimit (0.02f, 0.98f, 0.5f + cents / 100.0f);
        juce::Rectangle<float> needle (x - 3, scale.getY() - 8, 6, scale.getHeight() + 16);
        juce::Path np; np.addRoundedRectangle (needle, 3);
        juce::DropShadow (zc::red, 12, {}).drawForPath (g, np);
        g.setColour (inTune ? zc::green : zc::red); g.fillPath (np);
    }
    g.setColour (juce::Colour (0xff9b9b94)); g.setFont (zc::mono (11.0f));
    g.drawText (freqText, r.removeFromTop (22), juce::Justification::centred);
}
