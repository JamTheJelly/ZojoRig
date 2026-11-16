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

    // --export "Preset Name" out.zojopreset: write a factory preset as a user preset file
    if (argc > 3 && juce::String (argv[1]) == "--export")
    {
        for (auto& pr : zojo::factoryPresets())
            if (juce::String (pr.name) == juce::String (argv[2]))
            {
                zojo::applyPreset (p.apvts, pr);
                juce::XmlElement xml ("ZOJOPRESET");
                for (auto* prm : p.getParameters())
                    if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                        if (const auto id = r->getParameterID(); id != "input_gain" && id != "input_ch" && id != "out_volume" && ! id.startsWith ("ply_") && id != "tuning")
                            xml.setAttribute (r->getParameterID(), (double) r->convertFrom0to1 (r->getValue()));
                if (pr.theme.isNotEmpty()) xml.setAttribute ("theme", pr.theme);
                return xml.writeTo (juce::File (juce::String (argv[3]))) ? 0 : 1;
            }
        return 1;
    }

    // --calibrate: find the Master setting that brings every factory preset to the same loudness
    if (argc > 1 && juce::String (argv[1]) == "--calibrate")
    {
        const double target = 0.16;
        for (auto& pr : zojo::factoryPresets())
        {
            zojo::applyPreset (p.apvts, pr);
            float lo = 1.0f, hi = 9.5f, m = 5.0f; double r = 0;
            for (int it = 0; it < 9; ++it)
            {
                m = 0.5f * (lo + hi); setP (p, "amp_master", m); r = run (p, 1.5).rms;
                if (r < target) lo = m; else hi = m;
            }
            std::cout << "CAL\t" << pr.name << "\t" << juce::String (m, 1) << "\t" << juce::String (r, 3) << "\n";
        }
        return 0;
    }

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

    // ---------------- practice player ----------------
    {
        std::cout << "File player:\n";
        // 6 s stereo 44.1 kHz test song: 220 Hz tone (also exercises resampling to 48 kHz)
        const auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zojo_test_song.wav");
        wav.deleteFile();
        {
            juce::WavAudioFormat fmt;
            std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), 44100.0, 2, 16, {}, 0));
            juce::AudioBuffer<float> b (2, 44100 * 6);
            for (int i = 0; i < b.getNumSamples(); ++i) { const float v = 0.5f * std::sin (2.0f * zojo::kPi * 220.0f * (float) i / 44100.0f); b.setSample (0, i, v); b.setSample (1, i, v); }
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
        zojo::applyPreset (p.apvts, zojo::factoryPresets()[0]);
        setP (p, "rev_on", 0); setP (p, "tuning", 0); setP (p, "ply_volume", 6); setP (p, "ply_follow", 1);
        p.player.load (wav);
        for (int i = 0; i < 300 && p.player.isLoading(); ++i) juce::Thread::sleep (10);
        check (! p.player.isLoading() && std::abs (p.player.getLengthSeconds() - 6.0) < 0.01, "loads + resamples 44.1k WAV: " + juce::String (p.player.getLengthSeconds(), 3) + " s");

        auto runSilent = [&] (double seconds, std::vector<float>* tail)
        {
            juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer midi;
            std::vector<float> keep;
            for (int pos = 0; pos < (int) (seconds * 48000); pos += 256)
            {
                b.clear(); p.processBlock (b, midi);
                for (int i = 0; i < 256; ++i) keep.push_back (b.getSample (0, i));
            }
            if (tail) tail->assign (keep.end() - 4096, keep.end());
        };
        auto pitchAfter = [&] (double seconds) { std::vector<float> t; runSilent (seconds, &t); return zojo::detectPitch (t.data(), 4096, 48000); };

        struct Case { float speed, key; int tuning; float expect; const char* what; };
        for (auto c : { Case { 100, 0, 0, 220.0f, "normal speed" }, Case { 75, 0, 0, 220.0f, "75% speed keeps pitch" },
                        Case { 100, 2, 0, 246.94f, "+2 st key change" }, Case { 100, 0, 2, 196.0f, "follows D-standard tuning (-2 st)" } })
        {
            setP (p, "ply_speed", c.speed); setP (p, "ply_key", c.key); setP (p, "tuning", (float) c.tuning);
            p.player.seekTo (0); p.player.setPlaying (true);
            runSilent (0.3, nullptr);                 // let the seek land and latency settle
            const double t0 = p.player.getPositionSeconds();
            const float f = pitchAfter (2.0);
            const double moved = p.player.getPositionSeconds() - t0;
            const float cents = 1200.0f * std::log2 (f / c.expect);
            check (std::abs (cents) < 15 && std::abs (moved - 2.0 * c.speed / 100.0) < 0.25,
                   juce::String (c.what) + ": " + juce::String (f, 1) + " Hz (" + juce::String (cents, 1) + " cents), song advanced " + juce::String (moved, 2) + " s in 2 s");
        }
        setP (p, "ply_speed", 100); setP (p, "ply_key", 0); setP (p, "tuning", 0);

        p.player.setLoopPoints (1.0, 2.0); p.player.seekTo (1.0); p.player.setPlaying (true);
        double minPos = 99, maxPos = 0;
        for (int i = 0; i < 30; ++i) { runSilent (0.1, nullptr); minPos = std::min (minPos, p.player.getPositionSeconds()); maxPos = std::max (maxPos, p.player.getPositionSeconds()); }
        check (minPos >= 0.85 && maxPos <= 2.15, "A-B loop 1.0-2.0 s holds over 3 s: stayed in " + juce::String (minPos, 2) + "-" + juce::String (maxPos, 2) + " s");
        p.player.setLoopPoints (0, 0);

        p.player.seekTo (5.0); p.player.setPlaying (true); runSilent (2.0, nullptr);
        check (! p.player.isPlaying(), "stops at the end of the song");
        { std::vector<float> t; runSilent (0.3, &t); float pk = 0; for (auto v : t) pk = std::max (pk, std::abs (v)); check (pk < 0.01f, "silent after stopping"); }
        wav.deleteFile();
    }

    // state round-trip
    {
        setP (p, "amp_gain", 7.3f); juce::MemoryBlock mb; p.getStateInformation (mb);
        setP (p, "amp_gain", 1.0f); p.setStateInformation (mb.getData(), (int) mb.getSize());
        check (std::abs (p.apvts.getRawParameterValue ("amp_gain")->load() - 7.3f) < 0.02f, "state save/restore");
    }

    // preset greeting clip mixes into the output
    {
        const auto base = run (p, 0.8).rms;
        juce::File wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("zojo_greet.wav");
        wav.deleteFile();
        {
            juce::AudioBuffer<float> b (1, 44100 / 2);
            for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (0, i, 0.5f * std::sin (6.2831853f * 880.0f * (float) i / 44100.0f));
            juce::WavAudioFormat wf;
            std::unique_ptr<juce::AudioFormatWriter> w (wf.createWriterFor (new juce::FileOutputStream (wav), 44100, 1, 16, {}, 0));
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
        p.playGreeting (wav);
        const auto with = run (p, 0.45).rms;
        const auto after = run (p, 0.8).rms;
        check (with > base * 1.3 && after < with, "greeting clip plays once (rms " + juce::String (base, 3) + " -> " + juce::String (with, 3) + " -> " + juce::String (after, 3) + ")");
    }

    // UI snapshot
    if (argc > 1)
    {
        zojo::applyPreset (p.apvts, zojo::factoryPresets().back());
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (RigContent::baseW, RigContent::baseH);
        run (p, 0.2);
        for (int i = 0; i < 4; ++i) { juce::Thread::sleep (40); }
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (argv[1]); out.deleteFile();
        juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "UI snapshot written: " << out.getFullPathName() << "\n";

        Toast toast; toast.setSize (460, 150); toast.pop ("KONNICHIWA!");
        juce::Thread::sleep (700);
        auto timg = juce::Image (juce::Image::ARGB, 460, 150, true);
        { juce::Graphics tg (timg); tg.fillAll (zc::paper); toast.paintEntireComponent (tg, true); }
        juce::File tout = out.getSiblingFile ("toast.png"); tout.deleteFile();
        juce::FileOutputStream ts (tout); juce::PNGImageFormat().writeImageToStream (timg, ts);
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED\n" : juce::String (failures) + " FAILURES\n");
    return failures == 0 ? 0 : 1;
}
