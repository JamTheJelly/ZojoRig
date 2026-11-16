#pragma once
#include "Widgets.h"
#include "../NowPlaying.h"

class ZojoRigProcessor;

// Dot-row seek bar with an A-B loop overlay
class SeekBar : public juce::Component
{
public:
    void setState (double pos, double len, double a, double b) { position = pos; length = len; loopA = a; loopB = b; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent& e) override;
    std::function<void (double seconds)> onSeek;
    juce::Colour accent = zc::red;
private:
    double position = 0, length = 0, loopA = 0, loopB = 0;
};

// "05 PLAYER": Now Playing (system media controls) + file player with speed/key/A-B loop
class PlayerPanel : public juce::Component
{
public:
    explicit PlayerPanel (ZojoRigProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();                      // call ~30x per second
    void loadFile (const juce::File&);
    void chooseFile();

private:
    ZojoRigProcessor& proc;
    NowPlaying nowPlaying;
    NowPlayingInfo np;
    juce::Image artwork; uint32_t artworkVersion = 0;

    juce::TextButton npPrev { "Prev" }, npPlay { "Play" }, npNext { "Next" };
    SeekBar npBar;

    juce::TextButton load { "Load song..." }, play { "Play" }, restart { "Restart" }, setA { "Set A" }, setB { "Set B" },
                     clearAB { "A-B off" }, loopSong { "Loop" }, follow { "Follow tuning" };
    std::unique_ptr<APVTS::ButtonAttachment> loopAttachment, followAttachment;
    std::unique_ptr<ZKnob> speed, key, volume;
    SeekBar fileBar;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> npCard, fileCard, artArea;
    static juce::String fmtTime (double s);
};
