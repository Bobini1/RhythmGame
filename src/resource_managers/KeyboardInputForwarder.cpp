//
// Created by PC on 24/09/2024.
//

#include "KeyboardInputForwarder.h"

#include "qml_components/ProfileList.h"

namespace resource_managers {
KeyboardInputForwarder::
KeyboardInputForwarder(qml_components::ProfileList* profileList,
                       QObject* parent)
  : QObject(parent)
  , profileList(profileList)
{
}
auto
KeyboardInputForwarder::eventFilter(QObject* watched, QEvent* event) -> bool
{
    auto ret = false;
    for (auto i = 0; i < profileList->rowCount(); ++i) {
        const auto* const profile = profileList->at(i);
        ret |= profile->getInputTranslator()->eventFilter(watched, event);
    }
    return ret;
}
} // namespace resource_managers