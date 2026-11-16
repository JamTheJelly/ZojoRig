// Windows implementation: Global System Media Transport Controls (C++/WinRT).
// All WinRT calls run on one background thread (multi-threaded apartment), so blocking
// .get() calls are safe and the UI/audio threads never wait on them.
#ifndef NOMINMAX
 #define NOMINMAX
#endif
#include "NowPlaying.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace wmc = winrt::Windows::Media::Control;
namespace wss = winrt::Windows::Storage::Streams;

struct NowPlaying::Impl
{
    enum class Cmd { toggle, next, previous, seek };
    struct Command { Cmd cmd; double seconds; };

    mutable std::mutex mutex;
    std::condition_variable wake;
    std::deque<Command> commands;
    NowPlayingInfo info;
    std::atomic<bool> quit { false };
    std::thread worker;
    std::string lastArtKey;

    Impl()
    {
        info.supported = true;
        worker = std::thread ([this] { run(); });
    }
    ~Impl()
    {
        quit = true;
        wake.notify_all();
        if (worker.joinable()) worker.join();
    }

    void push (Command c)
    {
        { std::lock_guard<std::mutex> l (mutex); commands.push_back (c); }
        wake.notify_all();
    }

    void run()
    {
        try { winrt::init_apartment (winrt::apartment_type::multi_threaded); } catch (...) {}
        wmc::GlobalSystemMediaTransportControlsSessionManager manager { nullptr };

        while (! quit)
        {
            try
            {
                if (manager == nullptr)
                    manager = wmc::GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();

                auto session = manager.GetCurrentSession();
                std::deque<Command> todo;
                { std::lock_guard<std::mutex> l (mutex); todo.swap (commands); }

                NowPlayingInfo next;
                next.supported = true;
                if (session != nullptr)
                {
                    for (auto& c : todo)
                    {
                        try
                        {
                            switch (c.cmd)
                            {
                                case Cmd::toggle:   session.TryTogglePlayPauseAsync().get(); break;
                                case Cmd::next:     session.TrySkipNextAsync().get(); break;
                                case Cmd::previous: session.TrySkipPreviousAsync().get(); break;
                                case Cmd::seek:     session.TryChangePlaybackPositionAsync ((int64_t) (c.seconds * 10000000.0)).get(); break;
                            }
                        }
                        catch (...) {}
                    }

                    next.hasSession = true;
                    next.app = winrt::to_string (session.SourceAppUserModelId());
                    auto playback = session.GetPlaybackInfo();
                    next.playing = playback.PlaybackStatus() == wmc::GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;

                    auto timeline = session.GetTimelineProperties();
                    const double start = std::chrono::duration<double> (timeline.StartTime()).count();
                    double pos = std::chrono::duration<double> (timeline.Position()).count() - start;
                    if (next.playing)  // the app only reports position occasionally; extrapolate
                        pos += std::chrono::duration<double> (winrt::clock::now() - timeline.LastUpdatedTime()).count();
                    next.duration = std::chrono::duration<double> (timeline.EndTime()).count() - start;
                    next.position = next.duration > 0 ? std::min (std::max (0.0, pos), next.duration) : std::max (0.0, pos);

                    auto props = session.TryGetMediaPropertiesAsync().get();
                    next.title = winrt::to_string (props.Title());
                    next.artist = winrt::to_string (props.Artist());

                    const std::string artKey = next.app + "|" + next.title + "|" + next.artist;
                    std::vector<uint8_t> art;
                    bool artChanged = false;
                    if (artKey != lastArtKey)
                    {
                        artChanged = true; lastArtKey = artKey;
                        try
                        {
                            if (auto thumb = props.Thumbnail())
                            {
                                auto stream = thumb.OpenReadAsync().get();
                                const auto size = (uint32_t) std::min<uint64_t> (stream.Size(), 8u * 1024u * 1024u);
                                wss::Buffer buffer (size);
                                auto filled = stream.ReadAsync (buffer, size, wss::InputStreamOptions::None).get();
                                art.resize (filled.Length());
                                if (! art.empty()) wss::DataReader::FromBuffer (filled).ReadBytes (art);
                            }
                        }
                        catch (...) { art.clear(); }
                    }

                    std::lock_guard<std::mutex> l (mutex);
                    if (artChanged) { next.artwork = std::move (art); next.artworkVersion = info.artworkVersion + 1; }
                    else            { next.artwork = info.artwork; next.artworkVersion = info.artworkVersion; }
                    info = std::move (next);
                }
                else
                {
                    std::lock_guard<std::mutex> l (mutex);
                    lastArtKey.clear();
                    next.artworkVersion = info.artworkVersion + (info.artwork.empty() ? 0 : 1);
                    info = std::move (next);
                }
            }
            catch (...)
            {
                manager = nullptr;   // try to reconnect next time round
            }

            std::unique_lock<std::mutex> l (mutex);
            wake.wait_for (l, std::chrono::milliseconds (400), [this] { return quit.load() || ! commands.empty(); });
        }
    }
};

NowPlaying::NowPlaying() : impl (std::make_unique<Impl>()) {}
NowPlaying::~NowPlaying() = default;
NowPlayingInfo NowPlaying::get() const { std::lock_guard<std::mutex> l (impl->mutex); return impl->info; }
void NowPlaying::togglePlayPause() { impl->push ({ Impl::Cmd::toggle, 0 }); }
void NowPlaying::next()            { impl->push ({ Impl::Cmd::next, 0 }); }
void NowPlaying::previous()        { impl->push ({ Impl::Cmd::previous, 0 }); }
void NowPlaying::seek (double s)   { impl->push ({ Impl::Cmd::seek, s }); }
