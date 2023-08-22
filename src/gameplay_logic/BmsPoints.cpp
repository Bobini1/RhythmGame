//
// Created by bobini on 23.06.23.
//

#include "BmsPoints.h"
gameplay_logic::BmsPoints::BmsPoints(double value,
                                     gameplay_logic::Judgement judgement,
                                     int64_t deviation)
  : value(value)
  , judgement(judgement)
  , deviation(deviation)
{
}
auto
gameplay_logic::BmsPoints::getValue() const -> double
{
    return value;
}
auto
gameplay_logic::BmsPoints::getJudgement() const -> gameplay_logic::Judgement
{
    return judgement;
}
auto
gameplay_logic::BmsPoints::getDeviation() const -> int64_t
{
    return deviation;
}
