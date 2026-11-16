#include "Player.h"
#include <signalsmith-stretch/signalsmith-stretch.h>

struct FilePlayer::Impl
{
    explicit Impl (FilePlayer& o) : owner (o) { formats.registerBasicFormats(); }

    FilePlayer& owner;
    juce::AudioFormatManager formats;
    juce::SpinLock lock;
    std::unique_ptr<juce::AudioBuffer<float>> audio;   // stereo, at deviceRate
    double deviceRate = 48000, audioRate = 0;
    int maxBlock = 512;

    signalsmith::stretch::SignalsmithStretch<float> stretch;
    juce::AudioBuffer<float> inTmp, outTmp;
    double readPos = 0;        // next input sample to feed (in audio buffer samples)
    double inFrac = 0;         // fractional accumulator for input block lengths
    float lastSemis = 1000.0f;
    bool needsReset = true;
    juce::SmoothedValue<float> gainSmooth { 0.0f };
    juce::uint32 loadGeneration = 0;

    void readInto (int ch, float* dest, juce::int64 start, int n) const
    {
        const auto len = (juce::int64) audio->getNumSamples();
        const float* src = audio->getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const auto p = start + i;
            dest[i] = (p >= 0 && p < len) ? src[p] : 0.0f;
        }
    }

    // reposition the stretcher so the next processed input starts at sample `pos`
    void jumpTo (double pos, double rate)
    {
        const int pre = std::min ((int) pos, stretch.blockSamples() + stretch.intervalSamples());
        if (pre > 0)
        {
            for (int c = 0; c < 2; ++c) readInto (c, inTmp.getWritePointer (c), (juce::int64) pos - pre, pre);
            float* in[2] = { inTmp.getWritePointer (0), inTmp.getWritePointer (1) };
            stretch.seek (in, pre, rate);
        }
        readPos = pos;
    }
};

FilePlayer::FilePlayer() : impl (std::make_unique<Impl> (*this)) {}
FilePlayer::~FilePlayer()
{
    // wait for any background load to finish touching `impl`
    for (int i = 0; i < 200 && loading.load(); ++i) juce::Thread::sleep (10);
}

void FilePlayer::prepare (double sampleRate, int maxBlockSize)
{
    auto& m = *impl;
    const bool rateChanged = std::abs (sampleRate - m.deviceRate) > 0.5;
    {
        const juce::SpinLock::ScopedLockType sl (m.lock);
        m.deviceRate = sampleRate; m.maxBlock = std::max (32, maxBlockSize);
        m.stretch.presetDefault (2, (float) sampleRate, true);
        m.inTmp.setSize (2, m.stretch.blockSamples() + m.stretch.intervalSamples() + m.maxBlock * 2 + 64);
        m.outTmp.setSize (2, m.maxBlock);
        m.gainSmooth.reset (sampleRate, 0.03);
        m.needsReset = true; m.lastSemis = 1000.0f;
    }
    if (rateChanged && file.existsAsFile() && m.audioRate > 0 && std::abs (m.audioRate - sampleRate) > 0.5)
        load (file);  // re-decode at the new device rate
}

