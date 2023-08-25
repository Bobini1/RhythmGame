//
// Created by bobini on 25.06.23.
//

#include "BmsScore.h"
#include <magic_enum.hpp>

namespace gameplay_logic {
auto
BmsScore::addTap(Tap tap) -> void
{
    if (auto points = tap.getPointsOptional()) {
        hitsWithPoints.emplace_back(tap);
        this->points += points->getValue();
        judgementCounts[points->getJudgement()]++;
        emit pointsChanged();
        emit judgementCountsChanged();
        emit hitsWithPointsChanged();
    } else {
        hitsWithoutPoints.push_back(tap);
        emit hitsWithoutPointsChanged();
    }
}
auto
BmsScore::addMiss(Miss miss) -> void
{
    misses.push_back(miss);
    judgementCounts[Judgement::MISS]++;
    emit missesChanged();
    emit judgementCountsChanged();
}
BmsScore::BmsScore(int maxHits, QObject* parent)
  : QObject(parent)
  , maxPoints(maxHits * BmsPoints::maxValue)
  , maxHits(maxHits)
{
}
auto
BmsScore::getMaxPoints() const -> double
{
    return maxPoints;
}
auto
BmsScore::getMaxHits() const -> int
{
    return maxHits;
}
auto
BmsScore::getPoints() const -> double
{
    return points;
}
auto
BmsScore::getJudgementCounts() const -> QVariantMap
{
    auto map = QVariantMap{};
    for (const auto& [judgement, count] : judgementCounts) {
        auto judgementName = std::string{ magic_enum::enum_name(judgement) };
        map[QString::fromStdString(judgementName)] = count;
    }
    return map;
}

} // namespace gameplay_logic