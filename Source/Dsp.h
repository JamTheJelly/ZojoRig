#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

// ZOJO RIG signal-processing building blocks (ported from the browser prototype).
namespace zojo
{
constexpr float kPi = 3.14159265358979f;
inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
inline float expMap (float a, float b, float t) { return a * std::pow (b / a, t); }

//==============================================================================
// Waveshaper curves (input clamped to [-1, 1], like the browser's WaveShaperNode)
inline float clamp1 (float x) { return x > 1.0f ? 1.0f : (x < -1.0f ? -1.0f : x); }
inline float tubeCurve (float x)
{
    x = clamp1 (x);
    constexpr float k = 2.5f, b = 0.12f;
    static const float off = std::tanh (k * b);
    return std::tanh (k * (x + b)) - off;
}
inline float powerCurve (float x) { x = clamp1 (x); static const float n = std::tanh (1.3f); return std::tanh (1.3f * x) / n; }
inline float odCurve (float x)    { x = clamp1 (x); return (2.0f / kPi) * std::atan (4.0f * x); }
inline float fuzzCurve (float x)  { x = clamp1 (x); const float y = x * 4.0f; return y > 0 ? std::min (y, 0.75f) : std::max (y, -0.55f); }

//==============================================================================
// Biquad that only recomputes coefficients when its settings change (no allocation).
struct Biquad
{
    enum Type { LP, HP, Peak, LowShelf, HighShelf, AllPass };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        filter.coefficients = new juce::dsp::IIR::Coefficients<float> (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        dirty = true; update(); filter.reset();
    }
    void set (Type t, float f, float q = 0.707f, float gainDb = 0.0f)
    {
        if (t != type || f != freq || q != Q || gainDb != db) { type = t; freq = f; Q = q; db = gainDb; dirty = true; }
    }
    void update()
    {
        if (! dirty || filter.coefficients == nullptr) return;
        dirty = false;
        using A = juce::dsp::IIR::ArrayCoefficients<float>;
        const float f = juce::jlimit (10.0f, (float) (sr * 0.45), freq);
        const float g = dbToGain (db);
        switch (type)
        {
            case LP:        *filter.coefficients = A::makeLowPass (sr, f, Q); break;
            case HP:        *filter.coefficients = A::makeHighPass (sr, f, Q); break;
            case Peak:      *filter.coefficients = A::makePeakFilter (sr, f, Q, g); break;
            case LowShelf:  *filter.coefficients = A::makeLowShelf (sr, f, Q, g); break;
            case HighShelf: *filter.coefficients = A::makeHighShelf (sr, f, Q, g); break;
            case AllPass:   *filter.coefficients = A::makeAllPass (sr, f, Q); break;
        }
    }
    inline float process (float x) { return filter.processSample (x); }
    void reset() { filter.reset(); }

    juce::dsp::IIR::Filter<float> filter;
    double sr = 48000.0;
    Type type = Peak;
    float freq = 1000.0f, Q = 0.707f, db = 0.0f;
    bool dirty = true;
};

//==============================================================================
// Simple fractional delay line (linear interpolation)
struct DelayLine
{
    void prepare (int maxSamples) { buf.assign ((size_t) maxSamples + 4, 0.0f); w = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) % (int) buf.size(); }
    inline float read (float delaySamples) const
    {
        const int n = (int) buf.size();
        float pos = (float) w - 1.0f - delaySamples;
        while (pos < 0) pos += (float) n;
        const int i = (int) pos; const float f = pos - (float) i;
        return buf[(size_t) (i % n)] * (1.0f - f) + buf[(size_t) ((i + 1) % n)] * f;
    }
    std::vector<float> buf; int w = 0;
};

