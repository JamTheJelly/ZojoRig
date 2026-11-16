#include "PluginEditor.h"
#include "Presets.h"
#if defined (JucePlugin_Build_Standalone) && JucePlugin_Build_Standalone
 #include <juce_audio_utils/juce_audio_utils.h>
 #include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
 #define ZOJO_HAS_STANDALONE 1
#else
 #define ZOJO_HAS_STANDALONE 0
#endif

namespace
{
int getChoice (APVTS& s, const char* id) { return (int) s.getRawParameterValue (id)->load(); }
void setChoice (APVTS& s, const char* id, int index)
{
    if (auto* p = s.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
}
bool isStandalone()
{
   #if ZOJO_HAS_STANDALONE
    return juce::JUCEApplicationBase::isStandaloneApp() && juce::StandalonePluginHolder::getInstance() != nullptr;
   #else
    return false;
   #endif
}
void openAudioSettings()
{
   #if ZOJO_HAS_STANDALONE
    if (isStandalone()) juce::StandalonePluginHolder::getInstance()->showAudioSettingsDialog();
   #endif
}
void unmuteStandaloneInput()
{
   #if ZOJO_HAS_STANDALONE
    if (isStandalone()) juce::StandalonePluginHolder::getInstance()->getMuteInputValue().setValue (false);
   #endif
}
}

//==============================================================================
AmpPanel::AmpPanel (ZojoRigProcessor& p) : proc (p)
{
    const auto& ms = zojo::ampModels();
    for (int i = 0; i < (int) ms.size(); ++i)
    {
        auto* b = models.add (new juce::TextButton (ms[(size_t) i].name));
        b->getProperties().set ("chip", true);
        b->getProperties().set ("accent", (juce::int64) zc::white.getARGB());
        b->onClick = [this, i] { setChoice (proc.apvts, "amp_model", i); refresh(); };
        addAndMakeVisible (b);
    }
    const struct { const char* id; const char* label; } ks[] = { { "amp_gain", "Gain" }, { "amp_bass", "Bass" }, { "amp_mid", "Middle" },
                                                                  { "amp_treble", "Treble" }, { "amp_presence", "Presence" }, { "amp_master", "Master" } };
    for (auto& k : ks)
    {
        const bool gain = juce::String (k.id) == "amp_gain";
        knobs.push_back (std::make_unique<ZKnob> (proc.apvts, k.id, k.label, gain ? zc::red : juce::Colour (0xffefefea),
                                                  gain ? juce::Colours::white : juce::Colour (0xff0b0b0c)));
        knobs.back()->setOffColour (juce::Colour (0xff232326));
        knobs.back()->setTextColours (juce::Colour (0xffe9e9e4), juce::Colour (0xff7c7c76));
        addAndMakeVisible (*knobs.back());
    }
    refresh();
}

void AmpPanel::refresh()
{
    const int m = getChoice (proc.apvts, "amp_model");
    for (int i = 0; i < models.size(); ++i) models[i]->setToggleState (i == m, juce::dontSendNotification);
    if (m != shownModel) { shownModel = m; repaint(); }
}

void AmpPanel::resized()
{
    auto r = getLocalBounds().reduced (20, 16);
    r.removeFromTop (78);
    auto chips = r.removeFromTop (26);
    juce::FlexBox fb; fb.flexWrap = juce::FlexBox::Wrap::wrap;
    for (auto* b : models) fb.items.add (juce::FlexItem (*b).withWidth (zc::textWidth (zc::mono (9.0f, true), b->getButtonText().toUpperCase()) + 26).withHeight (24).withMargin ({ 0, 6, 0, 0 }));
    fb.performLayout (chips);
    r.removeFromTop (12);
    auto kr = r.removeFromTop (92);
    const int kw = kr.getWidth() / (int) knobs.size();
    for (size_t i = 0; i < knobs.size(); ++i) knobs[i]->setBounds (kr.getX() + (int) i * kw + (kw - 70) / 2, kr.getY(), 70, 90);
}

void AmpPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    juce::Path p; p.addRoundedRectangle (r, 18);
    juce::DropShadow (zc::red.withAlpha (0.18f), 40, {}).drawForPath (g, p);
    g.setColour (juce::Colour (0xf0050505)); g.fillPath (p);
    g.saveState(); g.reduceClipRegion (p);
    zc::drawDotGrid (g, r, 10.0f, 1.1f, juce::Colour (0xff1d1d1d));
    g.restoreState();
    g.setColour (juce::Colour (0xff222226)); g.strokePath (p, juce::PathStrokeType (1));

