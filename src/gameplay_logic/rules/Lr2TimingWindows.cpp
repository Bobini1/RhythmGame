//
// Created by bobini on 10.09.23.
//

#include "Lr2TimingWindows.h"
namespace gameplay_logic::rules::lr2_timing_windows {
using namespace std::chrono_literals;

auto
judgeEasy() -> TimingWindows
{
    using Nanos = boost::icl::interval<std::chrono::nanoseconds>;
    static auto map = [] {
        auto map = TimingWindows{};
        map.set({ Nanos::open(-1000ms, 0ms), Judgement::EmptyPoor });
        map.set({ Nanos::open(-200ms, +200ms), Judgement::Bad });
        map.set({ Nanos::open(-120ms, +120ms), Judgement::Good });
        map.set({ Nanos::open(-60ms, +60ms), Judgement::Great });
        map.set({ Nanos::open(-21ms, +21ms), Judgement::Perfect });
        return map;
    }();
    return map;
}

auto
judgeNormal() -> TimingWindows
{
    using Nanos = boost::icl::interval<std::chrono::nanoseconds>;
    static auto map = [] {
        auto map = TimingWindows{};
        map.set({ Nanos::open(-1000ms, 0ms), Judgement::EmptyPoor });
        map.set({ Nanos::open(-200ms, +200ms), Judgement::Bad });
        map.set({ Nanos::open(-100ms, +100ms), Judgement::Good });
        map.set({ Nanos::open(-40ms, +40ms), Judgement::Great });
        map.set({ Nanos::open(-18ms, +18ms), Judgement::Perfect });
        return map;
    }();
    return map;
}

auto
judgeHard() -> TimingWindows
{
    using Nanos = boost::icl::interval<std::chrono::nanoseconds>;
    static auto map = [] {
        auto map = TimingWindows{};
        map.set({ Nanos::open(-1000ms, 0ms), Judgement::EmptyPoor });
        map.set({ Nanos::open(-200ms, +200ms), Judgement::Bad });
        map.set({ Nanos::open(-60ms, +60ms), Judgement::Good });
        map.set({ Nanos::open(-30ms, +30ms), Judgement::Great });
        map.set({ Nanos::open(-15ms, +15ms), Judgement::Perfect });
        return map;
    }();
    return map;
}

auto
judgeVeryHard() -> TimingWindows
{
    using Nanos = boost::icl::interval<std::chrono::nanoseconds>;
    static auto map = [] {
        auto map = TimingWindows{};
        map.set({ Nanos::open(-1000ms, 0ms), Judgement::EmptyPoor });
        map.set({ Nanos::open(-200ms, +200ms), Judgement::Bad });
        map.set({ Nanos::open(-40ms, +40ms), Judgement::Good });
        map.set({ Nanos::open(-24ms, +24ms), Judgement::Great });
        map.set({ Nanos::open(-8ms, +8ms), Judgement::Perfect });
        return map;
    }();
    return map;
}
auto
getTimingWindows(BmsRank type) -> TimingWindows
{
    switch (type) {
        case BmsRank::Easy:
            return judgeEasy();
        case BmsRank::Normal:
            return judgeNormal();
        case BmsRank::Hard:
            return judgeHard();
        case BmsRank::VeryHard:
            return judgeVeryHard();
    }
    throw std::invalid_argument("Invalid rank");
}

} // namespace gameplay_logic::rules::lr2_timing_windows
