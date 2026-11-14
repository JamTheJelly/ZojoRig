#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace zojo;

//==============================================================================
const std::vector<AmpModel>& zojo::ampModels()
{
    using B = Biquad;
    static const std::vector<AmpModel> m {
        { "Clean '65", "Glassy, big headroom. Pushes into gentle breakup at high gain.", 0xff7fb7c9,
          60, 0.5f, 7, 1, 0, 10000, 120, 500, 0.6f, 2400, {} },
        { "Glass Acoustic", "Bright, airy, almost acoustic sparkle. Best with the compressor on.", 0xffe3c08a,
          95, 0.3f, 3, 1, 0, 15000, 140, 450, 0.7f, 3500, { { B::Peak, 240, 1.0f, -4 }, { B::Peak, 5200, 0.8f, 4 }, { B::HighShelf, 9000, 0.7f, 3 } } },
        { "Brit Crunch", "Mid-forward rock crunch. Classic rhythm and blues-rock tones.", 0xffe8a33d,
          110, 2, 45, 1, 0, 6500, 110, 800, 0.8f, 2800, {} },
        { "Hot-Rod Lead", "Two cascaded gain stages. Smooth, singing sustain for solos.", 0xffd86a4a,
          140, 2.5f, 30, 2, 5, 6000, 110, 700, 0.8f, 2800, {} },
        { "Silk Lead", "Creamy, compressed and vocal. Rounded top end for fast legato.", 0xffc59bd8,
          130, 3, 32, 2, 3.5f, 4800, 120, 900, 0.7f, 2600, { { B::Peak, 1400, 0.9f, 2.5f }, { B::Peak, 3800, 2, -3 } } },
        { "Modern Hi-Gain", "Tight, saturated, aggressive. Pair with the gate and an overdrive boost.", 0xffb9c2cc,
          230, 4, 55, 2, 10, 5200, 100, 650, 0.9f, 3200, {} },
        { "Iron Rhythm", "Huge, chunky and very tight: low-end thump with gritty upper-mid bite.", 0xff8fa07a,
          320, 6, 80, 2, 12, 4700, 95, 550, 0.8f, 3000, { { B::Peak, 90, 1.3f, 5 }, { B::Peak, 2000, 1.1f, 2.5f }, { B::Peak, 7000, 1, -4 } } } };
    return m;
}

juce::AudioProcessorValueTreeState::ParameterLayout zojo::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> ps;
    auto choice = [&] (const char* id, const char* name, const juce::StringArray& items, int def)
    { ps.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def)); };

    ps.push_back (knobParam ("input_gain", "Input", 5));
    choice ("input_ch", "Input Channel", inputNames(), 2);
    juce::StringArray tn; for (auto& t : tunings()) tn.add (t.longName);
    choice ("tuning", "Virtual Tuning", tn, 0);

    ps.push_back (onParam ("gate_on", "Gate On")); ps.push_back (knobParam ("gate_thresh", "Gate Threshold", 4)); ps.push_back (knobParam ("gate_release", "Gate Release", 3));
    ps.push_back (onParam ("comp_on", "Comp On")); ps.push_back (knobParam ("comp_sustain", "Comp Sustain", 5)); ps.push_back (knobParam ("comp_level", "Comp Level", 5));
    ps.push_back (onParam ("od_on", "Overdrive On")); ps.push_back (knobParam ("od_drive", "OD Drive", 5)); ps.push_back (knobParam ("od_tone", "OD Tone", 5)); ps.push_back (knobParam ("od_level", "OD Level", 5));
    ps.push_back (onParam ("fuzz_on", "Fuzz On")); ps.push_back (knobParam ("fuzz_fuzz", "Fuzz", 6)); ps.push_back (knobParam ("fuzz_tone", "Fuzz Tone", 5)); ps.push_back (knobParam ("fuzz_level", "Fuzz Level", 5));

    juce::StringArray an; for (auto& a : ampModels()) an.add (a.name);
    choice ("amp_model", "Amp Model", an, 2);
    ps.push_back (knobParam ("amp_gain", "Gain", 5.5f)); ps.push_back (knobParam ("amp_bass", "Bass", 5)); ps.push_back (knobParam ("amp_mid", "Middle", 6));
    ps.push_back (knobParam ("amp_treble", "Treble", 6)); ps.push_back (knobParam ("amp_presence", "Presence", 5.5f)); ps.push_back (knobParam ("amp_master", "Master", 5));
    choice ("cab_type", "Cabinet", cabNames(), 2);

    ps.push_back (onParam ("eq_on", "EQ On"));
    for (auto* b : { "eq_b1", "eq_b2", "eq_b3", "eq_b4", "eq_b5" }) ps.push_back (knobParam (b, juce::String ("EQ ") + juce::String (b).getLastCharacters (1), 5));
    ps.push_back (onParam ("dbl_on", "Doubler On")); ps.push_back (knobParam ("dbl_width", "Doubler Width", 6)); ps.push_back (knobParam ("dbl_time", "Doubler Time", 4));
    ps.push_back (onParam ("cho_on", "Chorus On")); ps.push_back (knobParam ("cho_rate", "Chorus Rate", 3)); ps.push_back (knobParam ("cho_depth", "Chorus Depth", 5)); ps.push_back (knobParam ("cho_mix", "Chorus Mix", 5));
    ps.push_back (onParam ("pha_on", "Phaser On")); ps.push_back (knobParam ("pha_rate", "Phaser Rate", 3)); ps.push_back (knobParam ("pha_depth", "Phaser Depth", 5)); ps.push_back (knobParam ("pha_mix", "Phaser Mix", 5));
    ps.push_back (onParam ("dly_on", "Delay On")); ps.push_back (knobParam ("dly_time", "Delay Time", 4.5f)); ps.push_back (knobParam ("dly_feedback", "Delay Repeats", 3.5f)); ps.push_back (knobParam ("dly_mix", "Delay Mix", 3));
    ps.push_back (onParam ("rev_on", "Reverb On", true)); ps.push_back (knobParam ("rev_size", "Reverb Size", 4)); ps.push_back (knobParam ("rev_mix", "Reverb Mix", 2.5f));
    ps.push_back (knobParam ("out_volume", "Volume", 5));
    return { ps.begin(), ps.end() };
}

