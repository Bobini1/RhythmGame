//
// Created by PC on 16/06/2025.
//

#include "CustomNotifyApp.h"
#include <QInputMethodEvent>
#include <spdlog/spdlog.h>

namespace input {

#ifdef _WIN32
CustomNotifyApp* CustomNotifyApp::s_instance = nullptr;

// ---------------------------------------------------------------------------
// Low-level keyboard hook
//
// WH_KEYBOARD_LL fires at the kernel level, *before* the Windows IME can
// swallow key events.  This is necessary for keyboards / input methods (e.g.
// Chinese Pinyin) that otherwise intercept Alt-key combinations.
//
// Key design decisions:
//  • LLKHF_INJECTED on a VK_LCONTROL event means it is the synthetic Left-Ctrl
//    that Windows injects immediately before every AltGr (Right-Alt) key-down.
//    We skip it so it does not pollute the game's key bindings.
//  • We track pressed scan codes ourselves to suppress OS-generated auto-repeat
//    messages (pressButton already guards against double-press, but skipping
//    early avoids unnecessary work and keeps configuring mode clean).
//  • We read QGuiApplication::focusObject() to avoid stealing input from any
//    focused text-entry widget (search bar, rename field, …).
//  • CallNextHookEx is ALWAYS called so Qt and the rest of the system receive
//    every event normally.
// ---------------------------------------------------------------------------
LRESULT CALLBACK
CustomNotifyApp::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && s_instance != nullptr &&
        s_instance->inputTranslator != nullptr) {

        const auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (isDown || isUp) {
            // Skip the synthetic Left-Ctrl that Windows injects before every
            // AltGr press.  In WH_KEYBOARD_LL this event carries LLKHF_INJECTED
            // (it does NOT come from a physical key stroke).
            if (kbd->vkCode == VK_LCONTROL && (kbd->flags & LLKHF_INJECTED)) {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            // Build the same scan-code value Qt's nativeScanCode() would
            // return: 8-bit hardware scan code ORed with 0x100 for extended
            // keys.
            const quint32 scanCode =
              kbd->scanCode | ((kbd->flags & LLKHF_EXTENDED) ? 0x100u : 0u);

            // Suppress auto-repeat key-down messages.
            if (isDown) {
                if (s_instance->m_pressedScanCodes.count(scanCode)) {
                    return CallNextHookEx(nullptr, nCode, wParam, lParam);
                }
                s_instance->m_pressedScanCodes.insert(scanCode);
            } else {
                s_instance->m_pressedScanCodes.erase(scanCode);
            }

            if (focusWindow() == nullptr) {
                return CallNextHookEx(nullptr, nCode, wParam, lParam);
            }

            // Do not process game input while a text-entry widget has focus
            // (search bar, rename field, etc.).
            bool textInputActive = false;
            if (auto* focusObj = QGuiApplication::focusObject()) {
                QInputMethodQueryEvent query(Qt::ImEnabled);
                QCoreApplication::sendEvent(focusObj, &query);
                textInputActive = query.value(Qt::ImEnabled).toBool();
            }

            if (!textInputActive) {
                const auto now =
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch());
                s_instance->inputTranslator->handleKeyEvent(
                  scanCode, isDown, now.count());
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void
CustomNotifyApp::installHook()
{
    m_keyboardHook =
      SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (m_keyboardHook == nullptr) {
        spdlog::error("CustomNotifyApp: failed to install WH_KEYBOARD_LL hook "
                      "(error {})",
                      GetLastError());
    }
    s_instance = this;
}

void
CustomNotifyApp::removeHook()
{
    if (m_keyboardHook != nullptr) {
        UnhookWindowsHookEx(m_keyboardHook);
        m_keyboardHook = nullptr;
    }
    s_instance = nullptr;
}
#endif // _WIN32

CustomNotifyApp::~CustomNotifyApp()
{
#ifdef _WIN32
    removeHook();
#endif
}

void
CustomNotifyApp::setInputTranslator(InputTranslator* translator)
{
    inputTranslator = translator;
#ifdef _WIN32
    if (translator != nullptr) {
        installHook();
    }
#endif
}

bool
CustomNotifyApp::notify(QObject* receiver, QEvent* event)
{
#ifndef _WIN32
    // On Windows the WH_KEYBOARD_LL hook handles game input before the IME
    // can intercept it.  On other platforms we handle it here instead.
    if (inputTranslator != nullptr && (event->type() == QEvent::KeyPress ||
                                       event->type() == QEvent::KeyRelease)) {
        bool textInputActive = false;
        if (auto* focusObj = QGuiApplication::focusObject()) {
            QInputMethodQueryEvent query(Qt::ImEnabled);
            QCoreApplication::sendEvent(focusObj, &query);
            textInputActive = query.value(Qt::ImEnabled).toBool();
        }

        if (!textInputActive) {
            auto* ev = static_cast<QKeyEvent*>(event);
            auto now = std::chrono::milliseconds{ ev->timestamp() };
            inputTranslator->eventFilter(now, event);
        }
    }
#endif
    return QGuiApplication::notify(receiver, event);
}

} // namespace input
