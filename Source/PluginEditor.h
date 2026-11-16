#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/ZojoLookAndFeel.h"
#include "UI/Widgets.h"
#include "UI/PlayerPanel.h"
#include "UI/Flames.h"

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
class RigContent : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit RigContent (ZojoRigProcessor&);
    ~RigContent() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    static constexpr int baseW = 1280, baseH = 1240;
private:
    void timerCallback() override;
    void rebuildPresetMenu();
    void savePresetDialog();
    void updateTheme();
    void repaintFireRegions();
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
    PlayerPanel playerPanel;
    TunerOverlay tuner;
    std::unique_ptr<juce::AlertWindow> saveWindow;
    juce::Rectangle<int> ioArea, tuningArea, inMeterArea, outMeterArea;
    juce::String currentPresetName;
    int lastTuning = -1;

    // Fern Burn theme
    bool burn = false;
    zc::FireField bgFire, headerFire;
    float glint = -1.0f;
    int frame = 0;
};

//==============================================================================
class ZojoRigEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ZojoRigEditor (ZojoRigProcessor&);
    ~ZojoRigEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void buildContent();
    ZojoRigProcessor& proc;
    ZojoLookAndFeel lnf;
    std::unique_ptr<RigContent> content;
    juce::String palette;
    Toast toast;
    int lastToast = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ZojoRigEditor)
};
