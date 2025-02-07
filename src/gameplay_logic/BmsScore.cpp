//
// Created by bobini on 25.06.23.
//

#include "BmsScore.h"
#include <magic_enum/magic_enum.hpp>
#include <utility>
#include <spdlog/spdlog.h>

namespace gameplay_logic {
BmsScore::BmsScore(int normalNoteCount,
                   int lnCount,
                   int mineCount,
                   int maxHits,
                   double maxHitValue,
                   QList<rules::BmsGauge*> gauges,
                   QList<qint64> randomSequence,
                   resource_managers::NoteOrderAlgorithm noteOrderAlgorithm,
                   resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2,
                   QList<int> permutation,
                   uint64_t seed,
                   QString sha256,
                   QObject* parent)
  : QObject(parent)
  , maxPoints(maxHitValue * maxHits)
  , mineCount(mineCount)
  , normalNoteCount(normalNoteCount)
  , lnCount(lnCount)
  , maxHits(maxHits)
  , gauges(std::move(gauges))
  , randomSequence(std::move(randomSequence))
  , noteOrderAlgorithm(noteOrderAlgorithm)
  , noteOrderAlgorithmP2(noteOrderAlgorithmP2)
  , permutation(std::move(permutation))
  , sha256(std::move(sha256))
  , randomSeed(seed)
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
void
BmsScore::sendVisualOnlyTap(HitEvent tap)
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
auto
BmsScore::getRandomSequence() const -> const QList<qint64>&
{
    return randomSequence;
}
auto
BmsScore::getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithm;
}
auto
BmsScore::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithmP2;
}
auto
BmsScore::getPermutation() const -> const QList<int>&
{
    return permutation;
}
auto
BmsScore::getRandomSeed() const -> uint64_t
{
    return randomSeed;
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
auto
BmsScore::addHit(HitEvent tap) -> void
{
    if (tap.getType() == HitEvent::HitType::Miss) {
        resetCombo();
        auto newPointSum = 0.0;
        points += tap.getPointsOptional()->getValue();
        newPointSum += tap.getPointsOptional()->getValue();
        if (newPointSum != 0) {
            emit pointsChanged();
        }
        judgementCounts[static_cast<int>(Judgement::Poor)]++;
        emit judgementCountsChanged();
        for (auto* gauge : gauges) {
            gauge->addHit(std::chrono::nanoseconds(tap.getOffsetFromStart()),
                          std::chrono::nanoseconds(
                            tap.getPointsOptional()->getDeviation()));
        }
    } else if (tap.getType() == HitEvent::HitType::Hit) {
        auto points = *tap.getPointsOptional();
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
        } else if (judgement != Judgement::EmptyPoor) {
            increaseCombo();
        }
    } else if (tap.getType() == HitEvent::HitType::None) {
    } else if (tap.getType() == HitEvent::HitType::LnEndSkip) {
    } else if (tap.getType() == HitEvent::HitType::Mine) {
        for (auto* gauge : gauges) {
            gauge->addMineHit(
              std::chrono::nanoseconds(tap.getOffsetFromStart()),
              tap.getPoints().value<double>());
        }
        ++mineHits;
        emit mineHitsChanged();
    }
    hits.append(tap);
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
    auto mineHitsSize = std::ranges::count_if(hits, [](const auto& hit) {
        return hit.getType() == HitEvent::HitType::Mine;
    });
    return std::make_unique<BmsResult>(maxPoints,
                                       maxHits,
                                       normalNoteCount,
                                       lnCount,
                                       mineCount,
                                       clearType,
                                       judgementCounts,
                                       mineHitsSize,
                                       points,
                                       maxCombo,
                                       randomSequence,
                                       sha256,
                                       md5);
}
auto
BmsScore::getReplayData() const -> std::unique_ptr<BmsReplayData>
{
    return std::make_unique<BmsReplayData>(hits);
}
auto
BmsScore::getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>
{
    auto gaugeHistory = QHash<QString, QList<rules::GaugeHistoryEntry>>{};
    auto gaugeInfo = QHash<QString, BmsGaugeInfo>{};
    for (auto* gauge : gauges) {
        gaugeHistory[gauge->objectName()] = gauge->getGaugeHistory();
        gaugeInfo[gauge->objectName()] =
          BmsGaugeInfo{ gauge->getGaugeMax(), gauge->getThreshold() };
    }
    return std::make_unique<BmsGaugeHistory>(std::move(gaugeHistory),
                                             std::move(gaugeInfo));
}
void
BmsScore::sendVisualOnlyRelease(HitEvent release)
{
    emit hit(release);
}
auto
BmsScore::getMineHits() const -> int
{
    return hits.count();
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