//
// Created by PC on 16/06/2025.
//

#include "CustomNotifyApp.h"

namespace input {
void
CustomNotifyApp::setInputTranslator(InputTranslator* translator)
{
    inputTranslator = translator;
}
bool
CustomNotifyApp::notify(QObject* receiver, QEvent* event)
{
    const auto notifyResult = QGuiApplication::notify(receiver, event);
    if (inputTranslator != nullptr) {
        inputTranslator->eventFilter(receiver, event);
    }
    return notifyResult;
}
} // input