//==============================================================================
// Noise gate with hysteresis (fast attack, adjustable release)
struct NoiseGate
{
    void prepare (double s) { sr = s; env = g = 0; open = false; }
    void setParams (float thresholdDb, float releaseSec)
    {
        th = dbToGain (thresholdDb); thClose = th * 0.6f;
        aAtt = std::exp (-1.0f / (float) (0.0015 * sr));
        aRel = std::exp (-1.0f / (float) (std::max (0.01f, releaseSec) * sr));
        envRel = std::exp (-1.0f / (float) (0.03 * sr));
    }
    inline float process (float x)
    {
        const float a = std::abs (x);
        env = a > env ? a : env * envRel;
        if (env > th) open = true; else if (env < thClose) open = false;
        const float t = open ? 1.0f : 0.0f, c = t > g ? aAtt : aRel;
        g = c * g + (1.0f - c) * t;
        return x * g;
    }
    double sr = 48000; float env = 0, g = 0, th = 0.001f, thClose = 0.0006f, aAtt = 0.9f, aRel = 0.999f, envRel = 0.999f; bool open = false;
};

//==============================================================================
// Pitch shifter: delay-line with two crossfaded read heads; each restarting head is
// phase-aligned with the other by a correlation search (WSOLA-style) so pitch stays exact.
struct PitchShifter
{
    void prepare (double sr)
    {
        buf.assign (N, 0.0f); w = 0;
        T = (int) (sr * 0.03); S = (int) (sr / 65.0); L = (int) (sr * 0.012);
        d[0] = d[1] = 2.0; age[0] = 0.0; age[1] = T * 0.5;
    }
    void setSemitones (float s) { ratio = std::pow (2.0, s / 12.0); }

    inline float at (int dd) const { int i = (w - 1 - dd) % N; if (i < 0) i += N; return buf[(size_t) i]; }
    inline float read (double dd) const
    {
        double pos = (double) w - 1.0 - dd; while (pos < 0) pos += N;
        const int i = (int) pos; const float f = (float) (pos - i);
        return buf[(size_t) (i % N)] * (1.0f - f) + buf[(size_t) ((i + 1) % N)] * f;
    }
    double pick (double dOther) const
    {
        const int base = ratio > 1.0 ? (int) std::ceil ((ratio - 1.0) * T) + 2 : 2;
        const int dO = (int) std::lround (dOther);
        int best = base; double bestC = -1e30;
        auto corr = [&] (int dc) { double c = 0; for (int j = 0; j < L; j += 2) c += (double) at (dc + j) * at (dO + j); return c; };
        for (int k = 0; k < S; k += 2) { const double c = corr (base + k); if (c > bestC) { bestC = c; best = base + k; } }
        for (int dc : { best - 1, best + 1 }) if (dc >= base) { const double c = corr (dc); if (c > bestC) { bestC = c; best = dc; } }
        return best + (dOther - dO);
    }
    inline float process (float x)
    {
        buf[(size_t) w] = x; w = (w + 1) % N;
        float y = 0;
        for (int h = 0; h < 2; ++h)
        {
            age[h] += 1.0;
            if (age[h] >= T) { age[h] -= T; d[h] = pick (d[1 - h]); }
            const float g = std::sin (kPi * (float) (age[h] / T));
            y += read (d[h]) * g * g;
        }
        d[0] = std::max (0.0, d[0] + 1.0 - ratio);
        d[1] = std::max (0.0, d[1] + 1.0 - ratio);
        return y;
    }

    static constexpr int N = 32768;
    std::vector<float> buf; int w = 0, T = 1440, S = 738, L = 576;
    double d[2] { 2, 2 }, age[2] { 0, 720 }, ratio = 1.0;
};

//==============================================================================
// Amp models
struct VoiceFilter { Biquad::Type type; float f, q, db; };
struct AmpModel
{
    const char* name; const char* desc; juce::uint32 colour;
    float hp, driveMin, driveMax; int stages; float stage2, lp;
    float bassF, midF, midQ, trebF;
    std::vector<VoiceFilter> voice;
};
const std::vector<AmpModel>& ampModels();

