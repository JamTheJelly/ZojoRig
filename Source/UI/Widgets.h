#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ZojoLookAndFeel.h"

class ZojoRigProcessor;
using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
// A glowing dot-ring knob with a label and value readout, bound to a parameter
class ZKnob : public juce::Component
{
public:
    enum class Format { tenths, decibels, percent };
    ZKnob (APVTS&, const juce::String& paramId, const juce::String& label, juce::Colour cap, juce::Colour ind = juce::Colour (0xff0b0b0c),
           Format fmt = Format::tenths);
    void resized() override;
    void paint (juce::Graphics&) override;
    void setGlow (float g) { slider.getProperties().set ("glow", g); slider.repaint(); }
    void setOffColour (juce::Colour c) { slider.getProperties().set ("off", (juce::int64) c.getARGB()); }
    void setTextColours (juce::Colour label, juce::Colour value) { labelColour = label; valueColour = value; repaint(); }
    juce::Slider slider;
private:
    std::unique_ptr<APVTS::SliderAttachment> attachment;
    juce::String label; Format format;
    juce::Colour labelColour = zc::ink, valueColour = zc::sub;
};

//==============================================================================
// A stompbox module: header, name, knobs and a footswitch
class Pedal : public juce::Component
{
public:
    struct KnobSpec { juce::String id, label; ZKnob::Format fmt = ZKnob::Format::tenths; };
    Pedal (APVTS&, int index, const juce::String& code, const juce::String& name, juce::Colour colour, const juce::String& onParamId,
           std::vector<KnobSpec> knobs, juce::Colour indicator = juce::Colours::white);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
private:
    juce::String code, name; int index; juce::Colour colour;
    std::vector<std::unique_ptr<ZKnob>> knobs;
    juce::TextButton foot;
    std::unique_ptr<APVTS::ButtonAttachment> footAttachment;
    bool lastOn = false;
};

//==============================================================================
// Dot-row level meter (green -> white -> red)
class DotMeter : public juce::Component
{
public:
    void setLevel (float peak);
    void paint (juce::Graphics&) override;
    bool clipped() const { return juce::Time::getMillisecondCounter() - clipTime < 1200; }
private:
    float level = 0; juce::uint32 clipTime = 0;
};

//==============================================================================
// Full-window tuner overlay (mutes output while open)
class TunerOverlay : public juce::Component, private juce::Timer
{
public:
    explicit TunerOverlay (ZojoRigProcessor&);
    void show (bool shouldShow);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::function<void()> onClose;
private:
    void timerCallback() override;
    ZojoRigProcessor& proc;
    juce::TextButton close { "Close" };
    juce::String note = "-", freqText = "play a single string";
    float cents = 0; bool inTune = false, hasNote = false;
    juce::uint32 lastNoteTime = 0;
    std::vector<float> buf;
    juce::Rectangle<float> box;
};