    const auto& m = zojo::ampModels()[(size_t) juce::jlimit (0, (int) zojo::ampModels().size() - 1, shownModel)];
    const auto col = juce::Colour (m.colour);
    juce::Rectangle<float> tagDot (21, 21, 7, 7);
    juce::Path td; td.addEllipse (tagDot); juce::DropShadow (col, 8, {}).drawForPath (g, td); g.setColour (col); g.fillPath (td);
    g.setColour (juce::Colour (0xff7c7c76)); g.setFont (zc::mono (9.0f));
    g.drawText ("AMP MODEL", 34, 17, 200, 14, juce::Justification::centredLeft);
    zc::drawDotMatrixText (g, juce::String (m.name).toUpperCase(), { 18.0f, 34.0f, r.getWidth() - 36, 62.0f }, 54.0f, zc::white, 3.4f);

    g.setColour (juce::Colour (0xff2c2c2a));
    const float y = 126; for (float x = 20; x < r.getRight() - 20; x += 6) g.drawHorizontalLine ((int) y, x, x + 3);
    g.setColour (juce::Colour (0xff8b8b85)); g.setFont (zc::mono (11.0f));
    g.drawText (m.desc, 20, getHeight() - 44, getWidth() - 40, 16, juce::Justification::centredLeft);
    zc::drawDotGrid (g, { 20.0f, (float) getHeight() - 24, (float) getWidth() - 40, 12.0f }, 7.0f, 1.6f, juce::Colour (0xff2e2e2c));
}

//==============================================================================
CabPanel::CabPanel (ZojoRigProcessor& p) : proc (p)
{
    const juce::StringArray labels { "1x12 Open", "2x12", "4x12 Closed", "IR file", "Off (DI)" };
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = types.add (new juce::TextButton (labels[i]));
        b->onClick = [this, i]
        {
            if (i == 3 && ! proc.hasIR()) { loadIR.triggerClick(); return; }
            setChoice (proc.apvts, "cab_type", i); refresh();
        };
        addAndMakeVisible (b);
    }
    loadIR.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a speaker impulse response", juce::File(), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f.existsAsFile()) { proc.loadImpulseResponse (f); setChoice (proc.apvts, "cab_type", 3); refresh(); }
        });
    };
    addAndMakeVisible (loadIR);
    refresh();
}

void CabPanel::refresh()
{
    const int t = getChoice (proc.apvts, "cab_type");
    for (int i = 0; i < types.size(); ++i) types[i]->setToggleState (i == t, juce::dontSendNotification);
    const auto txt = proc.hasIR() ? "IR: " + proc.getIRName() : juce::String ("Load a speaker impulse response (.wav) for the most realistic sound.");
    if (txt != irText) { irText = txt; repaint(); }
}

void CabPanel::resized()
{
    auto r = getLocalBounds().reduced (16);
    r.removeFromTop (38);
    auto grid = r.removeFromTop (76);
    const int bw = (grid.getWidth() - 12) / 3;
    for (int i = 0; i < 6; ++i)
    {
        juce::Rectangle<int> cell (grid.getX() + (i % 3) * (bw + 6), grid.getY() + (i / 3) * 40, bw, 34);
        if (i < types.size()) types[i]->setBounds (cell); else loadIR.setBounds (cell);
    }
}

