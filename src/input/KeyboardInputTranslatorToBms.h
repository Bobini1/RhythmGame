//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
#define RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H

#include <SFML/Window/Keyboard.hpp>
#include <optional>
#include "BmsKeys.h"
#include <boost/container/flat_map.hpp>

namespace input {
class KeyboardInputTranslatorToBms
{
    boost::container::flat_map<sf::Keyboard::Key, BmsKey> keyMap = {
        { sf::Keyboard::S, BmsKey::Col11 },
        { sf::Keyboard::D, BmsKey::Col12 },
        { sf::Keyboard::F, BmsKey::Col13 },
        { sf::Keyboard::Space, BmsKey::Col14 },
        { sf::Keyboard::J, BmsKey::Col15 },
        { sf::Keyboard::K, BmsKey::Col16 },
        { sf::Keyboard::L, BmsKey::Col17 },
        { sf::Keyboard::SemiColon, BmsKey::Col1s },
        { sf::Keyboard::A, BmsKey::Col21 },
        { sf::Keyboard::Q, BmsKey::Col22 },
        { sf::Keyboard::W, BmsKey::Col23 },
        { sf::Keyboard::E, BmsKey::Col24 },
        { sf::Keyboard::U, BmsKey::Col25 },
        { sf::Keyboard::I, BmsKey::Col26 },
        { sf::Keyboard::O, BmsKey::Col27 },
        { sf::Keyboard::P, BmsKey::Col2s }
    };

  public:
    auto translate(sf::Keyboard::Key key) -> std::optional<BmsKey>;
};
} // namespace input

#endif // RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
