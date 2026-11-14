#include "Presets.h"
#include "PluginProcessor.h"

namespace zojo
{
// Settings that belong to your rig rather than to a sound; presets leave them alone.
static bool isRigSetting (const juce::String& id) { return id == "input_gain" || id == "input_ch" || id == "out_volume"; }

const std::vector<Preset>& factoryPresets()
{
    // amp_model: 0 Clean '65, 1 Glass Acoustic, 2 Brit Crunch, 3 Hot-Rod Lead, 4 Silk Lead, 5 Modern Hi-Gain, 6 Iron Rhythm
    // cab_type: 0 1x12, 1 2x12, 2 4x12
    static const std::vector<Preset> p {
        { "Sparkle Clean", "Factory", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 0 }, { "amp_gain", 3 }, { "amp_bass", 6 }, { "amp_mid", 4 }, { "amp_treble", 6.5f }, { "amp_presence", 5.5f },
            { "cab_type", 1 }, { "cho_on", 1 }, { "cho_rate", 3 }, { "cho_depth", 4 }, { "cho_mix", 4 }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 3.5f } } },
        { "Classic Crunch", "Factory", { { "amp_model", 2 }, { "amp_gain", 5.5f }, { "amp_mid", 6.5f }, { "amp_treble", 6 }, { "amp_presence", 5.5f }, { "amp_master", 6 },
            { "cab_type", 2 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },
        { "Singing Lead", "Factory", { { "od_on", 1 }, { "od_drive", 2 }, { "od_level", 7 }, { "amp_model", 3 }, { "amp_gain", 6 }, { "amp_mid", 7 }, { "amp_treble", 6 }, { "amp_presence", 5.5f },
            { "amp_master", 5.5f }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 4.5f }, { "dly_feedback", 3.5f }, { "dly_mix", 3 }, { "rev_on", 1 } } },
        { "Metal Rhythm", "Factory", { { "gate_on", 1 }, { "gate_thresh", 5 }, { "gate_release", 2 }, { "od_on", 1 }, { "od_drive", 0.5f }, { "od_tone", 6 }, { "od_level", 8 },
            { "amp_model", 5 }, { "amp_gain", 7 }, { "amp_bass", 6.5f }, { "amp_mid", 3.5f }, { "amp_treble", 6.5f }, { "amp_presence", 6.5f }, { "cab_type", 2 }, { "rev_on", 0 } } },
        { "Ambient Wash", "Factory", { { "comp_on", 1 }, { "comp_sustain", 6 }, { "amp_model", 0 }, { "amp_gain", 2.5f }, { "amp_treble", 6 }, { "cab_type", 1 },
            { "cho_on", 1 }, { "cho_rate", 1.5f }, { "cho_depth", 6 }, { "cho_mix", 4 }, { "dly_on", 1 }, { "dly_time", 6.5f }, { "dly_feedback", 6 }, { "dly_mix", 4.5f },
            { "rev_on", 1 }, { "rev_size", 9 }, { "rev_mix", 6 } } },
        { "Garage Fuzz", "Factory", { { "fuzz_on", 1 }, { "fuzz_fuzz", 7 }, { "fuzz_tone", 5.5f }, { "amp_model", 0 }, { "amp_gain", 4.5f }, { "amp_bass", 5.5f }, { "amp_mid", 5.5f },
            { "amp_treble", 5.5f }, { "amp_master", 6 }, { "cab_type", 0 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },

        { "Glass Sparkle", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 6 }, { "amp_model", 1 }, { "amp_gain", 4 }, { "amp_bass", 4.5f }, { "amp_mid", 4.5f }, { "amp_treble", 6.5f },
            { "amp_presence", 6 }, { "cab_type", 1 }, { "dbl_on", 1 }, { "dbl_width", 5 }, { "dbl_time", 3 }, { "cho_on", 1 }, { "cho_rate", 2 }, { "cho_depth", 3 }, { "cho_mix", 3 },
            { "dly_on", 1 }, { "dly_time", 3.5f }, { "dly_feedback", 2.5f }, { "dly_mix", 2 }, { "rev_on", 1 }, { "rev_size", 6 }, { "rev_mix", 3.5f } } },
        { "Silk Lead", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 4 }, { "amp_gain", 5.5f }, { "amp_mid", 6.5f }, { "amp_treble", 5.5f }, { "amp_presence", 5.5f },
            { "amp_master", 5.5f }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 5 }, { "dly_feedback", 3 }, { "dly_mix", 3 }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 3 } } },
        { "Crisp Rhythm", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 3 }, { "od_on", 1 }, { "od_drive", 1 }, { "od_tone", 6 }, { "od_level", 7 }, { "amp_model", 2 }, { "amp_gain", 4 },
            { "amp_bass", 4.5f }, { "amp_mid", 6 }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "amp_master", 5.5f }, { "cab_type", 2 }, { "dbl_on", 1 }, { "dbl_width", 7 },
            { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },

        { "Iron Chug", "Gojira / Nolly-style", { { "gate_on", 1 }, { "gate_thresh", 5.5f }, { "gate_release", 2 }, { "od_on", 1 }, { "od_drive", 0 }, { "od_tone", 6.5f }, { "od_level", 8 },
            { "amp_model", 6 }, { "amp_gain", 7.5f }, { "amp_bass", 6 }, { "amp_mid", 5.5f }, { "amp_treble", 6 }, { "amp_presence", 6 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 5.4f }, { "eq_b2", 4.2f }, { "eq_b4", 5.6f }, { "eq_b5", 4.6f }, { "dbl_on", 1 }, { "dbl_width", 7 }, { "rev_on", 0 } } },
        { "Djent Precision", "Gojira / Nolly-style", { { "gate_on", 1 }, { "gate_thresh", 6 }, { "gate_release", 1.5f }, { "comp_on", 1 }, { "comp_sustain", 3 }, { "od_on", 1 }, { "od_drive", 0 },
            { "od_tone", 7 }, { "od_level", 8 }, { "amp_model", 5 }, { "amp_gain", 6.5f }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 4.4f }, { "eq_b2", 4.2f }, { "eq_b3", 5.6f }, { "eq_b4", 5.8f }, { "eq_b5", 4.4f }, { "dbl_on", 1 }, { "dbl_time", 3 }, { "rev_on", 0 } } },
        { "Phased Clean Atmos", "Gojira / Nolly-style", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 0 }, { "amp_gain", 3 }, { "amp_bass", 5.5f }, { "amp_mid", 5 }, { "amp_treble", 5.5f },
            { "cab_type", 1 }, { "pha_on", 1 }, { "pha_rate", 2 }, { "pha_depth", 6 }, { "pha_mix", 5 }, { "dly_on", 1 }, { "dly_time", 6 }, { "dly_feedback", 5 }, { "dly_mix", 3.5f },
            { "rev_on", 1 }, { "rev_size", 8 }, { "rev_mix", 5 } } } };
    return p;
}

