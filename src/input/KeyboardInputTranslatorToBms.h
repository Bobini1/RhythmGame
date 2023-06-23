//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
#define RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H

class KeyboardInputTranslatorToBms
{
  public:
    auto translate(sf::Keyboard::Key key) -> std::string;
};

#endif // RHYTHMGAME_KEYBOARDINPUTTRANSLATORTOBMS_H