//==============================================================================
ZojoRigProcessor::ZojoRigProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "ZOJO", createLayout())
{
    tunerRing.assign (8192, 0.0f);
}

bool ZojoRigProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo() || in.isDisabled();
}

void ZojoRigProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int maxBlock = std::max (32, samplesPerBlock);
    mono.setSize (1, maxBlock); stereo.setSize (2, maxBlock);

    oversampling.reset(); oversampling.initProcessing ((size_t) maxBlock);
    const double osr = sr * (double) oversampling.getOversamplingFactor();

    gate.prepare (sr);
    comp.prepare ({ sr, (juce::uint32) maxBlock, 1 }); comp.setAttack (6.0f); comp.setRelease (180.0f);
    shifter.prepare (sr);
    for (auto* b : { &odHP, &odTone, &fzHP, &fzDC, &fzTone }) b->prepare (osr);
    odHP.set (Biquad::HP, 550, 0.6f); fzHP.set (Biquad::HP, 120, 0.7f); fzDC.set (Biquad::HP, 30, 0.7f);
    amp.prepare (osr); amp.setModel ((int) p ("amp_model")); lastModel = (int) p ("amp_model");
    cab.prepare (sr); cab.current = -1;
    conv.prepare ({ sr, (juce::uint32) maxBlock, 1 });
    static const float eqF[5] = { 100, 300, 900, 2500, 6000 }, eqQ[5] = { .9f, 1.1f, 1.1f, 1.1f, .9f };
    for (int i = 0; i < 5; ++i) { eq[(size_t) i].prepare (sr); eq[(size_t) i].set (Biquad::Peak, eqF[i], eqQ[i], 0); }
    doubler.prepare (sr); chorus.prepare (sr); delay.prepare (sr);
    phaser.prepare ({ sr, (juce::uint32) maxBlock, 2 }); phaser.setCentreFrequency (900.0f); phaser.setFeedback (0.35f);
    reverb.prepare ({ sr, (juce::uint32) maxBlock, 2 });

    for (auto* s : { &inGain, &outGain, &tuneMix, &gateMix, &compMix, &compMake, &eqMix, &muteGain }) s->reset (sr, 0.02);
    for (auto* s : { &odMix, &odDrive, &odLevel, &fzMix, &fzDrive, &fzLevel }) s->reset (osr, 0.02);
    ampFade.reset (osr, 0.03); ampFade.setCurrentAndTargetValue (1.0f);
    muteGain.setCurrentAndTargetValue (1.0f);
    updateParams();
    for (auto* s : { &inGain, &outGain, &tuneMix, &gateMix, &compMix, &compMake, &eqMix, &odMix, &odDrive, &odLevel, &fzMix, &fzDrive, &fzLevel })
        s->setCurrentAndTargetValue (s->getTargetValue());
    setLatencySamples ((int) oversampling.getLatencyInSamples());
}

