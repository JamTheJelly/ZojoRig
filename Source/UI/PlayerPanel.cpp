#include "PlayerPanel.h"
#include "../PluginProcessor.h"

//==============================================================================
void SeekBar::paint (juce::Graphics& g)
{
    const float s = 7.0f, r = 2.1f;
    const int cols = std::max (2, (int) ((float) getWidth() / s));
    const float cy = (float) getHeight() * 0.5f;
    const double frac = length > 0 ? juce::jlimit (0.0, 1.0, position / length) : 0.0;
    const bool ab = length > 0 && loopB > loopA + 0.2;
    for (int c = 0; c < cols; ++c)
    {
        const double t = (double) c / (cols - 1), sec = t * length;
        auto col = zc::dotOff;
        if (ab && sec >= loopA && sec <= loopB) col = zc::orange.withAlpha (0.45f);
        if (length > 0 && t <= frac) col = (ab && sec >= loopA && sec <= loopB) ? zc::orange : zc::ink;
        g.setColour (col);
        g.fillEllipse (c * s + s * 0.5f - r, cy - r, r * 2, r * 2);
    }
    if (length > 0)
    {
        const float x = (float) (frac * (getWidth() - s)) + s * 0.5f;
        juce::Path head; head.addEllipse (x - 4.5f, cy - 4.5f, 9, 9);
        juce::DropShadow (accent, 10, {}).drawForPath (g, head);
        g.setColour (accent); g.fillPath (head);
    }
}

void SeekBar::mouseDrag (const juce::MouseEvent& e)
{
    if (length <= 0 || ! onSeek) return;
    onSeek (juce::jlimit (0.0, 1.0, (double) e.position.x / getWidth()) * length);
}

//==============================================================================
juce::String PlayerPanel::fmtTime (double s)
{
    if (! std::isfinite (s) || s < 0) s = 0;
    const int t = (int) s;
    return juce::String (t / 60) + ":" + juce::String (t % 60).paddedLeft ('0', 2);
}

static juce::String prettyAppName (const std::string& aumid)
{
    juce::String a (juce::CharPointer_UTF8 (aumid.c_str()));
    a = a.upToFirstOccurrenceOf ("!", false, false).upToFirstOccurrenceOf ("_", false, false);
    if (a.endsWithIgnoreCase (".exe")) a = a.dropLastCharacters (4);
    if (a.containsChar ('.')) a = a.fromLastOccurrenceOf (".", false, false);
    if (a.equalsIgnoreCase ("ZuneMusic")) a = "Media Player";
    if (a.equalsIgnoreCase ("MSEdge")) a = "Edge";
    return a.toUpperCase();
}

PlayerPanel::PlayerPanel (ZojoRigProcessor& p) : proc (p)
{
    auto& s = proc.apvts;
    for (auto* b : { &npPrev, &npPlay, &npNext, &load, &play, &restart, &setA, &setB, &clearAB })
        addAndMakeVisible (b);
    for (auto* b : { &loopSong, &follow })
    {
        b->setClickingTogglesState (true);
        b->getProperties().set ("chip", true);
        b->getProperties().set ("accent", (juce::int64) zc::green.getARGB());
        addAndMakeVisible (b);
    }
    loopAttachment = std::make_unique<APVTS::ButtonAttachment> (s, "ply_loop", loopSong);
    followAttachment = std::make_unique<APVTS::ButtonAttachment> (s, "ply_follow", follow);
    play.getProperties().set ("accent", (juce::int64) zc::red.getARGB());
    play.getProperties().set ("onText", (juce::int64) zc::white.getARGB());

    npPrev.onClick = [this] { nowPlaying.previous(); };
    npNext.onClick = [this] { nowPlaying.next(); };
    npPlay.onClick = [this] { nowPlaying.togglePlayPause(); };
    npBar.onSeek = [this] (double sec) { nowPlaying.seek (sec); };
    npBar.accent = zc::green;
    addAndMakeVisible (npBar);

    load.onClick = [this] { chooseFile(); };
    play.onClick = [this] { proc.player.setPlaying (! proc.player.isPlaying()); refresh(); };
    restart.onClick = [this] { proc.player.seekTo (proc.player.getLoopB() > proc.player.getLoopA() ? proc.player.getLoopA() : 0.0); };
    setA.onClick = [this]
    {
        const double pos = proc.player.getPositionSeconds(), b = proc.player.getLoopB();
        proc.player.setLoopPoints (pos, b > pos + 0.3 ? b : 0.0);
    };
    setB.onClick = [this]
    {
        const double pos = proc.player.getPositionSeconds(), a = proc.player.getLoopA();
        if (pos > a + 0.3) { proc.player.setLoopPoints (a, pos); proc.player.seekTo (a); }
    };
    clearAB.onClick = [this] { proc.player.setLoopPoints (0, 0); };
    fileBar.onSeek = [this] (double sec) { proc.player.seekTo (sec); };
    addAndMakeVisible (fileBar);

    speed = std::make_unique<ZKnob> (s, "ply_speed", "Speed", zc::orange, juce::Colours::white, ZKnob::Format::percent);
    key = std::make_unique<ZKnob> (s, "ply_key", "Key", zc::violet, juce::Colours::white, ZKnob::Format::semitones);
    volume = std::make_unique<ZKnob> (s, "ply_volume", "Volume", zc::white);
    for (auto* k : { speed.get(), key.get(), volume.get() }) addAndMakeVisible (k);
    refresh();
}

