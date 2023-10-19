//
// Created by bobini on 25.06.23.
//

#include "BmsScore.h"
#include <magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace gameplay_logic {
auto
BmsScore::addNoteHit(HitEvent tap) -> void
{
    auto points = *tap.getPointsOptional();
    hitsWithPoints.emplace_back(tap);
    this->points += points.getValue();
    auto judgement = points.getJudgement();
    judgementCounts[static_cast<int>(judgement)]++;
    emit judgementCountsChanged();
    for (auto* gauge : gauges) {
        gauge->addHit(std::chrono::nanoseconds(tap.getOffsetFromStart()),
                      std::chrono::nanoseconds(points.getDeviation()));
    }
    if (points.getValue() != 0.0) {
        emit pointsChanged();
    }
    if (judgement == Judgement::Bad) {
        resetCombo();
    } else {
        increaseCombo();
    }
    emit noteHit(tap);
    emit pressed(tap.getColumn());
}
auto
BmsScore::addMisses(QList<HitEvent> newMisses) -> void
{
    if (newMisses.empty()) {
        return;
    }
    resetCombo();
    auto newPointSum = 0.0;
    for (const auto& miss : newMisses) {
        misses.append(miss);
        points += miss.getPointsOptional()->getValue();
        newPointSum += miss.getPointsOptional()->getValue();
    }
    if (newPointSum != 0) {
        emit pointsChanged();
    }
    judgementCounts[static_cast<int>(Judgement::Poor)] += newMisses.size();
    emit judgementCountsChanged();
    for (auto* gauge : gauges) {
        for (const auto& miss : newMisses) {
            gauge->addHit(std::chrono::nanoseconds(miss.getOffsetFromStart()),
                          std::chrono::nanoseconds(
                            miss.getPointsOptional()->getDeviation()));
        }
    }
    emit missed(newMisses);
}
BmsScore::BmsScore(int normalNoteCount,
                   int lnCount,
                   int mineCount,
                   int maxHits,
                   double maxHitValue,
                   QList<rules::BmsGauge*> gauges,
                   QObject* parent)
  : QObject(parent)
  , maxPoints(maxHitValue * maxHits)
  , mineCount(mineCount)
  , normalNoteCount(normalNoteCount)
  , lnCount(lnCount)
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
BmsScore::sendVisualOnlyTap(HitEvent tap) -> void
{
    emit emptyHit(tap);
    emit pressed(tap.getColumn());
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
    } else if (judgementCounts[static_cast<int>(Judgement::Perfect)] +
                 judgementCounts[static_cast<int>(Judgement::Great)] ==
               maxHits) {
        clearType = QStringLiteral("PERFECT");
    }
    return std::make_unique<BmsResult>(maxPoints,
                                       maxHits,
                                       normalNoteCount,
                                       lnCount,
                                       mineCount,
                                       clearType,
                                       judgementCounts,
                                       mineHits.size(),
                                       points,
                                       maxCombo);
}
auto
BmsScore::getReplayData() const -> std::unique_ptr<BmsReplayData>
{
    return std::make_unique<BmsReplayData>(misses,
                                           hitsWithPoints,
                                           hitsWithoutPoints,
                                           releasesWithoutPoints,
                                           mineHits,
                                           lnEndHits,
                                           lnEndMisses,
                                           lnEndSkips);
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
void
BmsScore::addMineHits(QVector<MineHit> mineHits)
{
    for (const auto& mineHit : mineHits) {
        for (auto* gauge : gauges) {
            gauge->addMineHit(
              std::chrono::nanoseconds(mineHit.getOffsetFromStart()),
              mineHit.getPenalty());
        }
    }
    this->mineHits.append(mineHits);
    emit minesHit(mineHits);
}
void
BmsScore::addLnEndHit(HitEvent lnEndHit)
{
    lnEndHits.append(lnEndHit);
    emit this->lnEndHit(lnEndHit);
    emit released(lnEndHit.getColumn());
}
void
BmsScore::addLnEndMisses(QList<HitEvent> lnEndMisses)
{
    this->lnEndMisses.append(lnEndMisses);
    emit lnEndMissed(lnEndMisses);
    resetCombo();
    for (const auto& miss : lnEndMisses) {
        if (miss.getPointsOptional()->getDeviation() < 0.0) {
            emit released(miss.getColumn());
        }
    }
}
void
BmsScore::addLnEndSkips(QList<HitEvent> lnEndSkips)
{
    this->lnEndSkips.append(lnEndSkips);
    emit lnEndSkipped(lnEndSkips);
}
auto
BmsScore::addEmptyHit(HitEvent tap) -> void
{
    hitsWithoutPoints.append(tap);
    emit emptyHit(tap);
    emit pressed(tap.getColumn());
}
auto
BmsScore::sendVisualOnlyRelease(HitEvent release) -> void
{
    emit emptyRelease(release);
    emit released(release.getColumn());
}
auto
BmsScore::addEmptyRelease(HitEvent release) -> void
{
    releasesWithoutPoints.append(release);
    emit emptyRelease(release);
    emit released(release.getColumn());
}
auto
BmsScore::getMineHits() const -> int
{
    return mineHits.count();
}
auto
BmsScore::getNormalNoteCount() const -> int
{
    return normalNoteCount;
}
auto
BmsScore::getLnCount() const -> int
{
    return lnCount;
}
auto
BmsScore::getMineCount() const -> int
{
    return mineCount;
}

} // namespace gameplay_logic