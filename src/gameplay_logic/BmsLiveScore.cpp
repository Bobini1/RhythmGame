//
// Created by bobini on 25.06.23.
//

#include "BmsLiveScore.h"
#include <magic_enum/magic_enum.hpp>
#include <utility>
#include <spdlog/spdlog.h>

namespace gameplay_logic {
BmsLiveScore::BmsLiveScore(int normalNoteCount,
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
                   QString md5,
                   QString guid,
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
  , md5(std::move(md5))
  , randomSeed(seed)
{
    for (auto* gauge : this->gauges) {
        gauge->setParent(this);
    }
}
auto
BmsLiveScore::getMaxPoints() const -> double
{
    return maxPoints;
}
auto
BmsLiveScore::getMaxHits() const -> int
{
    return maxHits;
}
auto
BmsLiveScore::getPoints() const -> double
{
    return points;
}
auto
BmsLiveScore::getJudgementCounts() -> JudgementCounts*
{
    return &judgementCounts;
}

auto
BmsLiveScore::getJudgementCounts() const -> const JudgementCounts*
{
    return &judgementCounts;
}

void
BmsLiveScore::sendVisualOnlyTap(HitEvent tap)
{
    emit hit(tap);
}
auto
BmsLiveScore::getCombo() const -> int
{
    return combo;
}
auto
BmsLiveScore::getMaxCombo() const -> int
{
    return maxCombo;
}
auto
BmsLiveScore::getGauges() const -> QList<rules::BmsGauge*>
{
    return gauges;
}
auto
BmsLiveScore::getRandomSequence() const -> const QList<qint64>&
{
    return randomSequence;
}
auto
BmsLiveScore::getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithm;
}
auto
BmsLiveScore::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithmP2;
}
auto
BmsLiveScore::getPermutation() const -> const QList<int>&
{
    return permutation;
}
auto
BmsLiveScore::getRandomSeed() const -> uint64_t
{
    return randomSeed;
}
auto
BmsLiveScore::getGuid() const -> QString
{
    return guid;
}
void
BmsLiveScore::increaseCombo()
{
    combo++;
    if (combo > maxCombo) {
        maxCombo = combo;
        emit maxComboChanged();
    }
    emit comboChanged();
}
auto
BmsLiveScore::addHit(HitEvent tap) -> void
{
    hits.append(tap);
    emit hit(tap);
    if (!tap.getPointsOptional()) {
        return;
    }
    const auto points = tap.getPointsOptional();
    judgementCounts.addJudgement(points->getJudgement());
    switch (points->getJudgement()) {
        case Judgement::Poor:
            resetCombo();
            this->points += points->getValue();
            if (points->getValue() != 0) {
                emit pointsChanged();
            }
            for (auto* gauge : gauges) {
                gauge->addHit(
                  std::chrono::nanoseconds(tap.getOffsetFromStart()),
                  std::chrono::nanoseconds(points->getDeviation()),
                  points->getJudgement());
            }
            break;
        case Judgement::MineHit:
            for (auto* gauge : gauges) {
                gauge->addMineHit(
                  std::chrono::nanoseconds(tap.getOffsetFromStart()),
                  tap.getPoints().value<double>());
            }
            ++mineHits;
            emit mineHitsChanged();
            break;
        case Judgement::MineAvoided:
            break;
        case Judgement::LnEndSkip:
            break;
        default:
            this->points += points->getValue();
            auto judgement = points->getJudgement();
            for (auto* gauge : gauges) {
                gauge->addHit(
                  std::chrono::nanoseconds(tap.getOffsetFromStart()),
                  std::chrono::nanoseconds(points->getDeviation()),
                  judgement);
            }
            if (points->getValue() != 0.0) {
                emit pointsChanged();
            }
            if (judgement == Judgement::Bad) {
                resetCombo();
            } else if (judgement != Judgement::EmptyPoor) {
                increaseCombo();
            }
    }
}
void
BmsLiveScore::resetCombo()
{
    combo = 0;
    emit comboChanged();
}
auto
BmsLiveScore::getResult() const -> std::unique_ptr<BmsResult>
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
    } else if (judgementCounts
                   .getJudgementCounts()[static_cast<int>(Judgement::Perfect)] +
                 judgementCounts
                   .getJudgementCounts()[static_cast<int>(Judgement::Great)] ==
               maxHits) {
        clearType = QStringLiteral("PERFECT");
    }
    auto mineHitsSize = std::ranges::count_if(hits, [](const auto& hit) {
        return hit.getPointsOptional() &&
               hit.getPointsOptional()->getJudgement() == Judgement::MineHit;
    });
    return std::make_unique<BmsResult>(maxPoints,
                                       maxHits,
                                       normalNoteCount,
                                       lnCount,
                                       mineCount,
                                       clearType,
                                       judgementCounts.getJudgementCounts(),
                                       mineHitsSize,
                                       points,
                                       maxCombo,
                                       randomSequence,
                                       guid,
                                       sha256,
                                       md5);
}
auto
BmsLiveScore::getReplayData() const -> std::unique_ptr<BmsReplayData>
{
    return std::make_unique<BmsReplayData>(hits, guid);
}
auto
BmsLiveScore::getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>
{
    auto gaugeHistory = QHash<QString, QList<rules::GaugeHistoryEntry>>{};
    auto gaugeInfo = QHash<QString, BmsGaugeInfo>{};
    for (const auto* gauge : gauges) {
        gaugeHistory[gauge->objectName()] = gauge->getGaugeHistory();
        gaugeInfo[gauge->objectName()] =
          BmsGaugeInfo{ gauge->getGaugeMax(), gauge->getThreshold() };
    }
    return std::make_unique<BmsGaugeHistory>(
      std::move(gaugeHistory), std::move(gaugeInfo), guid);
}
void
BmsLiveScore::sendVisualOnlyRelease(HitEvent release)
{
    emit hit(release);
}
auto
BmsLiveScore::getMineHits() const -> int
{
    return hits.count();
}
auto
BmsLiveScore::getNormalNoteCount() const -> int
{
    return normalNoteCount;
}
auto
BmsLiveScore::getLnCount() const -> int
{
    return lnCount;
}
auto
BmsLiveScore::getMineCount() const -> int
{
    return mineCount;
}

} // namespace gameplay_logic