void PlayerPanel::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a song to play along with", juce::File(), FilePlayer::supportedWildcard());
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) loadFile (fc.getResult()); });
}

void PlayerPanel::loadFile (const juce::File& f) { proc.player.load (f); repaint(); }

void PlayerPanel::resized()
{
    npCard = { 0, 0, 470, getHeight() };
    fileCard = { 480, 0, getWidth() - 480, getHeight() };

    artArea = { npCard.getX() + 16, 36, 112, 112 };
    const int tx = artArea.getRight() + 16;
    npPrev.setBounds (tx, 92, 64, 30); npPlay.setBounds (tx + 70, 92, 84, 30); npNext.setBounds (tx + 160, 92, 64, 30);
    npBar.setBounds (tx, 136, npCard.getRight() - tx - 16, 14);

    const int fx = fileCard.getX() + 16, knobsW = 236, leftW = fileCard.getWidth() - 32 - knobsW - 10;
    int x = fx; const int y = 40, h = 30;
    auto place = [&] (juce::Button& b, int w) { b.setBounds (x, y, w, h); x += w + 5; };
    place (load, 96); place (play, 58); place (restart, 70); place (setA, 50); place (setB, 50); place (clearAB, 64); place (loopSong, 54);
    fileBar.setBounds (fx, 86, leftW, 16);

    const int kx = fileCard.getRight() - 16 - knobsW;
    speed->setBounds (kx, 28, 76, 92); key->setBounds (kx + 80, 28, 76, 92); volume->setBounds (kx + 160, 28, 76, 92);
    follow.setBounds (kx + 18, 132, knobsW - 36, 26);
}

void PlayerPanel::refresh()
{
    // --- now playing ---
    auto info = nowPlaying.get();
    const bool changed = info.title != np.title || info.artist != np.artist || info.playing != np.playing
                         || info.hasSession != np.hasSession || info.app != np.app;
    if (info.artworkVersion != artworkVersion)
    {
        artworkVersion = info.artworkVersion;
        artwork = info.artwork.empty() ? juce::Image() : juce::ImageFileFormat::loadFrom (info.artwork.data(), info.artwork.size());
        repaint (artArea);
    }
    np = std::move (info);
    npPlay.setButtonText (np.playing ? "Pause" : "Play");
    for (auto* b : { &npPrev, &npPlay, &npNext }) b->setEnabled (np.hasSession);
    npBar.setState (np.position, np.duration, 0, 0);
    if (changed) repaint (npCard);
    repaint (npCard.getX(), 150, npCard.getWidth(), 20);

    // --- file player ---
    auto& pl = proc.player;
    play.setButtonText (pl.isPlaying() ? "Pause" : "Play");
    play.setToggleState (pl.isPlaying(), juce::dontSendNotification);
    const bool has = pl.getLengthSeconds() > 0;
    for (auto* b : { &play, &restart, &setA, &setB, &clearAB }) b->setEnabled (has);
    fileBar.setState (pl.getPositionSeconds(), pl.getLengthSeconds(), pl.getLoopA(), pl.getLoopB());
    repaint (fileCard.getX(), 102, fileCard.getWidth() - 260, fileCard.getHeight() - 102);
}