void ZojoRigProcessor::updateParams()
{
    inGain.setTargetValue (dbToGain (-20.0f + p ("input_gain") * 4.0f));

    const int tIdx = juce::jlimit (0, (int) tunings().size() - 1, (int) p ("tuning"));
    const int semis = tunings()[(size_t) tIdx].semis;
    shifter.setSemitones ((float) semis);
    tuneMix.setTargetValue (semis != 0 ? 1.0f : 0.0f);

    gate.setParams (-80.0f + p ("gate_thresh") * 5.0f, 0.02f + p ("gate_release") * 0.048f);
    gateMix.setTargetValue (p ("gate_on") > 0.5f ? 1.0f : 0.0f);

    const float s = p ("comp_sustain");
    comp.setThreshold (-10.0f - s * 4.0f); comp.setRatio (3.0f + s * 1.2f);
    compMake.setTargetValue (dbToGain (s * 2.0f) * dbToGain (-12.0f + p ("comp_level") * 2.4f));
    compMix.setTargetValue (p ("comp_on") > 0.5f ? 1.0f : 0.0f);

    odMix.setTargetValue (p ("od_on") > 0.5f ? 1.0f : 0.0f);
    odDrive.setTargetValue (expMap (1.5f, 60, p ("od_drive") / 10.0f));
    odTone.set (Biquad::LP, expMap (900, 8000, p ("od_tone") / 10.0f), 0.7f);
    odLevel.setTargetValue (0.25f * dbToGain (-15.0f + p ("od_level") * 3.0f));
    fzMix.setTargetValue (p ("fuzz_on") > 0.5f ? 1.0f : 0.0f);
    fzDrive.setTargetValue (expMap (3, 250, p ("fuzz_fuzz") / 10.0f));
    fzTone.set (Biquad::LP, expMap (600, 6000, p ("fuzz_tone") / 10.0f), 0.7f);
    fzLevel.setTargetValue (0.22f * dbToGain (-15.0f + p ("fuzz_level") * 3.0f));
    for (auto* b : { &odHP, &odTone, &fzHP, &fzDC, &fzTone }) b->update();

    const int model = (int) p ("amp_model");
    if (model != lastModel)
    {
        amp.setModel (model); lastModel = model;
        ampFade.setCurrentAndTargetValue (0.0f); ampFade.setTargetValue (1.0f);
    }
    amp.setKnobs (p ("amp_gain"), p ("amp_bass"), p ("amp_mid"), p ("amp_treble"), p ("amp_presence"), p ("amp_master"));

    static const char* eqIds[5] = { "eq_b1", "eq_b2", "eq_b3", "eq_b4", "eq_b5" };
    for (int i = 0; i < 5; ++i) { auto& b = eq[(size_t) i]; b.set (Biquad::Peak, b.freq, b.Q, (p (eqIds[i]) - 5.0f) * 2.4f); b.update(); }
    eqMix.setTargetValue (p ("eq_on") > 0.5f ? 1.0f : 0.0f);

    doubler.width.setTargetValue (p ("dbl_on") > 0.5f ? p ("dbl_width") / 10.0f : 0.0f);
    doubler.time.setTargetValue (8.0f + p ("dbl_time") / 10.0f * 30.0f);

    chorusRate = expMap (0.1f, 6, p ("cho_rate") / 10.0f);
    chorusDepth = 0.0002f + p ("cho_depth") / 10.0f * 0.005f;
    chorus.mix.setTargetValue (p ("cho_on") > 0.5f ? p ("cho_mix") / 10.0f : 0.0f);

    phaser.setRate (expMap (0.08f, 5, p ("pha_rate") / 10.0f));
    phaser.setDepth (juce::jlimit (0.0f, 1.0f, p ("pha_depth") / 10.0f * 0.9f));
    phaser.setMix (p ("pha_on") > 0.5f ? 0.15f + p ("pha_mix") / 10.0f * 0.4f : 0.0f);

    delay.time.setTargetValue (expMap (0.06f, 1.2f, p ("dly_time") / 10.0f));
    delay.fb.setTargetValue (p ("dly_feedback") / 10.0f * 0.85f);
    delay.wet.setTargetValue (p ("dly_on") > 0.5f ? p ("dly_mix") / 10.0f * 0.85f : 0.0f);

    juce::dsp::Reverb::Parameters rp;
    const bool rOn = p ("rev_on") > 0.5f; const float rm = p ("rev_mix") / 10.0f;
    rp.roomSize = juce::jlimit (0.0f, 0.95f, 0.35f + p ("rev_size") * 0.06f);
    rp.damping = 0.45f; rp.width = 1.0f; rp.freezeMode = 0.0f;
    rp.wetLevel = rOn ? rm * 0.55f : 0.0f; rp.dryLevel = rOn ? 1.0f - rm * 0.25f : 1.0f;
    reverb.setParameters (rp);

    const float v = p ("out_volume");
    outGain.setTargetValue (v <= 0 ? 0.0f : dbToGain (-30.0f + v * 4.0f));
    muteGain.setTargetValue (tunerMute.load() ? 0.0f : 1.0f);
}

void ZojoRigProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples(), nIn = getTotalNumInputChannels(), nOut = getTotalNumOutputChannels();
    const int maxBlock = mono.getNumSamples();
    updateParams();
    const int inCh = (int) p ("input_ch"), cabType = (int) p ("cab_type");
    const bool useIR = cabType == 3 && irLoaded.load();
    if (! useIR && cabType != 4) cab.setType (cabType == 3 ? 2 : cabType);
    float inPk = 0, outPk = 0;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = std::min (maxBlock, total - start);
        float* m = mono.getWritePointer (0);

        // 1) input select, tuner tap, virtual tuning, gate, compressor
        int tw = tunerWrite.load();
        for (int i = 0; i < n; ++i)
        {
            float x = 0;
            if (nIn > 0)
            {
                const float a = buffer.getSample (0, start + i), b = nIn > 1 ? buffer.getSample (1, start + i) : a;
                x = inCh == 0 ? a : inCh == 1 ? b : (nIn > 1 ? a + b : a);
            }
            x *= inGain.getNextValue();
            inPk = std::max (inPk, std::abs (x));
            tunerRing[(size_t) tw] = x; tw = (tw + 1) % (int) tunerRing.size();

            const float tm = tuneMix.getNextValue();
            if (tm > 0.0f || tuneMix.getTargetValue() > 0.0f) { const float s = shifter.process (x); x += (s - x) * tm; }
            const float gw = gateMix.getNextValue();
            if (gw > 0.0f) { const float g = gate.process (x); x += (g - x) * gw; }
            const float cw = compMix.getNextValue();
            const float c = comp.processSample (0, x) * compMake.getNextValue();
            x += (c - x) * cw;
            m[i] = x;
        }
        tunerWrite.store (tw);

        // 2) overdrive, fuzz and amp run 4x oversampled to keep distortion smooth
        juce::dsp::AudioBlock<float> mb (mono);
        auto sub = mb.getSubBlock (0, (size_t) n);
        auto up = oversampling.processSamplesUp (sub);
        float* u = up.getChannelPointer (0);
        for (size_t j = 0; j < up.getNumSamples(); ++j)
        {
            float x = u[j];
            const float ow = odMix.getNextValue(), od = odDrive.getNextValue(), ol = odLevel.getNextValue();
            if (ow > 0.0f) { const float y = odTone.process (odCurve (odHP.process (x) * od)) * ol; x += (y - x) * ow; }
            const float fw = fzMix.getNextValue(), fd = fzDrive.getNextValue(), fl = fzLevel.getNextValue();
            if (fw > 0.0f) { const float y = fzTone.process (fzDC.process (fuzzCurve (fzHP.process (x) * fd))) * fl; x += (y - x) * fw; }
            u[j] = amp.process (x) * ampFade.getNextValue();
        }
        oversampling.processSamplesDown (sub);

        // 3) cabinet
        if (useIR) { juce::dsp::ProcessContextReplacing<float> ctx (sub); conv.process (ctx); }
        else if (cabType == 4) sub.multiplyBy (0.5f);
        else for (int i = 0; i < n; ++i) m[i] = cab.process (m[i]);

        // 4) EQ, doubler (mono -> stereo), chorus, delay
        float* L = stereo.getWritePointer (0); float* R = stereo.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            float x = m[i];
            const float ew = eqMix.getNextValue();
            if (ew > 0.0f) { float y = x; for (auto& b : eq) y = b.process (y); x += (y - x) * ew; }
            float l, r; doubler.process (x, l, r);
            chorus.process (l, r, chorusRate, chorusDepth);
            delay.process (l, r);
            L[i] = l; R[i] = r;
        }

        // 5) phaser + reverb (stereo blocks)
        juce::dsp::AudioBlock<float> sb (stereo);
        auto ssub = sb.getSubBlock (0, (size_t) n);
        { juce::dsp::ProcessContextReplacing<float> ctx (ssub); phaser.process (ctx); }
        { juce::dsp::ProcessContextReplacing<float> ctx (ssub); reverb.process (ctx); }

        // 6) output volume, tuner mute, meters
        for (int i = 0; i < n; ++i)
        {
            const float g = outGain.getNextValue() * muteGain.getNextValue();
            L[i] *= g; R[i] *= g;
            outPk = std::max (outPk, std::max (std::abs (L[i]), std::abs (R[i])));
        }
        if (nOut >= 2) { buffer.copyFrom (0, start, stereo, 0, 0, n); buffer.copyFrom (1, start, stereo, 1, 0, n); }
        else if (nOut == 1) { buffer.copyFrom (0, start, stereo, 0, 0, n); buffer.addFrom (0, start, stereo, 1, 0, n); buffer.applyGain (0, start, n, 0.5f); }
        for (int c = 2; c < nOut; ++c) buffer.clear (c, start, n);
    }
    inPeak.store (std::max (inPeak.load(), inPk));
    outPeak.store (std::max (outPeak.load(), outPk));
}

