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
        judgementCounts[points->getJudgementEnum()]++;
        for (auto* gauge : gauges) {
            gauge->addHit(std::chrono::nanoseconds(tap.getOffsetFromStart()),
                          std::chrono::nanoseconds(points->getDeviation()));
        }
        emit pointsChanged();
        auto judgement = points->getJudgementEnum();
        if (judgement == Judgement::Bad) {
            resetCombo();
        } else {
            increaseCombo();
        }
    } else {
        hitsWithoutPoints.push_back(tap);
    }
    emit hit(tap);
}
auto
BmsScore::addMisses(QVector<Miss> newMisses) -> void
{
    if (newMisses.empty()) {
        return;
    }
    resetCombo();
    for (const auto& miss : newMisses) {
        misses.append(miss);
    }
    judgementCounts[Judgement::Poor] += newMisses.size();
    emit judgementCountsChanged();
    auto newMissesVariantList = QVariantList{};
    for (const auto& miss : newMisses) {
        newMissesVariantList.append(QVariant::fromValue(miss));
    }
    for (auto* gauge : gauges) {
        for (const auto& miss : newMisses) {
            gauge->addHit(
              std::chrono::nanoseconds(miss.getOffsetFromStart()),
              std::chrono::nanoseconds(miss.getPoints().getDeviation()));
        }
    }
    emit missed(std::move(newMissesVariantList));
}
BmsScore::BmsScore(int maxHits,
                   double maxHitValue,
                   QList<rules::BmsGauge*> gauges,
                   QObject* parent)
  : QObject(parent)
  , maxPoints(maxHits * maxHitValue)
  , maxHits(maxHits)
  , gauges(std::move(gauges))
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
auto
BmsScore::sendVisualOnlyTap(Tap tap) -> void
{
    emit hit(tap);
}
auto
BmsScore::getCombo() const -> int
{
    return combo;
}
auto
BmsScore::getMaxCombo() const -> int
{
    return maxCombo;
}
auto
BmsScore::getGauges() const -> QList<rules::BmsGauge*>
{
    return gauges;
}
void
BmsScore::increaseCombo()
{
    combo++;
    if (combo > maxCombo) {
        maxCombo = combo;
        emit maxComboChanged();
    }
    emit comboChanged();
}
void
BmsScore::resetCombo()
{
    combo = 0;
    emit comboChanged();
}

} // namespace gameplay_logic