void CabPanel::paint (juce::Graphics& g)
{
    zc::drawModule (g, getLocalBounds().toFloat().reduced (1));
    zc::drawDotMatrixText (g, "CABINET", { 16.0f, 8.0f, 220.0f, 34.0f }, 32.0f, zc::ink, 2.3f, juce::Justification::centredLeft, false);
    g.setColour (zc::sub); g.setFont (zc::mono (9.0f));
    g.drawText ("CAB", getWidth() - 60, 18, 44, 14, juce::Justification::centredRight);
    auto spk = juce::Rectangle<float> (16.0f, 136.0f, (float) getWidth() - 32, (float) getHeight() - 196);
    const float d = std::min (spk.getHeight(), (spk.getWidth() - 12) / 2);
    for (int i = 0; i < 2; ++i)
    {
        juce::Rectangle<float> c (spk.getCentreX() + (i == 0 ? -d - 6 : 6), spk.getCentreY() - d / 2, d, d);
        g.setColour (juce::Colour (0xff0c0c0e)); g.fillEllipse (c.expanded (5));
        juce::Path clip; clip.addEllipse (c);
        g.saveState(); g.reduceClipRegion (clip); zc::drawDotGrid (g, c, 6.0f, 1.1f, juce::Colour (0xff34343a)); g.restoreState();
        g.setColour (zc::hair); g.drawEllipse (c, 2.0f);
        g.setColour (juce::Colours::black); g.fillEllipse (c.withSizeKeepingCentre (d * 0.28f, d * 0.28f));
    }
    g.setColour (zc::sub); g.setFont (zc::mono (10.0f));
    g.drawFittedText (irText, 16, getHeight() - 48, getWidth() - 32, 34, juce::Justification::centredLeft, 2);
}

