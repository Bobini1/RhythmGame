//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSRANKS_H
#define RHYTHMGAME_BMSRANKS_H

namespace gameplay_logic::rules {
enum class BmsRank
{
    Easy,
    Normal,
    Hard,
    VeryHard
};

static constexpr auto defaultBmsRank = BmsRank::Normal;
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSRANKS_H
