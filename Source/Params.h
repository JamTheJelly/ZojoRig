#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace zojo
{
// Virtual tuning options: semitone shift + labels
struct Tuning { int semis; const char* shortName; const char* longName; };
inline const std::vector<Tuning>& tunings()
{
    static const std::vector<Tuning> t {
        { 0, "E", "E standard" }, { -1, "Eb", "Eb standard" }, { -2, "D", "D standard" }, { -3, "C#", "C# standard" },
        { -4, "C", "C standard" }, { -5, "B", "B standard" }, { -7, "A", "A standard" }, { -12, "-8va", "One octave down" },
        { 2, "F#", "F# (up 2)" } };
    return t;
}
inline const juce::StringArray& cabNames() { static const juce::StringArray s { "1x12 Open", "2x12", "4x12 Closed", "IR file", "Off (DI)" }; return s; }
inline const juce::StringArray& inputNames() { static const juce::StringArray s { "Input 1", "Input 2", "Both" }; return s; }

// A knob parameter on the prototype's 0..10 scale
inline std::unique_ptr<juce::AudioParameterFloat> knobParam (const juce::String& id, const juce::String& name, float def)
{
    return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), def,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); }));
}
inline std::unique_ptr<juce::AudioParameterBool> onParam (const juce::String& id, const juce::String& name, bool def = false)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}
juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
