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
    Q_PROPERTY(QVariantMap judgementCounts READ getJudgementCounts NOTIFY
                 judgementCountsChanged)

    double maxPoints;
    int maxHits;
    QVector<Miss> misses;
    QVector<Tap> hitsWithPoints;
    QVector<Tap> hitsWithoutPoints;
    double points = 0;
    boost::container::flat_map<Judgement, int> judgementCounts;

  public:
    auto addTap(Tap tap) -> void;
    auto addMiss(Miss miss) -> void;

    explicit BmsScore(int maxHits, QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getPoints() const -> double;
    auto getJudgementCounts() const -> const QVariantMap&;

  signals:
    void pointsChanged();
    void missesChanged();
    void hitsWithPointsChanged();
    void hitsWithoutPointsChanged();
    void judgementCountsChanged();

    void miss(Miss miss);
    void hit(Tap hit);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
