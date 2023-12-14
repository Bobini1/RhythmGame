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
    Col12 = 1,
    Col13 = 2,
    Col14 = 3,
    Col15 = 4,
    Col16 = 5,
    Col17 = 6,
    Col1s = 7,
    Col21 = 8,
    Col22 = 9,
    Col23 = 10,
    Col24 = 11,
    Col25 = 12,
    Col26 = 13,
    Col27 = 14,
    Col2s = 15,
    Start = 16,
    Select = 17,
};
Q_ENUM_NS(BmsKey);
} // namespace input

#endif // RHYTHMGAME_BMSKEYS_H