void FilePlayer::load (const juce::File& f)
{
    playing = false;
    file = f; error.clear();
    loading = true;
    const auto generation = ++impl->loadGeneration;
    const double targetRate = impl->deviceRate;
    juce::Thread::launch ([this, f, generation, targetRate]
    {
        auto& m = *impl;
        std::unique_ptr<juce::AudioFormatReader> reader (m.formats.createReaderFor (f));
        std::unique_ptr<juce::AudioBuffer<float>> buf;
        juce::String err;
        if (reader == nullptr) err = "Can't read this file type";
        else if (reader->lengthInSamples <= 0) err = "File is empty";
        else if (reader->lengthInSamples > (juce::int64) (reader->sampleRate * 60 * 20)) err = "File is longer than 20 minutes";
        else
        {
            const int n = (int) reader->lengthInSamples;
            juce::AudioBuffer<float> raw (2, n);
            reader->read (&raw, 0, n, 0, true, reader->numChannels > 1);
            if (reader->numChannels == 1) raw.copyFrom (1, 0, raw, 0, 0, n);
            const double ratio = reader->sampleRate / targetRate;   // input samples per output sample
            if (std::abs (ratio - 1.0) < 1e-6) buf = std::make_unique<juce::AudioBuffer<float>> (std::move (raw));
            else
            {
                const int outN = (int) std::floor ((double) n / ratio);
                buf = std::make_unique<juce::AudioBuffer<float>> (2, outN);
                for (int c = 0; c < 2; ++c)
                {
                    juce::WindowedSincInterpolator interp;
                    interp.process (ratio, raw.getReadPointer (c), buf->getWritePointer (c), outN, n, 0);
                }
            }
        }
        if (generation == m.loadGeneration)
        {
            {
                const juce::SpinLock::ScopedLockType sl (m.lock);
                std::swap (m.audio, buf);
                m.audioRate = targetRate;
                m.needsReset = true;
                lengthSeconds = m.audio != nullptr ? (double) m.audio->getNumSamples() / targetRate : 0.0;
                positionSeconds = 0; seekRequest = 0; loopA = 0; loopB = 0;
            }
            error = err;
        }
        loading = false;
    });
}

void FilePlayer::process (float* L, float* R, int n, float speed, float semitones, float gain)
{
    auto& m = *impl;
    const juce::SpinLock::ScopedTryLockType sl (m.lock);
    if (! sl.isLocked() || m.audio == nullptr) return;
    m.gainSmooth.setTargetValue (playing.load() ? gain : 0.0f);
    if (! playing.load() && ! m.gainSmooth.isSmoothing() && m.gainSmooth.getCurrentValue() == 0.0f) return;

    const double rate = juce::jlimit (0.25, 2.0, (double) speed);
    const auto total = (double) m.audio->getNumSamples();

    if (m.needsReset) { m.stretch.reset(); m.needsReset = false; m.inFrac = 0; m.jumpTo (0, rate); }
    const double seek = seekRequest.exchange (-1.0);
    if (seek >= 0) { m.stretch.reset(); m.jumpTo (std::min (seek * m.deviceRate, total - 1), rate); }
    if (semitones != m.lastSemis) { m.stretch.setTransposeSemitones (semitones); m.lastSemis = semitones; }

    for (int done = 0; done < n;)
    {
        const int chunk = std::min (n - done, m.maxBlock);
        m.inFrac += chunk * rate;
        int inN = (int) m.inFrac; m.inFrac -= inN;

        // A-B loop / whole-song loop / end of file
        const double a = loopA.load() * m.deviceRate, b = loopB.load() * m.deviceRate;
        const bool abLoop = b > a + m.deviceRate * 0.2;
        const double end = abLoop ? std::min (b, total) : total;
        if (playing.load() && m.readPos + inN > end)
        {
            if (abLoop || loopAll.load()) m.jumpTo (abLoop ? a : 0.0, rate);
            else if (m.readPos >= total + m.stretch.inputLatency()) { playing = false; seekRequest = 0; }
        }

        for (int c = 0; c < 2; ++c) m.readInto (c, m.inTmp.getWritePointer (c), (juce::int64) m.readPos, inN);
        float* in[2] = { m.inTmp.getWritePointer (0), m.inTmp.getWritePointer (1) };
        float* out[2] = { m.outTmp.getWritePointer (0), m.outTmp.getWritePointer (1) };
        m.stretch.process (in, inN, out, chunk);
        m.readPos += inN;

        for (int i = 0; i < chunk; ++i)
        {
            const float g = m.gainSmooth.getNextValue();
            L[done + i] += out[0][i] * g;
            R[done + i] += out[1][i] * g;
        }
        done += chunk;
    }
    const double heard = m.readPos - m.stretch.inputLatency() - rate * m.stretch.outputLatency();
    positionSeconds = juce::jlimit (0.0, total, heard) / m.deviceRate;
}