//==============================================================================
RigContent::RigContent (ZojoRigProcessor& p) : proc (p), amp (p), cab (p), playerPanel (p), tuner (p)
{
    auto& s = proc.apvts;

    currentPresetName = proc.apvts.state.getProperty ("presetName").toString();
    rebuildPresetMenu();
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        const auto& fp = zojo::factoryPresets();
        if (id <= (int) fp.size()) { zojo::applyPreset (proc.apvts, fp[(size_t) id - 1]); currentPresetName = fp[(size_t) id - 1].name; }
        else { currentPresetName = presetBox.getText(); zojo::loadUserPreset (proc, currentPresetName); }
        proc.apvts.state.setProperty ("presetName", currentPresetName, nullptr);
        if (auto f = zojo::greetingFileFor (currentPresetName); f.existsAsFile()) proc.playGreeting (f);
        if (zojo::currentTheme (proc.apvts) == "neon") { proc.toastText = "KONNICHIWA!"; ++proc.toastSerial; }
    };
    addAndMakeVisible (presetBox);
    savePreset.onClick = [this] { savePresetDialog(); };
    deletePreset.onClick = [this]
    {
        if (presetBox.getSelectedId() >= 1000) { zojo::deleteUserPreset (presetBox.getText()); rebuildPresetMenu(); }
    };
    tunerButton.onClick = [this] { tuner.show (true); tunerButton.setToggleState (true, juce::dontSendNotification); };
    tuner.onClose = [this] { tuner.show (false); tunerButton.setToggleState (false, juce::dontSendNotification); };
    audioButton.onClick = [] { openAudioSettings(); };
    for (auto* b : { &savePreset, &deletePreset, &tunerButton }) addAndMakeVisible (b);
    if (isStandalone()) addAndMakeVisible (audioButton);

    inputKnob = std::make_unique<ZKnob> (s, "input_gain", "Input", zc::orange, juce::Colours::white);
    volumeKnob = std::make_unique<ZKnob> (s, "out_volume", "Volume", zc::white);
    addAndMakeVisible (*inputKnob); addAndMakeVisible (*volumeKnob);
    inputChannel.addItemList (zojo::inputNames(), 1);
    addAndMakeVisible (inputChannel);
    inputChannelAttachment = std::make_unique<APVTS::ComboBoxAttachment> (s, "input_ch", inputChannel);
    for (int i = 0; i < (int) zojo::tunings().size(); ++i)
    {
        auto* b = tuningButtons.add (new juce::TextButton (zojo::tunings()[(size_t) i].shortName));
        b->getProperties().set ("accent", (juce::int64) zc::violet.getARGB());
        b->getProperties().set ("onText", (juce::int64) zc::white.getARGB());
        b->setTooltip (zojo::tunings()[(size_t) i].longName);
        b->onClick = [this, i] { setChoice (proc.apvts, "tuning", i); };
        addAndMakeVisible (b);
    }
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);

    using F = ZKnob::Format;
    const auto ink = juce::Colour (0xff0b0b0c);
    pre.push_back (std::make_unique<Pedal> (s, 1, "NGT", "Noise Gate", zc::cyan, "gate_on", std::vector<Pedal::KnobSpec> { { "gate_thresh", "Thresh" }, { "gate_release", "Release" } }, ink));
    pre.push_back (std::make_unique<Pedal> (s, 2, "CMP", "Compressor", zc::blue, "comp_on", std::vector<Pedal::KnobSpec> { { "comp_sustain", "Sustain" }, { "comp_level", "Level" } }));
    pre.push_back (std::make_unique<Pedal> (s, 3, "OD", "Overdrive", zc::green, "od_on", std::vector<Pedal::KnobSpec> { { "od_drive", "Drive" }, { "od_tone", "Tone" }, { "od_level", "Level" } }));
    pre.push_back (std::make_unique<Pedal> (s, 4, "FZ", "Fuzz", zc::orange, "fuzz_on", std::vector<Pedal::KnobSpec> { { "fuzz_fuzz", "Fuzz" }, { "fuzz_tone", "Tone" }, { "fuzz_level", "Level" } }));
    post.push_back (std::make_unique<Pedal> (s, 5, "EQ5", "Graphic EQ", zc::violet, "eq_on", std::vector<Pedal::KnobSpec> {
        { "eq_b1", "100", F::decibels }, { "eq_b2", "300", F::decibels }, { "eq_b3", "900", F::decibels }, { "eq_b4", "2.5k", F::decibels }, { "eq_b5", "6k", F::decibels } }));
    post.push_back (std::make_unique<Pedal> (s, 6, "DBL", "Doubler", zc::white, "dbl_on", std::vector<Pedal::KnobSpec> { { "dbl_width", "Width" }, { "dbl_time", "Time" } }, ink));
    post.push_back (std::make_unique<Pedal> (s, 7, "CHO", "Chorus", zc::blue, "cho_on", std::vector<Pedal::KnobSpec> { { "cho_rate", "Rate" }, { "cho_depth", "Depth" }, { "cho_mix", "Mix" } }));
    post.push_back (std::make_unique<Pedal> (s, 8, "PHS", "Phaser", zc::yellow, "pha_on", std::vector<Pedal::KnobSpec> { { "pha_rate", "Rate" }, { "pha_depth", "Depth" }, { "pha_mix", "Mix" } }, ink));
    post.push_back (std::make_unique<Pedal> (s, 9, "DLY", "Delay", zc::green, "dly_on", std::vector<Pedal::KnobSpec> { { "dly_time", "Time" }, { "dly_feedback", "Repeats" }, { "dly_mix", "Mix" } }));
    post.push_back (std::make_unique<Pedal> (s, 10, "VRB", "Reverb", zc::orange, "rev_on", std::vector<Pedal::KnobSpec> { { "rev_size", "Size" }, { "rev_mix", "Mix" } }));
    for (auto& ped : pre) addAndMakeVisible (*ped);
    for (auto& ped : post) addAndMakeVisible (*ped);
    addAndMakeVisible (amp); addAndMakeVisible (cab); addAndMakeVisible (playerPanel);
    addChildComponent (tuner);

    setSize (baseW, baseH);
    updateTheme();
    startTimerHz (30);
}

RigContent::~RigContent() { proc.tunerMute = false; }

bool RigContent::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (juce::File (f).hasFileExtension ("mp3;wav;flac;ogg;aif;aiff;m4a;wma")) return true;
    return false;
}

void RigContent::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("mp3;wav;flac;ogg;aif;aiff;m4a;wma")) { playerPanel.loadFile (juce::File (f)); return; }
}

