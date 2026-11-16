#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>

// Plays an audio file underneath the guitar with independent speed (pitch kept) and key (tempo kept),
// plus A-B looping. Decoding happens on a background thread; process() is real-time safe.
class FilePlayer
{
public:
    FilePlayer();
    ~FilePlayer();

    // --- message thread ---
    void prepare (double sampleRate, int maxBlockSize);
    void load (const juce::File&);
    juce::File getFile() const { return file; }
    bool isLoading() const { return loading.load(); }
    juce::String getError() const { return error; }
    double getLengthSeconds() const { return lengthSeconds.load(); }
    double getPositionSeconds() const { return positionSeconds.load(); }
    void setPlaying (bool p) { playing = p && lengthSeconds.load() > 0; }
    bool isPlaying() const { return playing.load(); }
    void seekTo (double seconds) { seekRequest = std::max (0.0, seconds); }
    void setLoopPoints (double a, double b) { loopA = a; loopB = b; }   // b <= a disables
    double getLoopA() const { return loopA.load(); }
    double getLoopB() const { return loopB.load(); }
    void setLoopWholeSong (bool l) { loopAll = l; }

    // --- audio thread ---
    // speed: 0.5 .. 1.25 (1 = original), semitones: key change, gain: linear. Adds into L/R.
    void process (float* L, float* R, int numSamples, float speed, float semitones, float gain);

    static juce::String supportedWildcard() { return "*.mp3;*.wav;*.flac;*.ogg;*.aif;*.aiff;*.m4a;*.wma"; }

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
    juce::File file;
    juce::String error;
    std::atomic<bool> loading { false }, playing { false }, loopAll { false };
    std::atomic<double> lengthSeconds { 0 }, positionSeconds { 0 }, seekRequest { -1 }, loopA { 0 }, loopB { 0 };
    friend struct Impl;
};