struct AmpSim
{
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto* b : all()) b->prepare (sr);
        for (auto& b : voice) b.prepare (sr);
        g1.reset (sr, 0.02); comp.reset (sr, 0.02); master.reset (sr, 0.02);
    }
    std::vector<Biquad*> all() { return { &hp, &dc1, &lp1, &dc2, &lp2, &bass, &mid, &treb, &pres }; }
    void setModel (int index)
    {
        const auto& ms = ampModels(); model = juce::jlimit (0, (int) ms.size() - 1, index);
        const auto& m = ms[(size_t) model];
        hp.set (Biquad::HP, m.hp, 0.7f); dc1.set (Biquad::HP, 25, 0.7f); lp1.set (Biquad::LP, m.lp, 0.7f);
        dc2.set (Biquad::HP, 40, 0.7f); lp2.set (Biquad::LP, m.lp * 1.3f, 0.7f);
        for (size_t i = 0; i < voice.size(); ++i)
            if (i < m.voice.size()) voice[i].set (m.voice[i].type, m.voice[i].f, m.voice[i].q, m.voice[i].db);
            else voice[i].set (Biquad::Peak, 1000, 0.7f, 0);
        for (auto* b : all()) { b->update(); b->reset(); }
        for (auto& b : voice) { b.update(); b.reset(); }
    }
    // knob values are 0..10 like the prototype
    void setKnobs (float gain, float b, float mi, float t, float pr, float ma)
    {
        const auto& m = ampModels()[(size_t) model];
        const float drive = expMap (m.driveMin, m.driveMax, gain / 10.0f);
        g1.setTargetValue (drive);
        comp.setTargetValue (m.stages == 2 ? 0.55f : 0.6f / std::max (0.08f, std::tanh (0.375f * drive)));
        bass.set (Biquad::LowShelf, m.bassF, 0.707f, (b - 5.0f) * 3.0f);
        mid.set (Biquad::Peak, m.midF, m.midQ, (mi - 5.0f) * 2.4f);
        treb.set (Biquad::HighShelf, m.trebF, 0.707f, (t - 5.0f) * 3.0f);
        pres.set (Biquad::HighShelf, 4500, 0.707f, (pr - 5.0f) * 2.0f);
        master.setTargetValue (ma <= 0 ? 0.0f : 0.08f * std::pow (40.0f, ma / 10.0f));
        for (auto* bq : all()) bq->update();
    }
    inline float process (float x)
    {
        const auto& m = ampModels()[(size_t) model];
        float y = hp.process (x) * g1.getNextValue();
        y = lp1.process (dc1.process (tubeCurve (y)));
        if (m.stages == 2) y = lp2.process (dc2.process (tubeCurve (y * m.stage2)));
        y *= comp.getNextValue();
        y = pres.process ([&] { float v = treb.process (mid.process (bass.process (y))); for (auto& vf : voice) v = vf.process (v); return v; }());
        return powerCurve (y * master.getNextValue()) * 0.8f;
    }
    double sr = 48000; int model = 1;
    Biquad hp, dc1, lp1, dc2, lp2, bass, mid, treb, pres;
    std::array<Biquad, 3> voice;
    juce::SmoothedValue<float> g1 { 1.0f }, comp { 1.0f }, master { 0.5f };
};

//==============================================================================
// Speaker cabinet simulation (filter voicings) — IR convolution lives in the processor
struct CabSim
{
    void prepare (double sr) { for (auto& b : f) b.prepare (sr); }
    void setType (int t)
    {
        static const float specs[3][6][4] = {
            { { 1, 90, .7f, 0 }, { 2, 130, 1.2f, 3 }, { 2, 1800, 1.5f, 2 }, { 2, 3200, 2, 3 }, { 0, 5600, .8f, 0 }, { 0, 6800, .6f, 0 } },
            { { 1, 80, .7f, 0 }, { 2, 110, 1.2f, 4 }, { 2, 700, 1.2f, -2 }, { 2, 2600, 1.8f, 3 }, { 0, 5000, .8f, 0 }, { 0, 6200, .6f, 0 } },
            { { 1, 70, .8f, 0 }, { 2, 105, 1.4f, 5 }, { 2, 450, 1, -3 }, { 2, 2200, 1.6f, 4 }, { 0, 4300, .9f, 0 }, { 0, 5300, .6f, 0 } } };
        t = juce::jlimit (0, 2, t);
        if (t == current) return;
        current = t;
        for (int i = 0; i < 6; ++i)
        {
            const auto* s = specs[t][i];
            f[(size_t) i].set (s[0] == 0 ? Biquad::LP : s[0] == 1 ? Biquad::HP : Biquad::Peak, s[1], s[2], s[3]);
            f[(size_t) i].update();
        }
    }
    inline float process (float x) { for (auto& b : f) x = b.process (x); return x; }
    std::array<Biquad, 6> f; int current = -1;
};

