#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/ZojoLookAndFeel.h"
#include "UI/Widgets.h"

//==============================================================================
class AmpPanel : public juce::Component
{
public:
    explicit AmpPanel (ZojoRigProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
private:
    ZojoRigProcessor& proc;
    juce::OwnedArray<juce::TextButton> models;
    std::vector<std::unique_ptr<ZKnob>> knobs;
    int shownModel = -1;
};

class CabPanel : public juce::Component
{
public:
    explicit CabPanel (ZojoRigProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
private:
    ZojoRigProcessor& proc;
    juce::OwnedArray<juce::TextButton> types;
    juce::TextButton loadIR { "Load IR..." };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String irText;
};

//==============================================================================
class RigContent : public juce::Component, private juce::Timer
{
public:
    explicit RigContent (ZojoRigProcessor&);
    ~RigContent() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int baseW = 1280, baseH = 1006;
private:
    void timerCallback() override;
    void rebuildPresetMenu();
    void savePresetDialog();
    void sectionLabel (juce::Graphics&, int y, const juce::String& num, const juce::String& title, const juce::String& sub);

    ZojoRigProcessor& proc;
    juce::ComboBox presetBox, inputChannel;
    juce::TextButton savePreset { "Save" }, deletePreset { "Delete" }, tunerButton { "Tuner" }, audioButton { "Audio settings" };
    juce::OwnedArray<juce::TextButton> tuningButtons;
    std::unique_ptr<APVTS::ComboBoxAttachment> inputChannelAttachment;
    std::unique_ptr<ZKnob> inputKnob, volumeKnob;
    DotMeter inMeter, outMeter;
    std::vector<std::unique_ptr<Pedal>> pre, post;
    AmpPanel amp;
    CabPanel cab;
    TunerOverlay tuner;
    std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::Rectangle<int> ioArea, tuningArea, inMeterArea, outMeterArea;
    juce::String currentPresetName;
    int lastTuning = -1;
};

//==============================================================================
class ZojoRigEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZojoRigEditor (ZojoRigProcessor&);
    ~ZojoRigEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    ZojoLookAndFeel lnf;
    RigContent content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZojoRigEditor)
};
