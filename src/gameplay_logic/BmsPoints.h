//
// Created by bobini on 23.06.23.
//

#ifndef RHYTHMGAME_BMSPOINTS_H
#define RHYTHMGAME_BMSPOINTS_H
#include <boost/serialization/strong_typedef.hpp>
#include <chrono>
#include "Judgement.h"
namespace gameplay_logic {
class BmsPoints
{
    Q_GADGET
    Q_PROPERTY(double value READ getValue CONSTANT)
    Q_PROPERTY(Judgement judgement READ getJudgement CONSTANT)
    Q_PROPERTY(int64_t deviation READ getDeviation CONSTANT)
    Q_PROPERTY(bool noteRemoved READ getNoteRemoved CONSTANT)
    double value;
    Judgement judgement;
    int64_t deviation;
    bool noteRemoved;

  public:
    BmsPoints(double value,
              Judgement judgement,
              int64_t deviation,
              bool noteRemoved);
    BmsPoints() = default;
    [[nodiscard]] auto getValue() const -> double;
    [[nodiscard]] auto getJudgement() const -> Judgement;
    [[nodiscard]] auto getDeviation() const -> int64_t;
    auto getNoteRemoved() const -> bool;

    friend auto operator<<(QDataStream& stream, const BmsPoints& points)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsPoints& points)
      -> QDataStream&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSPOINTS_H
