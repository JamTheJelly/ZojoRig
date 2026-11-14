#include "Presets.h"
#include "PluginProcessor.h"

namespace zojo
{
// Settings that belong to your rig rather than to a sound; presets leave them alone.
static bool isRigSetting (const juce::String& id) { return id == "input_gain" || id == "input_ch" || id == "out_volume"; }

const std::vector<Preset>& factoryPresets()
{
    // amp_model: 0 Clean '65, 1 Glass Acoustic, 2 Brit Crunch, 3 Hot-Rod Lead, 4 Silk Lead, 5 Modern Hi-Gain, 6 Iron Rhythm
    // cab_type: 0 1x12, 1 2x12, 2 4x12. Anything not listed falls back to its default.
    static const std::vector<Preset> p {
        // ---------------- Clean & Ambient ----------------
        { "Sparkle Clean", "Clean & Ambient", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 0 }, { "amp_master", 5.8f }, { "amp_gain", 3 }, { "amp_bass", 6 }, { "amp_mid", 4 }, { "amp_treble", 6.5f }, { "amp_presence", 5.5f },
            { "cab_type", 1 }, { "cho_on", 1 }, { "cho_rate", 3 }, { "cho_depth", 4 }, { "cho_mix", 4 }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 3.5f } } },
        { "Jazz Box", "Clean & Ambient", { { "comp_on", 1 }, { "comp_sustain", 3 }, { "amp_model", 0 }, { "amp_master", 4.2f }, { "amp_gain", 2 }, { "amp_bass", 6.5f }, { "amp_mid", 5 }, { "amp_treble", 3.5f },
            { "amp_presence", 3.5f }, { "cab_type", 1 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },
        { "Country Snap", "Clean & Ambient", { { "comp_on", 1 }, { "comp_sustain", 6 }, { "comp_level", 5.5f }, { "amp_model", 0 }, { "amp_master", 5.6f }, { "amp_gain", 3.5f }, { "amp_bass", 4.5f }, { "amp_mid", 5.5f },
            { "amp_treble", 7 }, { "amp_presence", 6.5f }, { "cab_type", 0 }, { "dly_on", 1 }, { "dly_time", 2 }, { "dly_feedback", 1.5f }, { "dly_mix", 2.5f }, { "rev_on", 0 } } },
        { "Surf Spring", "Clean & Ambient", { { "amp_model", 0 }, { "amp_master", 6.5f }, { "amp_gain", 4 }, { "amp_mid", 4 }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "cab_type", 0 },
            { "rev_on", 1 }, { "rev_size", 6 }, { "rev_mix", 6 } } },
        { "Shimmer Arps", "Clean & Ambient", { { "comp_on", 1 }, { "amp_model", 1 }, { "amp_master", 8.1f }, { "amp_gain", 3 }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "cab_type", 1 },
            { "dbl_on", 1 }, { "dbl_width", 6 }, { "cho_on", 1 }, { "cho_rate", 2.5f }, { "cho_depth", 5 }, { "cho_mix", 4 },
            { "dly_on", 1 }, { "dly_time", 5.5f }, { "dly_feedback", 5 }, { "dly_mix", 4 }, { "rev_on", 1 }, { "rev_size", 8 }, { "rev_mix", 5 } } },
        { "Lo-Fi Bedroom", "Clean & Ambient", { { "comp_on", 1 }, { "amp_model", 0 }, { "amp_master", 5.7f }, { "amp_gain", 3.5f }, { "amp_bass", 5.5f }, { "amp_treble", 4 }, { "cab_type", 0 },
            { "cho_on", 1 }, { "cho_rate", 1 }, { "cho_depth", 7 }, { "cho_mix", 5 }, { "eq_on", 1 }, { "eq_b1", 5.5f }, { "eq_b5", 3 },
            { "rev_on", 1 }, { "rev_size", 4 }, { "rev_mix", 3 } } },
        { "Ambient Wash", "Clean & Ambient", { { "comp_on", 1 }, { "comp_sustain", 6 }, { "amp_model", 0 }, { "amp_master", 6.5f }, { "amp_gain", 2.5f }, { "amp_treble", 6 }, { "cab_type", 1 },
            { "cho_on", 1 }, { "cho_rate", 1.5f }, { "cho_depth", 6 }, { "cho_mix", 4 }, { "dly_on", 1 }, { "dly_time", 6.5f }, { "dly_feedback", 6 }, { "dly_mix", 4.5f },
            { "rev_on", 1 }, { "rev_size", 9 }, { "rev_mix", 6 } } },

        // ---------------- Blues & Classic Rock ----------------
        { "Texas Blues", "Blues & Classic Rock", { { "od_on", 1 }, { "od_drive", 3 }, { "od_tone", 6 }, { "od_level", 6.5f }, { "amp_model", 0 }, { "amp_gain", 5 }, { "amp_bass", 5.5f },
            { "amp_mid", 6 }, { "amp_treble", 6 }, { "amp_presence", 5.5f }, { "amp_master", 7 }, { "cab_type", 0 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },
        { "Classic Crunch", "Blues & Classic Rock", { { "amp_model", 2 }, { "amp_gain", 5.5f }, { "amp_mid", 6.5f }, { "amp_treble", 6 }, { "amp_presence", 5.5f }, { "amp_master", 4.5f },
            { "cab_type", 2 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },
        { "Cranked Stack", "Blues & Classic Rock", { { "amp_model", 2 }, { "amp_gain", 7 }, { "amp_mid", 7 }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "amp_master", 4.2f },
            { "cab_type", 2 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },
        { "Southern Rhythm", "Blues & Classic Rock", { { "amp_model", 2 }, { "amp_master", 4.8f }, { "amp_gain", 5 }, { "amp_bass", 5.5f }, { "amp_mid", 6 }, { "amp_treble", 6.5f }, { "cab_type", 2 },
            { "dbl_on", 1 }, { "dbl_width", 6 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },
        { "70s Fuzz Rock", "Blues & Classic Rock", { { "fuzz_on", 1 }, { "fuzz_fuzz", 6 }, { "fuzz_tone", 5 }, { "fuzz_level", 5.5f }, { "amp_model", 2 }, { "amp_master", 3.6f }, { "amp_gain", 3.5f }, { "amp_mid", 6 },
            { "cab_type", 2 }, { "rev_on", 1 }, { "rev_size", 4 }, { "rev_mix", 2 } } },
        { "Garage Fuzz", "Blues & Classic Rock", { { "fuzz_on", 1 }, { "fuzz_fuzz", 7 }, { "fuzz_tone", 5.5f }, { "amp_model", 0 }, { "amp_gain", 4.5f }, { "amp_bass", 5.5f }, { "amp_mid", 5.5f },
            { "amp_treble", 5.5f }, { "amp_master", 4.7f }, { "cab_type", 0 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 2 } } },

        // ---------------- Hard Rock & Lead ----------------
        { "Singing Lead", "Hard Rock & Lead", { { "od_on", 1 }, { "od_drive", 2 }, { "od_level", 7 }, { "amp_model", 3 }, { "amp_gain", 6 }, { "amp_mid", 7 }, { "amp_treble", 6 }, { "amp_presence", 5.5f },
            { "amp_master", 4.5f }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 4.5f }, { "dly_feedback", 3.5f }, { "dly_mix", 3 }, { "rev_on", 1 } } },
        { "Classic Rock Lead", "Hard Rock & Lead", { { "od_on", 1 }, { "od_drive", 2 }, { "od_tone", 5.5f }, { "od_level", 7 }, { "amp_model", 2 }, { "amp_master", 5.4f }, { "amp_gain", 6.5f }, { "amp_mid", 7 },
            { "amp_treble", 6 }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 4.5f }, { "dly_feedback", 3 }, { "dly_mix", 2.5f }, { "rev_on", 1 }, { "rev_size", 4 }, { "rev_mix", 2 } } },
        { "80s Arena Lead", "Hard Rock & Lead", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "od_on", 1 }, { "od_drive", 1.5f }, { "od_level", 7.5f }, { "amp_model", 3 }, { "amp_master", 4.9f }, { "amp_gain", 7 },
            { "amp_bass", 5.5f }, { "amp_mid", 6.5f }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "cab_type", 2 }, { "cho_on", 1 }, { "cho_rate", 2 }, { "cho_depth", 3 }, { "cho_mix", 2.5f },
            { "dly_on", 1 }, { "dly_time", 5 }, { "dly_feedback", 4 }, { "dly_mix", 3.5f }, { "rev_on", 1 }, { "rev_size", 6 }, { "rev_mix", 3.5f } } },
        { "Modern Rock Wall", "Hard Rock & Lead", { { "gate_on", 1 }, { "gate_thresh", 4 }, { "amp_model", 5 }, { "amp_master", 3.8f }, { "amp_gain", 5 }, { "amp_bass", 5.5f }, { "amp_mid", 5.5f }, { "amp_treble", 6 },
            { "amp_presence", 6 }, { "cab_type", 2 }, { "dbl_on", 1 }, { "dbl_width", 8 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },
        { "Shred Lead", "Hard Rock & Lead", { { "gate_on", 1 }, { "gate_thresh", 4.5f }, { "od_on", 1 }, { "od_drive", 0.5f }, { "od_tone", 6.5f }, { "od_level", 8 }, { "amp_model", 4 }, { "amp_master", 3.8f }, { "amp_gain", 7 },
            { "amp_mid", 6.5f }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 4 }, { "dly_feedback", 3 }, { "dly_mix", 2.5f }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 2.5f } } },
        { "Ballad Solo", "Hard Rock & Lead", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 4 }, { "amp_master", 3.1f }, { "amp_gain", 5.5f }, { "amp_mid", 6 }, { "amp_treble", 5 }, { "amp_presence", 5 },
            { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 6.5f }, { "dly_feedback", 4.5f }, { "dly_mix", 4 }, { "rev_on", 1 }, { "rev_size", 7 }, { "rev_mix", 4 } } },

        // ---------------- Metal & Heavy ----------------
        { "Metal Rhythm", "Metal & Heavy", { { "gate_on", 1 }, { "gate_thresh", 5 }, { "gate_release", 2 }, { "od_on", 1 }, { "od_drive", 0.5f }, { "od_tone", 6 }, { "od_level", 8 },
            { "amp_model", 5 }, { "amp_master", 4.1f }, { "amp_gain", 7 }, { "amp_bass", 6.5f }, { "amp_mid", 3.5f }, { "amp_treble", 6.5f }, { "amp_presence", 6.5f }, { "cab_type", 2 }, { "rev_on", 0 } } },
        { "Thrash Machine", "Metal & Heavy", { { "gate_on", 1 }, { "gate_thresh", 5 }, { "gate_release", 1.5f }, { "od_on", 1 }, { "od_drive", 0 }, { "od_tone", 7 }, { "od_level", 8 },
            { "amp_model", 5 }, { "amp_master", 4.2f }, { "amp_gain", 7 }, { "amp_bass", 6 }, { "amp_mid", 3 }, { "amp_treble", 7 }, { "amp_presence", 7 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 5.5f }, { "eq_b2", 4 }, { "eq_b3", 4.2f }, { "eq_b4", 5.8f }, { "rev_on", 0 } } },
        { "Metalcore Chug", "Metal & Heavy", { { "gate_on", 1 }, { "gate_thresh", 6 }, { "gate_release", 1 }, { "od_on", 1 }, { "od_drive", 0 }, { "od_tone", 7.5f }, { "od_level", 8.5f },
            { "amp_model", 6 }, { "amp_master", 3.9f }, { "amp_gain", 8 }, { "amp_bass", 6.5f }, { "amp_mid", 4 }, { "amp_treble", 6.5f }, { "amp_presence", 7 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 5.6f }, { "eq_b2", 3.8f }, { "eq_b4", 6 }, { "eq_b5", 4.6f }, { "dbl_on", 1 }, { "dbl_width", 7 }, { "dbl_time", 3 }, { "rev_on", 0 } } },
        { "Prog Metal Lead", "Metal & Heavy", { { "gate_on", 1 }, { "gate_thresh", 4 }, { "gate_release", 2.5f }, { "od_on", 1 }, { "od_drive", 0 }, { "od_tone", 6.5f }, { "od_level", 7.5f },
            { "amp_model", 5 }, { "amp_master", 4.7f }, { "amp_gain", 6.5f }, { "amp_mid", 6.5f }, { "amp_treble", 6 }, { "cab_type", 2 },
            { "dly_on", 1 }, { "dly_time", 4.5f }, { "dly_feedback", 3 }, { "dly_mix", 2.5f }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 2.5f } } },
        { "Doom Sludge", "Metal & Heavy", { { "gate_on", 1 }, { "gate_thresh", 4 }, { "gate_release", 4 }, { "fuzz_on", 1 }, { "fuzz_fuzz", 5 }, { "fuzz_tone", 3.5f }, { "fuzz_level", 6 },
            { "amp_model", 6 }, { "amp_master", 3.7f }, { "amp_gain", 6 }, { "amp_bass", 7 }, { "amp_mid", 5 }, { "amp_treble", 4.5f }, { "amp_presence", 4.5f }, { "cab_type", 2 },
            { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 2 } } },

        // ---------------- Funk, Pop & Indie ----------------
        { "Funk Strut", "Funk, Pop & Indie", { { "comp_on", 1 }, { "comp_sustain", 7 }, { "amp_model", 0 }, { "amp_master", 8.1f }, { "amp_gain", 3 }, { "amp_bass", 4 }, { "amp_mid", 5.5f }, { "amp_treble", 7 },
            { "amp_presence", 6.5f }, { "cab_type", 1 }, { "pha_on", 1 }, { "pha_rate", 3 }, { "pha_depth", 4 }, { "pha_mix", 3 }, { "rev_on", 1 }, { "rev_size", 2 }, { "rev_mix", 1 } } },
        { "Indie Jangle", "Funk, Pop & Indie", { { "comp_on", 1 }, { "amp_model", 1 }, { "amp_master", 7.9f }, { "amp_gain", 4.5f }, { "amp_bass", 4.5f }, { "amp_treble", 7 }, { "amp_presence", 6.5f }, { "cab_type", 1 },
            { "cho_on", 1 }, { "cho_rate", 3 }, { "cho_depth", 3 }, { "cho_mix", 3 }, { "dly_on", 1 }, { "dly_time", 3.5f }, { "dly_feedback", 2.5f }, { "dly_mix", 2 },
            { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 3 } } },
        { "Pop-Punk Crunch", "Funk, Pop & Indie", { { "gate_on", 1 }, { "gate_thresh", 3 }, { "amp_model", 2 }, { "amp_master", 4.2f }, { "amp_gain", 6 }, { "amp_bass", 5.5f }, { "amp_mid", 5.5f }, { "amp_treble", 6.5f },
            { "cab_type", 2 }, { "dbl_on", 1 }, { "dbl_width", 7 }, { "dbl_time", 3.5f }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },
        { "Phaser Funk Rock", "Funk, Pop & Indie", { { "pha_on", 1 }, { "pha_rate", 4 }, { "pha_depth", 6 }, { "pha_mix", 5 }, { "amp_model", 2 }, { "amp_master", 5.3f }, { "amp_gain", 4.5f }, { "amp_mid", 6 },
            { "cab_type", 2 }, { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },
        { "Shoegaze Wall", "Funk, Pop & Indie", { { "fuzz_on", 1 }, { "fuzz_fuzz", 5 }, { "fuzz_tone", 4 }, { "fuzz_level", 5 }, { "amp_model", 0 }, { "amp_master", 4.9f }, { "amp_gain", 5 }, { "cab_type", 1 },
            { "cho_on", 1 }, { "cho_rate", 1.5f }, { "cho_depth", 7 }, { "cho_mix", 5 }, { "dly_on", 1 }, { "dly_time", 6 }, { "dly_feedback", 6 }, { "dly_mix", 4.5f },
            { "rev_on", 1 }, { "rev_size", 9.5f }, { "rev_mix", 7 } } },

        // ---------------- Tim Henson-style ----------------
        { "Glass Sparkle", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 6 }, { "amp_model", 1 }, { "amp_master", 8.9f }, { "amp_gain", 4 }, { "amp_bass", 4.5f }, { "amp_mid", 4.5f }, { "amp_treble", 6.5f },
            { "amp_presence", 6 }, { "cab_type", 1 }, { "dbl_on", 1 }, { "dbl_width", 5 }, { "dbl_time", 3 }, { "cho_on", 1 }, { "cho_rate", 2 }, { "cho_depth", 3 }, { "cho_mix", 3 },
            { "dly_on", 1 }, { "dly_time", 3.5f }, { "dly_feedback", 2.5f }, { "dly_mix", 2 }, { "rev_on", 1 }, { "rev_size", 6 }, { "rev_mix", 3.5f } } },
        { "Silk Lead", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 4 }, { "amp_gain", 5.5f }, { "amp_mid", 6.5f }, { "amp_treble", 5.5f }, { "amp_presence", 5.5f },
            { "amp_master", 4.1f }, { "cab_type", 2 }, { "dly_on", 1 }, { "dly_time", 5 }, { "dly_feedback", 3 }, { "dly_mix", 3 }, { "rev_on", 1 }, { "rev_size", 5 }, { "rev_mix", 3 } } },
        { "Crisp Rhythm", "Tim Henson-style", { { "comp_on", 1 }, { "comp_sustain", 3 }, { "od_on", 1 }, { "od_drive", 1 }, { "od_tone", 6 }, { "od_level", 7 }, { "amp_model", 2 }, { "amp_gain", 4 },
            { "amp_bass", 4.5f }, { "amp_mid", 6 }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "amp_master", 7.3f }, { "cab_type", 2 }, { "dbl_on", 1 }, { "dbl_width", 7 },
            { "rev_on", 1 }, { "rev_size", 3 }, { "rev_mix", 1.5f } } },

        // ---------------- Gojira / Nolly-style ----------------
        { "Iron Chug", "Gojira / Nolly-style", { { "gate_on", 1 }, { "gate_thresh", 5.5f }, { "gate_release", 2 }, { "od_on", 1 }, { "od_drive", 0 }, { "od_tone", 6.5f }, { "od_level", 8 },
            { "amp_model", 6 }, { "amp_master", 3.8f }, { "amp_gain", 7.5f }, { "amp_bass", 6 }, { "amp_mid", 5.5f }, { "amp_treble", 6 }, { "amp_presence", 6 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 5.4f }, { "eq_b2", 4.2f }, { "eq_b4", 5.6f }, { "eq_b5", 4.6f }, { "dbl_on", 1 }, { "dbl_width", 7 }, { "rev_on", 0 } } },
        { "Djent Precision", "Gojira / Nolly-style", { { "gate_on", 1 }, { "gate_thresh", 6 }, { "gate_release", 1.5f }, { "comp_on", 1 }, { "comp_sustain", 3 }, { "od_on", 1 }, { "od_drive", 0 },
            { "od_tone", 7 }, { "od_level", 8 }, { "amp_model", 5 }, { "amp_master", 4.7f }, { "amp_gain", 6.5f }, { "amp_treble", 6.5f }, { "amp_presence", 6 }, { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 4.4f }, { "eq_b2", 4.2f }, { "eq_b3", 5.6f }, { "eq_b4", 5.8f }, { "eq_b5", 4.4f }, { "dbl_on", 1 }, { "dbl_time", 3 }, { "rev_on", 0 } } },
        { "Phased Clean Atmos", "Gojira / Nolly-style", { { "comp_on", 1 }, { "comp_sustain", 4 }, { "amp_model", 0 }, { "amp_master", 5.6f }, { "amp_gain", 3 }, { "amp_bass", 5.5f }, { "amp_mid", 5 }, { "amp_treble", 5.5f },
            { "cab_type", 1 }, { "pha_on", 1 }, { "pha_rate", 2 }, { "pha_depth", 6 }, { "pha_mix", 5 }, { "dly_on", 1 }, { "dly_time", 6 }, { "dly_feedback", 5 }, { "dly_mix", 3.5f },
            { "rev_on", 1 }, { "rev_size", 8 }, { "rev_mix", 5 } } },

        // ---------------- ZOJO Custom ----------------
        // Deep, polished prog tone + tight early-thrash crunch + singing synthwave-style solo ambience
        { "ZOJO Signature", "ZOJO Custom", { { "gate_on", 1 }, { "gate_thresh", 3.5f }, { "gate_release", 3 },
            { "comp_on", 1 }, { "comp_sustain", 3 }, { "comp_level", 5 },
            { "od_on", 1 }, { "od_drive", 1 }, { "od_tone", 5.5f }, { "od_level", 7.5f },
            { "amp_model", 3 }, { "amp_master", 3.9f }, { "amp_gain", 6.5f }, { "amp_bass", 6.5f }, { "amp_mid", 5.5f }, { "amp_treble", 5.5f }, { "amp_presence", 5 },
            { "cab_type", 2 },
            { "eq_on", 1 }, { "eq_b1", 5.5f }, { "eq_b2", 4.6f }, { "eq_b3", 5.2f }, { "eq_b4", 5.4f }, { "eq_b5", 4 },
            { "dbl_on", 1 }, { "dbl_width", 5 }, { "dbl_time", 3.5f },
            { "cho_on", 1 }, { "cho_rate", 1.5f }, { "cho_depth", 3 }, { "cho_mix", 2 },
            { "dly_on", 1 }, { "dly_time", 5.5f }, { "dly_feedback", 4 }, { "dly_mix", 3 },
            { "rev_on", 1 }, { "rev_size", 6.5f }, { "rev_mix", 3 } } } };
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
