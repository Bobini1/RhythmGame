//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_JUDGEMENT_H
#define RHYTHMGAME_JUDGEMENT_H
#include <QObject>

namespace gameplay_logic {
namespace judgement {
Q_NAMESPACE
/**
 * @brief The possible judgements for note hits.
 * @details You probably don't care about some of those, but they are useful for
 * the engine.
 */
enum class Judgement
{
    Poor,
    EmptyPoor,
    Bad,
    Good,
    Great,
    Perfect,
    MineHit,
    MineAvoided,
    LnEndSkip,
    LnBeginHit,
};
Q_ENUM_NS(Judgement)
}
using namespace judgement;
} // namespace gameplay_logic

#endif // RHYTHMGAME_JUDGEMENT_H