void RigContent::rebuildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& fp = zojo::factoryPresets();
    juce::String bank;
    for (int i = 0; i < (int) fp.size(); ++i)
    {
        if (fp[(size_t) i].bank != bank) { bank = fp[(size_t) i].bank; presetBox.addSectionHeading (bank); }
        presetBox.addItem (fp[(size_t) i].name, i + 1);
    }
    const auto user = zojo::userPresetNames();
    if (! user.isEmpty())
    {
        presetBox.addSectionHeading ("Your presets");
        for (int i = 0; i < user.size(); ++i) presetBox.addItem (user[i], 1000 + i);
    }
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemText (i) == currentPresetName) presetBox.setSelectedItemIndex (i, juce::dontSendNotification);
}

void RigContent::savePresetDialog()
{
    saveWindow = std::make_unique<juce::AlertWindow> ("Save preset", "Name this sound:", juce::MessageBoxIconType::NoIcon);
    saveWindow->addTextEditor ("name", currentPresetName, "Name");
    saveWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create ([this] (int result)
    {
        if (result == 1 && saveWindow != nullptr)
        {
            const auto name = saveWindow->getTextEditorContents ("name").trim();
            if (name.isNotEmpty() && zojo::saveUserPreset (proc, name)) { currentPresetName = juce::File::createLegalFileName (name); proc.apvts.state.setProperty ("presetName", currentPresetName, nullptr); rebuildPresetMenu(); }
        }
        saveWindow.reset();
    }), false);
}

void RigContent::updateTheme()
{
    const bool b = zojo::currentTheme (proc.apvts) == "fernburn";
    if (b == burn) return;
    burn = b;
    if (burn)
    {
        bgFire.configure (baseW / 12 + 1, baseH / 12 + 1, 1.2f, 0.16f, 2.4f, 4.4f);       // flames rising from the floor
        headerFire.configure (baseW / 8, 10, 0.9f, 0.42f, 2.0f, 4.6f);               // flames licking behind the logo
        bgFire.warm (90); headerFire.warm (30);
    }
    repaint();
}

void RigContent::repaintFireRegions()
{
    // repaint only background that isn't covered by panels, so the fire costs little CPU
    juce::RectangleList<int> area (getLocalBounds());
    for (auto* c : getChildren())
        if (c->isVisible() && c != &tuner) area.subtract (c->getBounds());
    area.subtract (ioArea.reduced (8));
    area.add ({ 0, 0, baseW, 84 });  // header: chrome logo + glint
    for (auto& r : area) repaint (r);
}

void RigContent::timerCallback()
{
    updateTheme();
    if (burn)
    {
        bgFire.step(); headerFire.step();
        frame = (frame + 1) % 150;                          // glint sweeps the logo every 5 s
        glint = frame < 40 ? (float) frame / 40.0f : -1.0f;
        if (! tuner.isVisible()) repaintFireRegions();
    }

    inMeter.setLevel (proc.inPeak.exchange (0.0f));
    outMeter.setLevel (proc.outPeak.exchange (0.0f));
    amp.refresh(); cab.refresh(); playerPanel.refresh();
    for (auto& ped : pre) ped->refresh();
    for (auto& ped : post) ped->refresh();
    const int t = getChoice (proc.apvts, "tuning");
    if (t != lastTuning)
    {
        lastTuning = t;
        for (int i = 0; i < tuningButtons.size(); ++i) tuningButtons[i]->setToggleState (i == t, juce::dontSendNotification);
        repaint (tuningArea);
    }
    repaint (inMeterArea.expanded (0, 14)); repaint (outMeterArea.expanded (0, 14));
}

