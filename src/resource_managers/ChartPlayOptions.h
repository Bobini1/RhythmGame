#ifndef RHYTHMGAME_CHARTPLAYOPTIONS_H
#define RHYTHMGAME_CHARTPLAYOPTIONS_H

#include <QObject>

namespace resource_managers {

namespace note_order_algorithm {
Q_NAMESPACE
enum class NoteOrderAlgorithm
{
    Normal,
    Mirror,
    Random,
    SRandom,
    RRandom,
    RandomPlus,
    SRandomPlus,
    BeatorajaRandom,
    BeatorajaRandomEx,
    Lr2Random,
    Lr2RandomEx,
};
Q_ENUM_NS(NoteOrderAlgorithm)
} // namespace note_order_algorithm
using namespace note_order_algorithm;

namespace dp_options {
Q_NAMESPACE
enum class DpOptions
{
    Off,
    Flip,
    Battle,
    Lr2Flip
};
Q_ENUM_NS(DpOptions)
} // namespace dp_options
using namespace dp_options;

} // namespace resource_managers

#endif // RHYTHMGAME_CHARTPLAYOPTIONS_H
