//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_JUDGEMENT_H
#define RHYTHMGAME_JUDGEMENT_H
#include <QObject>

namespace gameplay_logic {
Q_NAMESPACE
enum class Judgement
{
    Poor,
    EmptyPoor,
    Bad,
    Good,
    Great,
    Perfect
};
Q_ENUM_NS(Judgement)
} // namespace gameplay_logic

#endif // RHYTHMGAME_JUDGEMENT_H
