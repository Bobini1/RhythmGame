#include "FrameRateLimiter.h"

#include <algorithm>
#include <cstdint>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace support {

FrameRateLimiter::~FrameRateLimiter()
{
#ifdef _WIN32
    if (highResolutionTimer != nullptr) {
        CloseHandle(static_cast<HANDLE>(highResolutionTimer));
    }
#endif
}

void
FrameRateLimiter::wait(const int frameRateLimit)
{
    pace(
      frameRateLimit,
      [] { return Clock::now(); },
      [this](const TimePoint deadline) { waitUntil(deadline); });
}

void
FrameRateLimiter::waitUntil(const TimePoint deadline)
{
#ifdef _WIN32
    auto timer = static_cast<HANDLE>(highResolutionTimer);
    if (timer == nullptr) {
        timer = CreateWaitableTimerExW(nullptr,
                                       nullptr,
                                       CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                       TIMER_MODIFY_STATE | SYNCHRONIZE);
        highResolutionTimer = timer;
    }

    const auto remaining = deadline - Clock::now();
    const auto remainingNanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(remaining).count();
    if (remainingNanoseconds <= 0) {
        return;
    }

    if (timer != nullptr) {
        LARGE_INTEGER dueTime;
        dueTime.QuadPart =
          -std::max<std::int64_t>(1, (remainingNanoseconds + 99) / 100);
        if (SetWaitableTimerEx(
              timer, &dueTime, 0, nullptr, nullptr, nullptr, 0) != FALSE &&
            WaitForSingleObject(timer, INFINITE) == WAIT_OBJECT_0) {
            return;
        }
    }
#else
    std::this_thread::sleep_until(deadline);
    return;
#endif

#ifdef _WIN32
    // High-resolution waitable timers are available on supported Windows 10
    // and newer systems. Preserve limiter accuracy if creating or waiting on
    // one nevertheless fails; this path is exceptional, so yielding is a
    // better fallback than silently reverting to a 15.6 ms sleep quantum.
    while (Clock::now() < deadline) {
        std::this_thread::yield();
    }
#endif
}

} // namespace support
