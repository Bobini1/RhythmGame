//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <vector>
#include <boost/container/flat_map.hpp>
#include <QObject>
#include <QtQmlIntegration>
#include "gameplay_logic/TimePoint.h"
#include "gameplay_logic/BmsPoints.h"
#include "gameplay_logic/rules/BmsGauge.h"
#include "input/BmsKeys.h"
#include "Tap.h"
#include "Miss.h"

namespace gameplay_logic {

class BmsScore : public QObject
{
    Q_OBJECT
  public:
    // nanoseconds
    using DeltaTime = uint64_t;

  private:
    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    Q_PROPERTY(QVariantMap judgementCounts READ getJudgementCounts NOTIFY
                 judgementCountsChanged)
    Q_PROPERTY(QList<rules::BmsGauge*> gauges READ getGauges CONSTANT)

    double maxPoints;
    int maxHits;
    QList<Miss> misses;
    QList<Tap> hitsWithPoints;
    QList<Tap> hitsWithoutPoints;
    QList<rules::BmsGauge*> gauges;
    boost::container::flat_map<Judgement, int> judgementCounts;
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
    auto getJudgementCounts() const -> QVariantMap;
    auto getCombo() const -> int;
    auto getMaxCombo() const -> int;
    auto getGauges() const -> QList<rules::BmsGauge*>;

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
