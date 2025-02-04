//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <magic_enum/magic_enum.hpp>
#include "gameplay_logic/rules/BmsGauge.h"
#include "HitEvent.h"
#include "BmsResult.h"
#include "BmsGaugeHistory.h"
#include "BmsReplayData.h"
#include "MineHit.h"
#include "resource_managers/Vars.h"

namespace gameplay_logic {

class BmsScore final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    Q_PROPERTY(QVector<int> judgementCounts READ getJudgementCounts NOTIFY
                 judgementCountsChanged)
    Q_PROPERTY(int mineHits READ getMineHits NOTIFY mineHit)
    Q_PROPERTY(QList<rules::BmsGauge*> gauges READ getGauges CONSTANT)
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm CONSTANT)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 CONSTANT)
    Q_PROPERTY(QList<int> permutation READ getPermutation CONSTANT)
    Q_PROPERTY(uint64_t randomSeed READ getRandomSeed CONSTANT)

    double maxPoints;
    int mineCount;
    int normalNoteCount;
    int lnCount;
    int maxHits;
    QList<HitEvent> misses;
    QList<HitEvent> hitsWithPoints;
    QList<HitEvent> hitsWithoutPoints;
    QList<HitEvent> releasesWithoutPoints;
    QList<MineHit> mineHits;
    QList<HitEvent> lnEndHits;
    QList<HitEvent> lnEndMisses;
    QList<HitEvent> lnEndSkips;
    QList<rules::BmsGauge*> gauges;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
    QList<qint64> randomSequence;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithm;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2;
    QList<int> permutation;
    QString sha256;
    QString md5;
    double points = 0;
    int combo = 0;
    int maxCombo = 0;
    uint64_t randomSeed;

    void resetCombo();
    void increaseCombo();

  public:
    auto addNoteHit(HitEvent tap) -> void;
    // for when the chart isn't loaded yet
    auto sendVisualOnlyTap(HitEvent tap) -> void;
    auto sendVisualOnlyRelease(HitEvent release) -> void;
    auto addEmptyHit(HitEvent tap) -> void;
    auto addEmptyRelease(HitEvent release) -> void;
    void addMiss(HitEvent misses);
    void addMineHit(MineHit mineHits);
    void addLnEndHit(HitEvent lnEndHit);
    void addLnEndMiss(HitEvent lnEndMiss);
    void addLnEndSkip(HitEvent lnEndSkips);

    explicit BmsScore(
      int normalNoteCount,
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
      QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getNormalNoteCount() const -> int;
    auto getLnCount() const -> int;
    auto getMineCount() const -> int;
    auto getPoints() const -> double;
    auto getJudgementCounts() const -> QVector<int>;
    auto getCombo() const -> int;
    auto getMaxCombo() const -> int;
    auto getMineHits() const -> int;
    auto getGauges() const -> QList<rules::BmsGauge*>;
    auto getRandomSequence() const -> const QList<qint64>&;
    auto getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm;
    auto getNoteOrderAlgorithmP2() const
      -> resource_managers::NoteOrderAlgorithm;
    auto getPermutation() const -> const QList<int>&;
    auto getRandomSeed() const -> uint64_t;

    auto getResult() const -> std::unique_ptr<BmsResult>;
    auto getReplayData() const -> std::unique_ptr<BmsReplayData>;
    auto getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>;

  signals:
    void pointsChanged();
    void judgementCountsChanged();
    void comboChanged();
    void maxComboChanged();

    void missed(HitEvent misses);
    void mineHit(MineHit mineHits);
    void emptyHit(HitEvent hit);
    void emptyRelease(HitEvent release);
    void noteHit(HitEvent hit);
    void lnEndHit(HitEvent lnEndHit);
    void lnEndMissed(HitEvent lnEndMiss);
    void lnEndSkipped(HitEvent lnEndSkip);

    void pressed(int column);
    void released(int column);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
