//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <vector>
#include <boost/container/flat_map.hpp>
#include "TimePoint.h"
#include "BmsPoints.h"

namespace gameplay_logic {

struct BmsScore
{
    double maxPoints = 0;
    double maxHits = 0;
    std::vector<gameplay_logic::TimePoint> misses;
    std::vector<std::pair<gameplay_logic::TimePoint, BmsPoints>> hitsWithPoints;
    std::vector<gameplay_logic::TimePoint> hitsWithoutPoints;
    double points = 0;
    boost::container::flat_map<Judgement, int> judgementCounts;
    auto addHit(gameplay_logic::TimePoint timePoint,
                std::optional<BmsPoints> points = std::nullopt) -> void;
    auto addMiss(gameplay_logic::TimePoint timePoint) -> void;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
