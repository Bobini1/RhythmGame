//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
#define RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H

#include <QKeyEvent>
#include <optional>
#include "BmsKeys.h"
#include <boost/container/flat_map.hpp>

namespace input {
class KeyboardInputTranslatorToBms
{
    boost::container::flat_map<Qt::Key, BmsKey> keyMap = {
        { Qt::Key::Key_S, BmsKey::Col11 },
        { Qt::Key::Key_D, BmsKey::Col12 },
        { Qt::Key::Key_F, BmsKey::Col13 },
        { Qt::Key::Key_Space, BmsKey::Col14 },
        { Qt::Key::Key_J, BmsKey::Col15 },
        { Qt::Key::Key_K, BmsKey::Col16 },
        { Qt::Key::Key_L, BmsKey::Col17 },
        { Qt::Key::Key_A, BmsKey::Col1s },
        { Qt::Key::Key_Tab, BmsKey::Col21 },
        { Qt::Key::Key_Q, BmsKey::Col22 },
        { Qt::Key::Key_W, BmsKey::Col23 },
        { Qt::Key::Key_E, BmsKey::Col24 },
        { Qt::Key::Key_U, BmsKey::Col25 },
        { Qt::Key::Key_I, BmsKey::Col26 },
        { Qt::Key::Key_O, BmsKey::Col27 },
        { Qt::Key::Key_P, BmsKey::Col2s }
    };

  public:
    auto translate(Qt::Key key) -> std::optional<BmsKey>;
};
} // namespace input

#endif // RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
