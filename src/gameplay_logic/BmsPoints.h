//
// Created by bobini on 23.06.23.
//

#ifndef RHYTHMGAME_BMSPOINTS_H
#define RHYTHMGAME_BMSPOINTS_H
#include <QObject>
#include "Judgement.h"
namespace gameplay_logic {
class BmsPoints
{
    Q_GADGET
    Q_PROPERTY(double value READ getValue CONSTANT)
    Q_PROPERTY(Judgement judgement READ getJudgement CONSTANT)
    Q_PROPERTY(int64_t deviation READ getDeviation CONSTANT)
    double value;
    Judgement judgement;
    int64_t deviation;

  public:
    BmsPoints(double value, Judgement judgement, int64_t deviation);
    BmsPoints() = default;
    auto getValue() const -> double;
    auto getJudgement() const -> Judgement;
    auto getDeviation() const -> int64_t;

    friend auto operator<<(QDataStream& stream, const BmsPoints& points)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsPoints& points)
      -> QDataStream&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSPOINTS_H