void RigContent::resized()
{
    const int m = 20, W = baseW - 2 * m;
    // top bar
    auto top = juce::Rectangle<int> (m, 18, W, 48);
    auto right = top.removeFromRight (720);
    tunerButton.setBounds (right.removeFromRight (80).withSizeKeepingCentre (80, 34));
    right.removeFromRight (8);
    if (audioButton.isVisible()) { audioButton.setBounds (right.removeFromRight (140).withSizeKeepingCentre (140, 34)); right.removeFromRight (8); }
    deletePreset.setBounds (right.removeFromRight (76).withSizeKeepingCentre (76, 34)); right.removeFromRight (6);
    savePreset.setBounds (right.removeFromRight (66).withSizeKeepingCentre (66, 34)); right.removeFromRight (6);
    presetBox.setBounds (right.removeFromRight (240).withSizeKeepingCentre (240, 34));

    // I/O row
    ioArea = { m, 106, W, 100 };
    auto io = ioArea.reduced (16, 8);
    inputKnob->setBounds (io.removeFromLeft (70).withHeight (86));
    io.removeFromLeft (12);
    auto chArea = io.removeFromLeft (120);
    inputChannel.setBounds (chArea.getX(), chArea.getY() + 36, 120, 30);
    io.removeFromLeft (18);
    tuningArea = io.removeFromLeft (380);
    for (int i = 0; i < tuningButtons.size(); ++i)
        tuningButtons[i]->setBounds (tuningArea.getX() + i * 42, tuningArea.getY() + 36, 38, 30);
    volumeKnob->setBounds (io.removeFromRight (70).withHeight (86));
    io.removeFromRight (16); io.removeFromLeft (18);
    const int mw = (io.getWidth() - 20) / 2;
    inMeterArea = { io.getX(), io.getY() + 38, mw, 18 };
    outMeterArea = { io.getX() + mw + 20, io.getY() + 38, mw, 18 };
    inMeter.setBounds (inMeterArea); outMeter.setBounds (outMeterArea);

    // pre pedals
    const int pedalH = 200;
    { const int pw = (W - 30) / 4; for (int i = 0; i < 4; ++i) pre[(size_t) i]->setBounds (m + i * (pw + 10), 248, pw, pedalH); }
    // amp + cab
    amp.setBounds (m, 482, W - 320, 272);
    cab.setBounds (m + W - 310, 482, 310, 272);
    // post pedals
    {
        const int eqW = 340, pw = (W - eqW - 50) / 5;
        post[0]->setBounds (m, 788, eqW, pedalH);
        for (int i = 1; i < 6; ++i) post[(size_t) i]->setBounds (m + eqW + 10 + (i - 1) * (pw + 10), 788, pw, pedalH);
    }
    playerPanel.setBounds (m, 1026, W, 196);
    tuner.setBounds (getLocalBounds());
}

void RigContent::sectionLabel (juce::Graphics& g, int y, const juce::String& num, const juce::String& title, const juce::String& sub)
{
    const int x = 20;
    g.setFont (zc::mono (10.0f, true));
    const int nw = (int) zc::textWidth (zc::mono (10.0f, true), num) + 12;
    if (burn) g.setGradientFill (zc::chromeGradient ({ (float) x, (float) y - 2, (float) nw, 20 }));
    else g.setColour (zc::ink);
    g.fillRoundedRectangle ((float) x, (float) y, (float) nw, 16, 4);
    g.setColour (zc::paper); g.drawText (num, x, y, nw, 16, juce::Justification::centred);
    g.setColour (zc::ink); g.drawText (title.toUpperCase(), x + nw + 10, y, 200, 16, juce::Justification::centredLeft);
    const int tw = (int) zc::textWidth (zc::mono (10.0f, true), title.toUpperCase());
    g.setColour (zc::sub); g.setFont (zc::mono (10.0f));
    g.drawText (sub.toUpperCase(), x + nw + 20 + tw, y, 600, 16, juce::Justification::centredLeft);
    const int lx = x + nw + 30 + tw + (int) zc::textWidth (zc::mono (10.0f), sub.toUpperCase());
    g.setColour (zc::hair);
    for (int xx = lx; xx < baseW - 20; xx += 6) g.drawHorizontalLine (y + 8, (float) xx, (float) xx + 3);
}