//==============================================================================
// Mono -> stereo doubler
struct Doubler
{
    void prepare (double s) { sr = s; dl.prepare ((int) (sr * 0.1)); tone.prepare (sr); tone.set (Biquad::LP, 7000, 0.6f); tone.update(); width.reset (sr, 0.05); time.reset (sr, 0.1); }
    inline void process (float x, float& l, float& r)
    {
        dl.push (x);
        phase += 0.37f / (float) sr; if (phase >= 1) phase -= 1;
        const float dMs = time.getNextValue() + 0.7f * std::sin (2 * kPi * phase);
        const float wet = tone.process (dl.read (dMs * 0.001f * (float) sr));
        const float w = width.getNextValue();
        l = x; r = x * (1.0f - 0.95f * w) + wet * w;
    }
    double sr = 48000; DelayLine dl; Biquad tone; float phase = 0;
    juce::SmoothedValue<float> width { 0.0f }, time { 20.0f };
};

//==============================================================================
// Stereo chorus (quadrature LFO for width)
struct Chorus
{
    void prepare (double s) { sr = s; for (auto& d : dl) d.prepare ((int) (sr * 0.1)); mix.reset (sr, 0.03); }
    inline void process (float& l, float& r, float rateHz, float depth)
    {
        phase += rateHz / (float) sr; if (phase >= 1) phase -= 1;
        const float m = mix.getNextValue();
        float* io[2] = { &l, &r };
        for (int c = 0; c < 2; ++c)
        {
            dl[(size_t) c].push (*io[c]);
            const float dSec = 0.014f + depth * std::sin (2 * kPi * phase + (float) c * kPi * 0.5f);
            const float wet = dl[(size_t) c].read (dSec * (float) sr);
            *io[c] = *io[c] * (1.0f - m * 0.35f) + wet * m * 0.9f;
        }
    }
    double sr = 48000; std::array<DelayLine, 2> dl; float phase = 0;
    juce::SmoothedValue<float> mix { 0.0f };
};

//==============================================================================
// Stereo delay with darkening feedback
struct Delay
{
    void prepare (double s)
    {
        sr = s; for (auto& d : dl) d.prepare ((int) (sr * 2.1));
        for (auto& b : lp) { b.prepare (sr); b.set (Biquad::LP, 3200, 0.5f); b.update(); }
        for (auto& b : hp) { b.prepare (sr); b.set (Biquad::HP, 120, 0.5f); b.update(); }
        time.reset (sr, 0.08); fb.reset (sr, 0.03); wet.reset (sr, 0.03);
    }
    inline void process (float& l, float& r)
    {
        const float t = time.getNextValue() * (float) sr, f = fb.getNextValue(), m = wet.getNextValue();
        float* io[2] = { &l, &r };
        for (int c = 0; c < 2; ++c)
        {
            const float d = dl[(size_t) c].read (t);
            dl[(size_t) c].push (*io[c] + hp[(size_t) c].process (lp[(size_t) c].process (d)) * f);
            *io[c] += d * m;
        }
    }
    double sr = 48000; std::array<DelayLine, 2> dl; std::array<Biquad, 2> lp, hp;
    juce::SmoothedValue<float> time { 0.3f }, fb { 0.3f }, wet { 0.0f };
};

//==============================================================================
// Monophonic pitch detection for the tuner (autocorrelation + parabolic interpolation)
float detectPitch (const float* buf, int n, double sampleRate);

} // namespace zojo
