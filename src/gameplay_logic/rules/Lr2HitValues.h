//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_LR2HITVALUES_H
#define RHYTHMGAME_LR2HITVALUES_H

#include "gameplay_logic/Judgement.h"
namespace gameplay_logic::rules::lr2_hit_values {
auto
getLr2HitValue(std::chrono::nanoseconds offset, gameplay_logic::Judgement judgement) -> double;
} // namespace gameplay_logic::rules::lr2_hit_values

#endif // RHYTHMGAME_LR2HITVALUES_H
