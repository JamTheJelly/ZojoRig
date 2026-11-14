#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class ZojoRigProcessor;

namespace zojo
{
struct Preset { juce::String name, bank; std::vector<std::pair<juce::String, float>> values; };

const std::vector<Preset>& factoryPresets();
void applyPreset (juce::AudioProcessorValueTreeState&, const Preset&);

juce::File userPresetDir();
juce::StringArray userPresetNames();
bool saveUserPreset (ZojoRigProcessor&, const juce::String& name);
bool loadUserPreset (ZojoRigProcessor&, const juce::String& name);
void deleteUserPreset (const juce::String& name);
}
