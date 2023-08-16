//
// Created by bobini on 23.06.23.
//

#ifndef RHYTHMGAME_BMSPOINTS_H
#define RHYTHMGAME_BMSPOINTS_H
#include <boost/serialization/strong_typedef.hpp>
#include <chrono>
#include "Judgement.h"
namespace gameplay_logic {
struct BmsPoints
{
    double value;
    static constexpr auto maxValue = 1.0;
    Judgement judgement;
    std::chrono::nanoseconds deviation;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSPOINTS_H
