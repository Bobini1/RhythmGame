//
// Created by bobini on 23.06.23.
//

#include <magic_enum.hpp>
#include "BmsPoints.h"
gameplay_logic::BmsPoints::BmsPoints(double value,
                                     gameplay_logic::Judgement judgement,
                                     int64_t deviation,
                                     bool noteRemoved)
  : value(value)
  , judgement(judgement)
  , deviation(deviation)
  , noteRemoved(noteRemoved)
{
}
auto
gameplay_logic::BmsPoints::getValue() const -> double
{
    return value;
}
auto
gameplay_logic::BmsPoints::getJudgementEnum() const -> gameplay_logic::Judgement
{
    return judgement;
}
auto
gameplay_logic::BmsPoints::getDeviation() const -> int64_t
{
    return deviation;
}
auto
gameplay_logic::BmsPoints::getNoteRemoved() const -> bool
{
    return noteRemoved;
}
auto
gameplay_logic::BmsPoints::getJudgement() const -> QString
{
    auto name = magic_enum::enum_name(judgement);
    return QString::fromStdString(std::string(name.begin(), name.end()));
}
