#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class ZojoRigProcessor;

namespace zojo
{
struct Preset { juce::String name, bank; std::vector<std::pair<juce::String, float>> values; juce::String theme = {}; };

// UI theme carried by a preset (stored as a property on the APVTS state); empty = standard look
juce::String currentTheme (const juce::AudioProcessorValueTreeState&);

const std::vector<Preset>& factoryPresets();
void applyPreset (juce::AudioProcessorValueTreeState&, const Preset&);

juce::File userPresetDir();
// Documents\ZOJO RIG\Sounds\<preset name>.(wav|mp3|flac|ogg|aiff|m4a), played when that preset is picked
juce::File greetingFileFor (const juce::String& presetName);
juce::StringArray userPresetNames();
bool saveUserPreset (ZojoRigProcessor&, const juce::String& name);
bool loadUserPreset (ZojoRigProcessor&, const juce::String& name);
void deleteUserPreset (const juce::String& name);
}