void RigContent::paint (juce::Graphics& g)
{
    g.fillAll (zc::paper);
    g.setGradientFill (juce::ColourGradient (zc::red.withAlpha (0.07f), (float) baseW * 0.5f, -80, juce::Colours::transparentBlack, (float) baseW * 0.5f, 420, true));
    g.fillRect (getLocalBounds());
    zc::drawDotGrid (g, getLocalBounds().toFloat(), 16.0f, 1.0f, zc::grid);

    if (burn)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff4a10).withAlpha (0.16f), 0, (float) baseH,
                                                 juce::Colours::transparentBlack, 0, (float) baseH * 0.45f, false));
        g.fillRect (getLocalBounds());
        bgFire.draw (g, { 0.0f, (float) baseH - (float) (baseH / 12 + 1) * 12.0f }, 12.0f);
        headerFire.draw (g, { 0.0f, 4.0f }, 8.0f);

        // chrome logo + edition tag
        const auto logoFont = zc::mono (48.0f, true).withExtraKerningFactor (-0.04f);
        zc::drawChromeText (g, "ZOJO RIG", { 20.0f, 12.0f, 270.0f, 46.0f }, logoFont, -0.18f, glint, true);
        zc::drawChromeText (g, "FERN BURN", { 312.0f, 24.0f, 220.0f, 30.0f }, zc::mono (30.0f, true), -0.22f,
                            glint < 0 ? -1.0f : glint * 1.4f - 0.4f, true);
        g.setColour (zc::orange); g.setFont (zc::mono (9.0f, true));
        g.drawText ("AMP SIMULATOR  -  FERN BURN EDITION", 22, 60, 400, 12, juce::Justification::centredLeft);
    }
    else if (zojo::currentTheme (proc.apvts) == "neon")
    {
        g.setGradientFill (juce::ColourGradient (zc::cyan.withAlpha (0.10f), (float) baseW, (float) baseH,
                                                 juce::Colours::transparentBlack, (float) baseW * 0.4f, (float) baseH * 0.5f, true));
        g.fillRect (getLocalBounds());
        zc::drawDotMatrixText (g, "ZOJO RIG", { 20.0f, 10.0f, 320.0f, 50.0f }, 48.0f, zc::red, 2.9f, juce::Justification::centredLeft, true);
        g.setGradientFill (juce::ColourGradient (zc::red.withAlpha (0.16f), 160.0f, -60.0f, juce::Colours::transparentBlack, 160.0f, 260.0f, true));
        g.fillRect (getLocalBounds());
        zc::drawDotMatrixText (g, "NEON RUSH", { 196.0f, 10.0f, 420.0f, 50.0f }, 40.0f, zc::cyan, 2.6f, juce::Justification::centredLeft, true);
        g.setColour (zc::violet); g.setFont (zc::mono (9.0f, true));
        g.drawText ("AMP SIMULATOR  -  NEON RUSH EDITION", 22, 60, 400, 12, juce::Justification::centredLeft);
    }
    else
    {
    // logo
    zc::drawDotMatrixText (g, "ZOJO RIG", { 20.0f, 10.0f, 320.0f, 50.0f }, 48.0f, zc::ink, 2.9f, juce::Justification::centredLeft, false);
    const float lw = zc::textWidth (zc::mono (48.0f, true).withExtraKerningFactor (-0.04f), "ZOJO RIG");
    juce::Path rd; rd.addEllipse (20.0f + lw + 4, 16, 9, 9);
    juce::DropShadow (zc::red, 10, {}).drawForPath (g, rd); g.setColour (zc::red); g.fillPath (rd);
    g.setColour (zc::sub); g.setFont (zc::mono (9.0f, true));
    g.drawText ("AMP SIMULATOR  -  ZR-1", 22, 60, 300, 12, juce::Justification::centredLeft);
    }

    sectionLabel (g, 84, "01", "I/O", "input - virtual tuning - levels");
    sectionLabel (g, 226, "02", "Pre", "stompboxes before the amp");
    sectionLabel (g, 460, "03", "Amp + Cab", "preamp - tone stack - power amp - speaker");
    sectionLabel (g, 766, "04", "Post", "effects after the amp");
    sectionLabel (g, 1004, "05", "Player", "play along - your music app or a song file");

    zc::drawModule (g, ioArea.toFloat());
    g.setFont (zc::mono (9.0f, true));
    auto lbl = [&] (juce::Rectangle<int> a, const juce::String& t, juce::Colour c, juce::Justification j)
    { g.setColour (c); g.drawText (t, a.getX(), ioArea.getY() + 16, a.getWidth(), 14, j); };
    lbl ({ inputChannel.getX(), 0, 120, 0 }, "CHANNEL", zc::sub, juce::Justification::centredLeft);
    lbl (tuningArea, "VIRTUAL TUNING", zc::sub, juce::Justification::centredLeft);
    const int t = juce::jlimit (0, (int) zojo::tunings().size() - 1, getChoice (proc.apvts, "tuning"));
    const auto& tn = zojo::tunings()[(size_t) t];
    juce::String tname = juce::String (tn.longName).toUpperCase();
    if (tn.semis != 0) tname << "  " << (tn.semis > 0 ? "+" : "-") << std::abs (tn.semis) << " ST";
    lbl (tuningArea.withTrimmedRight (2), tname, zc::violet, juce::Justification::centredRight);
    lbl (inMeterArea, "INPUT - PEAKS NEAR THE RED", zc::sub, juce::Justification::centredLeft);
    lbl (outMeterArea, "OUTPUT", zc::sub, juce::Justification::centredLeft);
    if (inMeter.clipped()) lbl (inMeterArea, "CLIP", zc::red, juce::Justification::centredRight);
    if (outMeter.clipped()) lbl (outMeterArea, "CLIP", zc::red, juce::Justification::centredRight);
}

