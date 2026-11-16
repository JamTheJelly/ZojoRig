#include "Flames.h"

namespace zc
{
void FireField::configure (int c, int r, float baseHeat, float d, float bMin, float bMax)
{
    cols = c; rows = r; base = baseHeat; decay = d; burstMin = bMin; burstMax = bMax;
    heat.assign ((size_t) (cols * rows), 0.0f);
    glow = juce::Image (juce::Image::ARGB, cols, rows, true);
    bursts.clear(); cooldown = 0;
}

void FireField::step()
{
    if (cols <= 0 || rows <= 1) return;

    // flare-ups: a few columns flood with extra heat for a moment
    if (--cooldown <= 0)
    {
        bursts.push_back ({ rng.nextInt (cols), 2 + rng.nextInt (std::max (2, cols / 18)),
                            burstMin + rng.nextFloat() * (burstMax - burstMin), 5 + rng.nextInt (12) });
        cooldown = 6 + rng.nextInt (22);
    }
    for (auto& b : bursts) --b.life;
    bursts.erase (std::remove_if (bursts.begin(), bursts.end(), [] (const Burst& b) { return b.life <= 0; }), bursts.end());

    // source row
    float* src = heat.data() + (size_t) ((rows - 1) * cols);
    for (int x = 0; x < cols; ++x)
    {
        float s = base * (0.7f + 0.3f * rng.nextFloat());
        for (auto& b : bursts)
        {
            const int dx = std::abs (x - b.centre);
            if (dx <= b.half) s = std::max (s, b.heat * (1.0f - 0.45f * (float) dx / (float) b.half));
        }
        src[x] = s;
    }

    // rise, drift and cool
    for (int y = 1; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            const int dx = juce::jlimit (0, cols - 1, x - rng.nextInt (3) + 1);
            heat[(size_t) ((y - 1) * cols + dx)] = std::max (0.0f, heat[(size_t) (y * cols + x)] - rng.nextFloat() * decay);
        }

    juce::Image::BitmapData bd (glow, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            const float t = shade (heat[(size_t) (y * cols + x)]);
            bd.setPixelColour (x, y, fireColour (t * 0.9f).withAlpha (juce::jlimit (0.0f, 1.0f, t * 0.8f)));
        }
}

juce::Colour fireColour (float t)
{
    struct Stop { float p; juce::uint32 c; };
    static const Stop stops[] = { { 0.0f, 0xff4a0800 }, { 0.3f, 0xffc4260a }, { 0.55f, 0xffff6a14 },
                                  { 0.8f, 0xffffb43a }, { 1.0f, 0xfffff1c4 } };
    t = juce::jlimit (0.0f, 1.0f, t);
    for (int i = 1; i < 5; ++i)
        if (t <= stops[i].p)
            return juce::Colour (stops[i - 1].c).interpolatedWith (juce::Colour (stops[i].c), (t - stops[i - 1].p) / (stops[i].p - stops[i - 1].p));
    return juce::Colour (stops[4].c);
}

void FireField::draw (juce::Graphics& g, juce::Point<float> origin, float spacing) const
{
    const auto clip = g.getClipBounds().toFloat();
    if (glow.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.setOpacity (0.85f);
        g.drawImage (glow, { origin.x, origin.y, (float) cols * spacing, (float) rows * spacing });
        g.setOpacity (1.0f);
    }
    for (int y = 0; y < rows; ++y)
    {
        const float cy = origin.y + ((float) y + 0.5f) * spacing;
        if (cy + spacing * 1.2f < clip.getY() || cy - spacing * 1.2f > clip.getBottom()) continue;
        for (int x = 0; x < cols; ++x)
        {
            const float t = shade (heat[(size_t) (y * cols + x)]);
            if (t < 0.18f) continue;
            const float cx = origin.x + ((float) x + 0.5f) * spacing;
            if (cx + spacing * 1.2f < clip.getX() || cx - spacing * 1.2f > clip.getRight()) continue;
            const auto col = fireColour (t);
            const float r = spacing * (0.1f + 0.17f * t);
            g.setColour (col.withAlpha (0.25f + 0.75f * t));
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        }
    }
}

juce::ColourGradient chromeGradient (juce::Rectangle<float> r)
{
    juce::ColourGradient cg (juce::Colour (0xffffffff), 0, r.getY(), juce::Colour (0xffa4a9b1), 0, r.getBottom(), false);
    cg.addColour (0.30, juce::Colour (0xffd3d6dc));
    cg.addColour (0.47, juce::Colour (0xff6a7079));
    cg.addColour (0.50, juce::Colour (0xff23262c));
    cg.addColour (0.54, juce::Colour (0xff7e848e));
    cg.addColour (0.78, juce::Colour (0xffeef0f3));
    return cg;
}

void drawChromeText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font,
                     float shear, float glint, bool fireGlow)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (font, text, 0, 0);
    juce::Path p; ga.createPath (p);
    if (p.isEmpty()) return;
    p.applyTransform (juce::AffineTransform::shear (shear, 0));
    p.applyTransform (p.getTransformToScaleToFit (area, true, juce::Justification::centredLeft));
    const auto b = p.getBounds();

    if (fireGlow) juce::DropShadow (juce::Colour (0xffff5a14).withAlpha (0.75f), 14, { 0, 2 }).drawForPath (g, p);
    juce::DropShadow (juce::Colours::black.withAlpha (0.9f), 4, { 1, 3 }).drawForPath (g, p);

    g.setGradientFill (chromeGradient (b));
    g.fillPath (p);

    if (glint >= 0.0f && glint <= 1.0f)
    {
        g.saveState();
        g.reduceClipRegion (p);
        const float gx = b.getX() - b.getHeight() + glint * (b.getWidth() + 2.0f * b.getHeight());
        juce::ColourGradient sh (juce::Colours::transparentWhite, gx - 22, 0, juce::Colours::transparentWhite, gx + 22, 0, false);
        sh.addColour (0.5, juce::Colours::white.withAlpha (0.85f));
        g.setGradientFill (sh);
        juce::Path band; band.addRectangle (gx - 22, b.getY(), 44, b.getHeight());
        band.applyTransform (juce::AffineTransform::shear (-0.5f, 0).translated (b.getCentreY() * 0.5f, 0));
        g.fillPath (band);
        g.restoreState();
    }

    g.setColour (juce::Colour (0xff121317));
    g.strokePath (p, juce::PathStrokeType (1.4f));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (p, juce::PathStrokeType (0.5f), juce::AffineTransform::translation (0, -0.6f));
}
}
