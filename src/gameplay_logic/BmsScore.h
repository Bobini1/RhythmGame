//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <vector>
#include <boost/container/flat_map.hpp>
#include <QObject>
#include <QtQmlIntegration>
#include <magic_enum.hpp>
#include "gameplay_logic/TimePoint.h"
#include "gameplay_logic/BmsPoints.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "input/BmsKeys.h"
#include "Tap.h"
#include "Miss.h"
#include "BmsResult.h"
#include "BmsGaugeHistory.h"
#include "BmsReplayData.h"

namespace gameplay_logic {

class BmsScore : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    Q_PROPERTY(QVector<int> judgementCounts READ getJudgementCounts NOTIFY
                 judgementCountsChanged)
    Q_PROPERTY(QList<rules::BmsGauge*> gauges READ getGauges CONSTANT)

    double maxPoints;
    int maxHits;
    QList<Miss> misses;
    QList<Tap> hitsWithPoints;
    QList<Tap> hitsWithoutPoints;
    QList<rules::BmsGauge*> gauges;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
    double points = 0;
    int combo = 0;
    int maxCombo = 0;

    void resetCombo();
    void increaseCombo();

  public:
    auto addTap(Tap tap) -> void;
    // for when the chart isn't loaded yet
    auto sendVisualOnlyTap(Tap tap) -> void;
    void addMisses(QVector<Miss> misses);

    explicit BmsScore(int maxHits,
                      double maxHitValue,
                      QList<rules::BmsGauge*> gauges,
                      QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getPoints() const -> double;
    auto getJudgementCounts() const -> QVector<int>;
    auto getCombo() const -> int;
    auto getMaxCombo() const -> int;
    auto getGauges() const -> QList<rules::BmsGauge*>;

    auto getResult() const -> std::unique_ptr<BmsResult>;
    auto getReplayData() const -> std::unique_ptr<BmsReplayData>;
    auto getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>;

  signals:
    void pointsChanged();
    void judgementCountsChanged();
    void comboChanged();
    void maxComboChanged();

    void missed(QVariantList misses);
    void hit(Tap hit);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