//==============================================================================
static juce::String paletteFor (const juce::String& theme) { return theme == "neon" ? "neon" : "default"; }

ZojoRigEditor::ZojoRigEditor (ZojoRigProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    juce::LookAndFeel::setDefaultLookAndFeel (&lnf);
    setLookAndFeel (&lnf);
    addChildComponent (toast);
    lastToast = proc.toastSerial.load();
    buildContent();
    startTimerHz (15);

    unmuteStandaloneInput();  // it's a guitar rig: hear the input

    float scale = 1.0f;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = std::min (1.0f, std::min ((float) (d->userArea.getHeight() - 90) / (float) RigContent::baseH,
                                          (float) (d->userArea.getWidth() - 40) / (float) RigContent::baseW));
    scale = std::max (0.55f, scale);
    setResizable (true, true);
    setResizeLimits ((int) (RigContent::baseW * 0.45f), (int) (RigContent::baseH * 0.45f), RigContent::baseW * 2, RigContent::baseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) RigContent::baseW / RigContent::baseH);
    setSize ((int) (RigContent::baseW * scale), (int) (RigContent::baseH * scale));
}

void ZojoRigEditor::buildContent()
{
    palette = paletteFor (zojo::currentTheme (proc.apvts));
    content.reset();
    zc::setPalette (palette);
    lnf.applyPalette();
    content = std::make_unique<RigContent> (proc);
    addAndMakeVisible (*content);
    toast.toFront (false);
    resized();
    repaint();
}

// A preset that changes the palette rebuilds the UI so every colour picks it up
void ZojoRigEditor::timerCallback()
{
    if (paletteFor (zojo::currentTheme (proc.apvts)) != palette) buildContent();
    if (const int s = proc.toastSerial.load(); s != lastToast) { lastToast = s; toast.pop (proc.toastText); toast.toFront (false); }
}

ZojoRigEditor::~ZojoRigEditor()
{
    stopTimer();
    content.reset();
    setLookAndFeel (nullptr);
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
}

void ZojoRigEditor::paint (juce::Graphics& g) { g.fillAll (zc::paper); }

void ZojoRigEditor::resized()
{
    const float s = (float) getWidth() / (float) RigContent::baseW;
    if (content == nullptr) return;
    content->setBounds (0, 0, RigContent::baseW, RigContent::baseH);
    content->setTransform (juce::AffineTransform::scale (s));
    toast.setBounds (420, 52, 460, 150);
    toast.setTransform (juce::AffineTransform::scale (s));
}
