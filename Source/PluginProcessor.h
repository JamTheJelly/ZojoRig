#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Dsp.h"
#include "Params.h"

class ZojoRigProcessor : public juce::AudioProcessor
{
public:
    ZojoRigProcessor();
    ~ZojoRigProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ZOJO RIG"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- used by the editor ---
    juce::AudioProcessorValueTreeState apvts;
    void loadImpulseResponse (const juce::File& file);
    juce::String getIRName() const { return irFile.getFileName(); }
    bool hasIR() const { return irLoaded.load(); }

    std::atomic<float> inPeak { 0 }, outPeak { 0 };
    std::atomic<bool> tunerMute { false };
    // copy the latest input samples (for the tuner); returns sample rate
    double copyTunerSamples (float* dest, int n) const;

    juce::File irFile;

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    void updateParams();

    double sr = 48000;
    int lastModel = -1;
    bool tuningActive = false;

    zojo::NoiseGate gate;
    juce::dsp::Compressor<float> comp;
    zojo::PitchShifter shifter;
    zojo::Biquad odHP, odTone, fzHP, fzDC, fzTone;
    zojo::AmpSim amp;
    zojo::CabSim cab;
    juce::dsp::Convolution conv;
    std::atomic<bool> irLoaded { false };
    std::array<zojo::Biquad, 5> eq;
    zojo::Doubler doubler;
    zojo::Chorus chorus;
    juce::dsp::Phaser<float> phaser;
    zojo::Delay delay;
    juce::dsp::Reverb reverb;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };

    juce::SmoothedValue<float> inGain, outGain, tuneMix, gateMix, compMix, compMake, odMix, odDrive, odLevel,
                               fzMix, fzDrive, fzLevel, eqMix, ampFade, muteGain;
    float chorusRate = 1, chorusDepth = 0.002f;

    juce::AudioBuffer<float> mono, stereo;
    std::vector<float> tunerRing; std::atomic<int> tunerWrite { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZojoRigProcessor)
};
