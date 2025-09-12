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
#ifdef _WIN32
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
#endif
    const auto notifyResult = QGuiApplication::notify(receiver, event);
    if (event->type() == QEvent::KeyPress ||
        event->type() == QEvent::KeyRelease) {
    }
    if (inputTranslator != nullptr && (event->type() == QEvent::KeyPress ||
                                       event->type() == QEvent::KeyRelease)) {
#ifndef _WIN32
        auto* event = static_cast<QKeyEvent*>(event);
        now = std::chrono::milliseconds{ event->timestamp() };
#endif
        inputTranslator->eventFilter(now, event);
    }
    return notifyResult;
}
} // namespace input