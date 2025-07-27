//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_LR2TIMINGWINDOWS_H
#define RHYTHMGAME_LR2TIMINGWINDOWS_H

#include "BmsRanks.h"
#include "TimingWindows.h"
namespace gameplay_logic::rules::lr2_timing_windows {
auto
judgeEasy() -> TimingWindows;
auto
judgeNormal() -> TimingWindows;
auto
judgeHard() -> TimingWindows;
auto
judgeVeryHard() -> TimingWindows;

auto
getTimingWindows(BmsRank type) -> TimingWindows;
} // namespace gameplay_logic::rules::lr2_timing_windows

#endif // RHYTHMGAME_LR2TIMINGWINDOWS_H
