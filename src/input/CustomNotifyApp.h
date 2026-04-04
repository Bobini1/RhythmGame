//
// Created by PC on 16/06/2025.
//

#ifndef CUSTOMNOTIFYAPP_H
#define CUSTOMNOTIFYAPP_H
#include "InputTranslator.h"

#include <QGuiApplication>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unordered_set>
#endif

namespace input {

class CustomNotifyApp final : public QGuiApplication
{
    Q_OBJECT
    using QGuiApplication::QGuiApplication;
    InputTranslator* inputTranslator = nullptr;

#ifdef _WIN32
    HHOOK m_keyboardHook = nullptr;
    // Tracks currently-pressed scan codes to filter out OS auto-repeat
    // messages before they reach handleKeyEvent.
    std::unordered_set<quint32> m_pressedScanCodes;

    static CustomNotifyApp* s_instance;
    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode,
                                                  WPARAM wParam,
                                                  LPARAM lParam);
    void installHook();
    void removeHook();
#endif

  public:
    ~CustomNotifyApp() override;
    void setInputTranslator(InputTranslator* translator);
    auto notify(QObject* receiver, QEvent* event) -> bool override;
};

} // input

#endif // CUSTOMNOTIFYAPP_H
