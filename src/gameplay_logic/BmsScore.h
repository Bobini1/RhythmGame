//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <magic_enum.hpp>
#include "gameplay_logic/rules/BmsGauge.h"
#include "HitEvent.h"
#include "BmsResult.h"
#include "BmsGaugeHistory.h"
#include "BmsReplayData.h"
#include "MineHit.h"

namespace gameplay_logic {

class BmsScore : public QObject
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
    support::Sha256 sha256;
    double points = 0;
    int combo = 0;
    int maxCombo = 0;

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

    explicit BmsScore(int normalNoteCount,
                      int lnCount,
                      int mineCount,
                      int maxHits,
                      double maxHitValue,
                      QList<rules::BmsGauge*> gauges,
                      QList<qint64> randomSequence,
                      support::Sha256 sha256,
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
