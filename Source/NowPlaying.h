#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

// Reads and controls whatever media app Windows considers "current" (Spotify, Apple Music,
// browsers playing YouTube Music / Tidal, etc.) through the system media transport controls.
// Plain C++ types only, so the Windows implementation doesn't need JUCE headers.
struct NowPlayingInfo
{
    bool supported = false;     // false on systems without the Windows media API
    bool hasSession = false;    // something is registered with the system media controls
    bool playing = false;
    std::string app, title, artist;
    double position = 0, duration = 0;   // seconds
    std::vector<uint8_t> artwork;        // encoded image bytes (PNG/JPEG), may be empty
    uint32_t artworkVersion = 0;         // changes whenever artwork changes
};

class NowPlaying
{
public:
    NowPlaying();
    ~NowPlaying();
    NowPlayingInfo get() const;
    void togglePlayPause();
    void next();
    void previous();
    void seek (double seconds);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