static void setParam (juce::AudioProcessorValueTreeState& s, const juce::String& id, float value)
{
    if (auto* prm = s.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void applyPreset (juce::AudioProcessorValueTreeState& s, const Preset& preset)
{
    for (auto* prm : s.processor.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! isRigSetting (r->getParameterID()) && r->getParameterID() != "tuning")
                r->setValueNotifyingHost (r->getDefaultValue());
    for (auto& [id, v] : preset.values) setParam (s, id, v);
}

juce::File userPresetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("ZOJO RIG").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::StringArray userPresetNames()
{
    juce::StringArray names;
    for (auto& f : userPresetDir().findChildFiles (juce::File::findFiles, false, "*.zojopreset")) names.add (f.getFileNameWithoutExtension());
    names.sortNatural();
    return names;
}

bool saveUserPreset (ZojoRigProcessor& proc, const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    juce::XmlElement xml ("ZOJOPRESET");
    for (auto* prm : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! isRigSetting (r->getParameterID()))
                xml.setAttribute (r->getParameterID(), (double) r->convertFrom0to1 (r->getValue()));
    if (proc.hasIR()) xml.setAttribute ("irPath", proc.irFile.getFullPathName());
    return xml.writeTo (userPresetDir().getChildFile (clean + ".zojopreset"));
}

bool loadUserPreset (ZojoRigProcessor& proc, const juce::String& name)
{
    auto xml = juce::XmlDocument::parse (userPresetDir().getChildFile (name + ".zojopreset"));
    if (xml == nullptr || ! xml->hasTagName ("ZOJOPRESET")) return false;
    for (auto* prm : proc.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            const auto id = r->getParameterID();
            if (! isRigSetting (id) && xml->hasAttribute (id))
                r->setValueNotifyingHost (r->convertTo0to1 ((float) xml->getDoubleAttribute (id)));
        }
    const juce::File ir (xml->getStringAttribute ("irPath"));
    if (ir.existsAsFile()) proc.loadImpulseResponse (ir);
    return true;
}

void deleteUserPreset (const juce::String& name) { userPresetDir().getChildFile (name + ".zojopreset").deleteFile(); }
}