void PlayerPanel::paint (juce::Graphics& g)
{
    // ---- Now Playing card ----
    zc::drawModule (g, npCard.toFloat().reduced (0.5f));
    g.setFont (zc::mono (9.0f, true)); g.setColour (zc::sub);
    g.drawText ("NOW PLAYING", npCard.getX() + 16, 12, 200, 14, juce::Justification::centredLeft);
    if (np.hasSession)
    {
        g.setColour (zc::green);
        g.drawText ((np.playing ? "* " : "") + prettyAppName (np.app), npCard.getRight() - 216, 12, 200, 14, juce::Justification::centredRight);
    }

    g.setColour (juce::Colour (0xff0c0c0e)); g.fillRoundedRectangle (artArea.toFloat(), 10);
    if (artwork.isValid())
    {
        g.saveState();
        juce::Path clip; clip.addRoundedRectangle (artArea.toFloat(), 10); g.reduceClipRegion (clip);
        g.drawImage (artwork, artArea.toFloat(), juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        g.restoreState();
    }
    else
    {
        g.saveState(); g.reduceClipRegion (artArea);
        zc::drawDotGrid (g, artArea.toFloat(), 8.0f, 1.3f, juce::Colour (0xff2a2a2e));
        g.restoreState();
        g.setColour (zc::hair.brighter (0.5f)); g.fillEllipse (artArea.toFloat().withSizeKeepingCentre (14, 14));
    }

    const int tx = artArea.getRight() + 16, tw = npCard.getRight() - tx - 16;
    if (! np.supported)
    {
        g.setColour (zc::ink); g.setFont (zc::mono (13.0f, true));
        g.drawText ("Windows only", tx, 38, tw, 18, juce::Justification::centredLeft);
        g.setColour (zc::sub); g.setFont (zc::mono (10.0f));
        g.drawFittedText ("Controls the music app playing on your PC.", tx, 58, tw, 28, juce::Justification::topLeft, 2);
    }
    else if (! np.hasSession)
    {
        g.setColour (zc::ink); g.setFont (zc::mono (13.0f, true));
        g.drawText ("Nothing playing", tx, 38, tw, 18, juce::Justification::centredLeft);
        g.setColour (zc::sub); g.setFont (zc::mono (10.0f));
        g.drawFittedText ("Start a song in Spotify, Apple Music, or YouTube Music in your browser. It will show up here.",
                          tx, 58, tw, 30, juce::Justification::topLeft, 2);
    }
    else
    {
        g.setColour (zc::ink); g.setFont (zc::mono (14.0f, true));
        g.drawText (juce::String (juce::CharPointer_UTF8 (np.title.c_str())), tx, 38, tw, 20, juce::Justification::centredLeft, true);
        g.setColour (zc::sub); g.setFont (zc::mono (11.0f));
        g.drawText (juce::String (juce::CharPointer_UTF8 (np.artist.c_str())), tx, 60, tw, 16, juce::Justification::centredLeft, true);
    }
    g.setColour (zc::sub); g.setFont (zc::mono (9.5f));
    if (np.hasSession && np.duration > 0)
    {
        g.drawText (fmtTime (np.position), tx, 152, 60, 14, juce::Justification::centredLeft);
        g.drawText (fmtTime (np.duration), npCard.getRight() - 76, 152, 60, 14, juce::Justification::centredRight);
    }
    g.drawText ("Volume: use the music app", npCard.getX() + 16, getHeight() - 26, 300, 14, juce::Justification::centredLeft);

    // ---- File player card ----
    zc::drawModule (g, fileCard.toFloat().reduced (0.5f));
    g.setFont (zc::mono (9.0f, true)); g.setColour (zc::sub);
    g.drawText ("FILE PLAYER  -  SLOW DOWN, CHANGE KEY, LOOP", fileCard.getX() + 16, 12, 400, 14, juce::Justification::centredLeft);
    auto& pl = proc.player;
    const int fx = fileCard.getX() + 16, leftW = fileBar.getWidth();
    g.setColour (zc::sub); g.setFont (zc::mono (9.5f));
    if (pl.getLengthSeconds() > 0)
    {
        g.drawText (fmtTime (pl.getPositionSeconds()), fx, 104, 60, 14, juce::Justification::centredLeft);
        g.drawText (fmtTime (pl.getLengthSeconds()), fx + leftW - 60, 104, 60, 14, juce::Justification::centredRight);
        if (pl.getLoopB() > pl.getLoopA() + 0.2)
        {
            g.setColour (zc::orange);
            g.drawText ("LOOP " + fmtTime (pl.getLoopA()) + " - " + fmtTime (pl.getLoopB()), fx, 104, leftW, 14, juce::Justification::centred);
        }
    }
    juce::String name, hint;
    if (pl.isLoading())                   { name = "Loading..."; hint = pl.getFile().getFileName(); }
    else if (pl.getError().isNotEmpty())  { name = pl.getError(); hint = pl.getFile().getFileName(); }
    else if (pl.getLengthSeconds() > 0)   { name = pl.getFile().getFileNameWithoutExtension(); hint = "Set A and B while it plays to loop a section. Speed keeps the pitch; Key keeps the tempo."; }
    else                                  { name = "No song loaded"; hint = "Click LOAD SONG or drag an MP3 / WAV / FLAC file anywhere onto ZOJO RIG."; }
    g.setColour (pl.getError().isNotEmpty() ? zc::red : zc::ink); g.setFont (zc::mono (14.0f, true));
    g.drawText (name, fx, 128, leftW, 20, juce::Justification::centredLeft, true);
    g.setColour (zc::sub); g.setFont (zc::mono (10.0f));
    g.drawFittedText (hint, fx, 150, leftW, 30, juce::Justification::topLeft, 2);
}
