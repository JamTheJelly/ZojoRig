// Offline test runner: exercises the DSP, presets and renders the UI to a PNG.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Presets.h"
#include <chrono>

static void setP (ZojoRigProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id); prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// guitar-ish test signal: plucked decaying sawtooth notes
static void fillGuitar (juce::AudioBuffer<float>& b, double sr, int startSample, float freq)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = (double) (startSample + i) / sr;
        const double tn = std::fmod (t, 0.5);
        const double ph = std::fmod (t * freq, 1.0);
        const float v = (float) ((2.0 * ph - 1.0) * 0.18 * std::exp (-tn * 4.0));
        for (int c = 0; c < b.getNumChannels(); ++c) b.setSample (c, i, v);
    }
}

struct Stats { float peak = 0; double rms = 0; bool bad = false; double rtf = 0; };
static Stats run (ZojoRigProcessor& p, double seconds, float freq = 110.0f)
{
    const double sr = 48000; const int bs = 256;
    juce::AudioBuffer<float> b (2, bs); juce::MidiBuffer midi;
    Stats s; double sum = 0; long n = 0;
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (int pos = 0; pos < (int) (seconds * sr); pos += bs)
    {
        fillGuitar (b, sr, pos, freq);
        p.processBlock (b, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < bs; ++i)
            {
                const float v = b.getSample (c, i);
                if (! std::isfinite (v)) s.bad = true;
                if (pos > sr * 0.3) { s.peak = std::max (s.peak, std::abs (v)); sum += (double) v * v; ++n; }
            }
    }
    const double el = std::chrono::duration<double> (std::chrono::high_resolution_clock::now() - t0).count();
    s.rms = std::sqrt (sum / std::max (1L, n)); s.rtf = seconds / el;
    return s;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    int failures = 0;
    auto check = [&] (bool ok, const juce::String& what) { std::cout << (ok ? "  ok   " : "  FAIL ") << what << "\n"; if (! ok) ++failures; };

    ZojoRigProcessor p;
    p.setPlayConfigDetails (2, 2, 48000, 256);
    p.prepareToPlay (48000, 256);

    std::cout << "Amp models (gain 0 / 5 / 10): peak, rms, realtime factor\n";
    for (int m = 0; m < (int) zojo::ampModels().size(); ++m)
    {
        setP (p, "amp_model", (float) m);
        juce::String line = juce::String (zojo::ampModels()[(size_t) m].name).paddedRight (' ', 16);
        bool ok = true;
        for (float g : { 0.0f, 5.0f, 10.0f })
        {
            setP (p, "amp_gain", g);
            auto s = run (p, 1.0);
            line << "  " << juce::String (s.peak, 2) << "/" << juce::String (s.rms, 3) << " (" << juce::String (s.rtf, 0) << "x)";
            ok = ok && ! s.bad && s.peak > 0.01f && s.peak < 1.5f;
        }
        std::cout << "  " << line << "\n";
        check (ok, juce::String ("levels sane for ") + zojo::ampModels()[(size_t) m].name);
    }
    setP (p, "amp_model", 2); setP (p, "amp_gain", 5.5f);

    std::cout << "Pedals on (peak, rms):\n";
    for (auto* id : { "gate_on", "comp_on", "od_on", "fuzz_on", "eq_on", "dbl_on", "cho_on", "pha_on", "dly_on", "rev_on" })
    {
        setP (p, id, 1); auto s = run (p, 1.0); setP (p, id, 0);
        check (! s.bad && s.peak > 0.01f && s.peak < 1.6f, juce::String (id) + "  peak " + juce::String (s.peak, 2) + " rms " + juce::String (s.rms, 3));
    }

    // virtual tuning pitch accuracy (direct on the shifter with a pure saw)
    std::cout << "Virtual tuning accuracy:\n";
    for (int semis : { -1, -2, -5, -7, 2 })
    {
        zojo::PitchShifter ps; ps.prepare (48000); ps.setSemitones ((float) semis);
        std::vector<float> out (48000 * 2);
        for (size_t i = 0; i < out.size(); ++i) { const double ph = std::fmod ((double) i * 110.0 / 48000.0, 1.0); out[i] = ps.process ((float) (2 * ph - 1) * 0.3f); }
        std::vector<float> f;
        for (int st = 24000; st + 4096 < (int) out.size(); st += 2000) f.push_back (zojo::detectPitch (out.data() + st, 4096, 48000));
        std::sort (f.begin(), f.end());
        const float med = f[f.size() / 2], expect = 110.0f * std::pow (2.0f, semis / 12.0f);
        const float cents = 1200.0f * std::log2 (med / expect);
        check (std::abs (cents) < 5, juce::String (semis) + " st: expect " + juce::String (expect, 1) + " got " + juce::String (med, 1) + " (" + juce::String (cents, 1) + " cents)");
    }

    // whole chain with tuning on
    setP (p, "tuning", 2); { auto s = run (p, 1.0); check (! s.bad && s.peak > 0.01f, "chain with D-standard shift  peak " + juce::String (s.peak, 2)); } setP (p, "tuning", 0);

    // tuner detection on the input tap
    {
        setP (p, "amp_model", 0); run (p, 0.5, 82.41f);
        std::vector<float> tb (4096); const double sr = p.copyTunerSamples (tb.data(), 4096);
        const float f = zojo::detectPitch (tb.data(), 4096, sr);
        check (std::abs (f - 82.41f) < 0.5f, "tuner reads low E: " + juce::String (f, 2) + " Hz");
    }

    // every preset produces sane audio
    std::cout << "Presets:\n";
    for (auto& pr : zojo::factoryPresets())
    {
        zojo::applyPreset (p.apvts, pr); auto s = run (p, 1.0);
        check (! s.bad && s.peak > 0.01f && s.peak < 1.6f, pr.bank + " / " + pr.name + "  peak " + juce::String (s.peak, 2) + " rms " + juce::String (s.rms, 3) + "  " + juce::String (s.rtf, 0) + "x realtime");
    }

    // state round-trip
    {
        setP (p, "amp_gain", 7.3f); juce::MemoryBlock mb; p.getStateInformation (mb);
        setP (p, "amp_gain", 1.0f); p.setStateInformation (mb.getData(), (int) mb.getSize());
        check (std::abs (p.apvts.getRawParameterValue ("amp_gain")->load() - 7.3f) < 0.02f, "state save/restore");
    }

    // UI snapshot
    if (argc > 1)
    {
        zojo::applyPreset (p.apvts, zojo::factoryPresets()[9]);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (RigContent::baseW, RigContent::baseH);
        run (p, 0.2);
        for (int i = 0; i < 4; ++i) { juce::Thread::sleep (40); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (argv[1]); out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "UI snapshot written: " << out.getFullPathName() << "\n";
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED\n" : juce::String (failures) + " FAILURES\n");
    return failures == 0 ? 0 : 1;
}
