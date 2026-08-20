#include "support/FrameRateLimiter.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>

namespace {

using namespace std::chrono_literals;

TEST_CASE("frame limiter does not accumulate wait overshoot",
          "[support][FrameRateLimiter]")
{
    support::FrameRateLimiter limiter;
    auto now = support::FrameRateLimiter::TimePoint{} + 1s;
    constexpr auto frameRateLimit = 300;
    constexpr auto waitOvershoot = 800us;
    constexpr auto frameCount = 301;

    const auto firstFrameStart = now;
    for (auto frame = 0; frame < frameCount; ++frame) {
        limiter.pace(
          frameRateLimit,
          [&] { return now; },
          [&](const auto deadline) { now = deadline + waitOvershoot; });
    }

    const auto elapsed = now - firstFrameStart;
    CHECK(elapsed >= 1s);
    CHECK(elapsed < 1002ms);
}

TEST_CASE("frame limiter does not catch up after a missed deadline",
          "[support][FrameRateLimiter]")
{
    support::FrameRateLimiter limiter;
    auto now = support::FrameRateLimiter::TimePoint{} + 1s;
    auto waits = 0;
    auto lastDeadline = support::FrameRateLimiter::TimePoint{};
    const auto waitUntil = [&](const auto deadline) {
        ++waits;
        lastDeadline = deadline;
        now = deadline;
    };

    limiter.pace(100, [&] { return now; }, waitUntil);
    now += 35ms;
    limiter.pace(100, [&] { return now; }, waitUntil);
    CHECK(waits == 0);

    limiter.pace(100, [&] { return now; }, waitUntil);
    CHECK(waits == 1);
    CHECK(lastDeadline == support::FrameRateLimiter::TimePoint{} + 1s + 45ms);
}

TEST_CASE("frame limiter resets its deadline when the limit changes",
          "[support][FrameRateLimiter]")
{
    support::FrameRateLimiter limiter;
    auto now = support::FrameRateLimiter::TimePoint{} + 1s;
    auto deadline = support::FrameRateLimiter::TimePoint{};
    const auto waitUntil = [&](const auto value) {
        deadline = value;
        now = value;
    };

    limiter.pace(100, [&] { return now; }, waitUntil);
    limiter.pace(100, [&] { return now; }, waitUntil);
    CHECK(deadline == support::FrameRateLimiter::TimePoint{} + 1s + 10ms);

    deadline = {};
    limiter.pace(200, [&] { return now; }, waitUntil);
    CHECK(deadline == support::FrameRateLimiter::TimePoint{});
    limiter.pace(200, [&] { return now; }, waitUntil);
    CHECK(deadline == support::FrameRateLimiter::TimePoint{} + 1s + 15ms);
}

} // namespace
