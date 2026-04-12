//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSRANKS_H
#define RHYTHMGAME_BMSRANKS_H

namespace gameplay_logic::rules {
enum class BmsRank
{
    VeryHard = 25,
    Hard = 50,
    Normal = 75,
    Easy = 100
};

static constexpr auto defaultBmsRank = BmsRank::Normal;
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSRANKS_H
