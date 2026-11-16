// Non-Windows builds: the system media controls API isn't available.
#include "NowPlaying.h"

struct NowPlaying::Impl {};
NowPlaying::NowPlaying() : impl (std::make_unique<Impl>()) {}
NowPlaying::~NowPlaying() = default;
NowPlayingInfo NowPlaying::get() const { return {}; }
void NowPlaying::togglePlayPause() {}
void NowPlaying::next() {}
void NowPlaying::previous() {}
void NowPlaying::seek (double) {}
