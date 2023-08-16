//
// Created by bobini on 25.06.23.
//

#include "BmsScore.h"

namespace gameplay_logic {
auto
BmsScore::addHit(gameplay_logic::TimePoint timePoint,
                 std::optional<BmsPoints> points) -> void
{
    if (points) {
        hitsWithPoints.emplace_back(timePoint, *points);
        this->points += points->value;
        judgementCounts[points->judgement]++;
    } else {
        hitsWithoutPoints.push_back(timePoint);
    }
}
auto
BmsScore::addMiss(gameplay_logic::TimePoint timePoint) -> void
{
    misses.push_back(timePoint);
    judgementCounts[Judgement::MISS]++;
}
} // namespace gameplay_logic