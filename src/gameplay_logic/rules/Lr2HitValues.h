//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_LR2HITVALUES_H
#define RHYTHMGAME_LR2HITVALUES_H

#include "TimingWindows.h"
namespace gameplay_logic::rules::lr2_hit_values {
auto
getLr2HitValue(const TimingWindows& timingWindows,
               std::chrono::nanoseconds offset) -> double;
} // namespace gameplay_logic::rules::lr2_hit_values

#endif // RHYTHMGAME_LR2HITVALUES_H
