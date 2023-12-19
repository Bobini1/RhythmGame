//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_BMSKEYS_H
#define RHYTHMGAME_BMSKEYS_H

#include <QObject>
namespace input {
Q_NAMESPACE
enum class BmsKey
{
    Col11 = 0,
    Col12,
    Col13,
    Col14,
    Col15,
    Col16,
    Col17,
    Col1sUp,
    Col21,
    Col22,
    Col23,
    Col24,
    Col25,
    Col26,
    Col27,
    Col2sUp,
    Start,
    Select,
    Col1sDown,
    Col2sDown,
};
Q_ENUM_NS(BmsKey);
} // namespace input

#endif // RHYTHMGAME_BMSKEYS_H
