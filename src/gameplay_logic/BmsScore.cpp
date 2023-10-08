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
        auto judgement = points->getJudgement();
        judgementCounts[static_cast<int>(judgement)]++;
        emit judgementCountsChanged();
        for (auto* gauge : gauges) {
            gauge->addHit(std::chrono::nanoseconds(tap.getOffsetFromStart()),
                          std::chrono::nanoseconds(points->getDeviation()));
        }
        if (points->getValue() != 0.0) {
            emit pointsChanged();
        }
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
BmsScore::addMisses(QList<Miss> newMisses) -> void
{
    if (newMisses.empty()) {
        return;
    }
    resetCombo();
    auto newPointSum = 0.0;
    for (const auto& miss : newMisses) {
        misses.append(miss);
        points += miss.getPoints().getValue();
        newPointSum += miss.getPoints().getValue();
    }
    if (newPointSum != 0) {
        emit pointsChanged();
    }
    judgementCounts[static_cast<int>(Judgement::Poor)] += newMisses.size();
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
    for (auto* gauge : this->gauges) {
        gauge->setParent(this);
    }
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
BmsScore::getJudgementCounts() const -> QList<int>
{
    return judgementCounts;
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
auto
BmsScore::getResult() const -> std::unique_ptr<BmsResult>
{
    auto clearType = QStringLiteral("FAILED");
    for (auto* gauge : gauges) {
        if (gauge->getGauge() > gauge->getThreshold()) {
            clearType = gauge->objectName();
            break;
        }
    }
    if (points == maxPoints) {
        clearType = QStringLiteral("MAX");
    }
    if (judgementCounts[static_cast<int>(Judgement::Perfect)] +
          judgementCounts[static_cast<int>(Judgement::Great)] ==
        maxHits) {
        clearType = QStringLiteral("PERFECT");
    }
    return std::make_unique<BmsResult>(
      maxPoints, maxHits, clearType, judgementCounts, points, maxCombo);
}
auto
BmsScore::getReplayData() const -> std::unique_ptr<BmsReplayData>
{
    return std::make_unique<BmsReplayData>(
      misses, hitsWithPoints, hitsWithoutPoints);
}
auto
BmsScore::getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>
{
    auto gaugeHistory = QVariantMap{};
    for (auto* gauge : gauges) {
        gaugeHistory[gauge->objectName()] = gauge->getGaugeHistory();
    }
    return std::make_unique<BmsGaugeHistory>(gaugeHistory);
}

} // namespace gameplay_logic