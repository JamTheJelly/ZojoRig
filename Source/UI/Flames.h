#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// "Fern Burn" theme: dot-matrix fire and chrome lettering.
namespace zc
{
// A cellular fire (classic demo-scene algorithm) whose cells are drawn as glowing dots.
// Heat rises from the bottom row; random bursts make flames shoot up.
class FireField
{
public:
    void configure (int cols, int rows, float baseHeat, float decay, float burstMin, float burstMax);
    void step();
    void warm (int steps) { for (int i = 0; i < steps; ++i) step(); }
    void draw (juce::Graphics&, juce::Point<float> origin, float spacing) const;

private:
    struct Burst { int centre, half; float heat; int life; };
    int cols = 0, rows = 0, cooldown = 0;
    float base = 1.0f, decay = 0.2f, burstMin = 2.0f, burstMax = 3.5f;
    std::vector<float> heat;
    std::vector<Burst> bursts;
    juce::Image glow;   // heat as a tiny image, drawn scaled up and smoothed = soft flame shapes
    static float shade (float h) { return 1.0f - std::exp (-h * 0.95f); }
    juce::Random rng;
};

juce::Colour fireColour (float t);
juce::ColourGradient chromeGradient (juce::Rectangle<float> r);

// Polished-chrome text: gradient fill, dark outline, optional fire glow and a moving glint (0..1, <0 = none).
void drawChromeText (juce::Graphics&, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font,
                     float shear, float glint, bool fireGlow);
}
