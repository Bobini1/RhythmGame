//
// Created by bobini on 22.06.23.
//

#include "KeyboardInputTranslatorToBms.h"
auto
input::KeyboardInputTranslatorToBms::translate(Qt::Key key)
  -> std::optional<BmsKey>
{
    auto elem = keyMap.find(key);
    if (elem != keyMap.end()) {
        return elem->second;
    }
    return std::nullopt;
}
