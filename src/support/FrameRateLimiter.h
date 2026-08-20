#ifndef RHYTHMGAME_FRAMERATELIMITER_H
#define RHYTHMGAME_FRAMERATELIMITER_H

#include <chrono>

namespace support {

class FrameRateLimiter
{
  public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

  private:
    TimePoint nextFrameDeadline;
    int previousFrameRateLimit = 0;
#ifdef _WIN32
    void* highResolutionTimer = nullptr;
#endif

    void waitUntil(TimePoint deadline);

  public:
    FrameRateLimiter() = default;
    ~FrameRateLimiter();
    FrameRateLimiter(const FrameRateLimiter&) = delete;
    auto operator=(const FrameRateLimiter&) -> FrameRateLimiter& = delete;

    void wait(int frameRateLimit);

    template<typename Now, typename WaitUntil>
    void pace(const int frameRateLimit, Now&& now, WaitUntil&& waitUntil)
    {
        if (frameRateLimit <= 0) {
            previousFrameRateLimit = 0;
            nextFrameDeadline = {};
            return;
        }

        const auto frameInterval =
          std::chrono::nanoseconds(std::chrono::seconds(1)) / frameRateLimit;
        auto currentTime = now();
        if (frameRateLimit != previousFrameRateLimit ||
            nextFrameDeadline == TimePoint{}) {
            previousFrameRateLimit = frameRateLimit;
            nextFrameDeadline = currentTime + frameInterval;
            return;
        }

        if (currentTime >= nextFrameDeadline) {
            nextFrameDeadline = currentTime + frameInterval;
            return;
        }

        waitUntil(nextFrameDeadline);
        currentTime = now();
        nextFrameDeadline += frameInterval;
        if (currentTime >= nextFrameDeadline) {
            nextFrameDeadline = currentTime + frameInterval;
        }
    }
};

} // namespace support

#endif // RHYTHMGAME_FRAMERATELIMITER_H
