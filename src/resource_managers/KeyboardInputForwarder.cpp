//
// Created by PC on 24/09/2024.
//

#include "KeyboardInputForwarder.h"

#include "InputTranslators.h"
#include "qml_components/ProfileList.h"

namespace resource_managers {
KeyboardInputForwarder::KeyboardInputForwarder(
  InputTranslators* inputTranslators,
  QObject* parent)
  : QObject(parent)
  , inputTranslators(inputTranslators)
{
}
auto
KeyboardInputForwarder::eventFilter(QObject* watched, QEvent* event) -> bool
{
    auto ret = false;
    for (auto* inputTranslator : inputTranslators->getInputTranslators()) {
        ret |= inputTranslator->eventFilter(watched, event);
    }
    return ret;
}
} // namespace resource_managers