//==============================================================================
double ZojoRigProcessor::copyTunerSamples (float* dest, int n) const
{
    const int size = (int) tunerRing.size(); n = std::min (n, size);
    int r = tunerWrite.load() - n; if (r < 0) r += size;
    for (int i = 0; i < n; ++i) dest[i] = tunerRing[(size_t) ((r + i) % size)];
    return sr;
}

void ZojoRigProcessor::loadImpulseResponse (const juce::File& file)
{
    if (! file.existsAsFile()) return;
    conv.loadImpulseResponse (file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::yes, 0,
                              juce::dsp::Convolution::Normalise::yes);
    irFile = file; irLoaded = true;
    apvts.state.setProperty ("irPath", file.getFullPathName(), nullptr);
}

void ZojoRigProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ZojoRigProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            const juce::File ir (apvts.state.getProperty ("irPath").toString());
            if (ir.existsAsFile()) loadImpulseResponse (ir);
        }
}

//==============================================================================
float zojo::detectPitch (const float* buf, int n, double sampleRate)
{
    double rms = 0; for (int i = 0; i < n; ++i) rms += buf[i] * buf[i];
    rms = std::sqrt (rms / n); if (rms < 0.004) return -1;
    const int minLag = (int) (sampleRate / 1100), maxLag = std::min ((int) (sampleRate / 60), n - 2);
    std::vector<double> c ((size_t) maxLag + 2, 0.0);
    for (int lag = 0; lag <= maxLag + 1; ++lag) { double s = 0; for (int i = 0; i < n - lag; ++i) s += buf[i] * buf[i + lag]; c[(size_t) lag] = s; }
    int d = minLag; while (d < maxLag && c[(size_t) d] > 0) ++d;
    double maxV = -1e30; for (int i = d; i <= maxLag; ++i) maxV = std::max (maxV, c[(size_t) i]);
    if (maxV <= 0 || maxV < c[0] * 0.3) return -1;
    int T = -1;
    for (int i = d + 1; i <= maxLag; ++i)
        if (c[(size_t) i] >= 0.9 * maxV && c[(size_t) i] >= c[(size_t) i - 1] && c[(size_t) i] >= c[(size_t) i + 1]) { T = i; break; }
    if (T < 0) return -1;
    const double a = c[(size_t) T - 1], b = c[(size_t) T], g = c[(size_t) T + 1], den = a - 2 * b + g;
    const double shift = den != 0 ? 0.5 * (a - g) / den : 0;
    return (float) (sampleRate / (T + shift));
}

juce::AudioProcessorEditor* ZojoRigProcessor::createEditor() { return new ZojoRigEditor (*this); }

#if ! ZOJO_TESTS
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZojoRigProcessor(); }
#endif
