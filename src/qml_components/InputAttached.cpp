//
// Created by PC on 25/09/2024.
//

#include "InputAttached.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace qml_components {

auto
InputAttached::isEnabled() const -> bool
{
    auto p = parent();
    const QQuickItem* current = nullptr;
    while (p) {
        if (const auto item = qobject_cast<QQuickItem*>(p)) {
            current = item;
            break;
        }
        p = p->parent();
    }
    return current && current->isEnabled();
}
InputAttached::InputAttached(QObject* obj)
  : QObject(obj)
{
    connect(inputSignalProvider,
            &input::InputTranslator::buttonPressed,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                if (isEnabled()) {
                    emit buttonPressed(button, time);
                }
            });
    connect(inputSignalProvider,
            &input::InputTranslator::buttonReleased,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                if (isEnabled()) {
                    emit buttonReleased(button, time);
                }
            });
    connect(inputSignalProvider,
            &input::InputTranslator::buttonPressed,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                if (!isEnabled()) {
                    return;
                }
                bool old{};
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::capital:                                               \
        old = keyStates[static_cast<int>(input::BmsKey::capital)];             \
        keyStates[static_cast<int>(input::BmsKey::capital)] = true;            \
        if (!old) {                                                            \
            emit key##Changed();                                               \
        }                                                                      \
        emit key##Pressed(time);                                               \
        break;
                    CASE(col11, Col11)
                    CASE(col12, Col12)
                    CASE(col13, Col13)
                    CASE(col14, Col14)
                    CASE(col15, Col15)
                    CASE(col16, Col16)
                    CASE(col17, Col17)
                    CASE(col1sUp, Col1sUp)
                    CASE(col21, Col21)
                    CASE(col22, Col22)
                    CASE(col23, Col23)
                    CASE(col24, Col24)
                    CASE(col25, Col25)
                    CASE(col26, Col26)
                    CASE(col27, Col27)
                    CASE(col2sUp, Col2sUp)
                    CASE(col1sDown, Col1sDown)
                    CASE(col2sDown, Col2sDown)
                    CASE(start1, Start1)
                    CASE(select1, Select1)
                    CASE(start2, Start2)
                    CASE(select2, Select2)
#undef CASE
                }
            });
    connect(inputSignalProvider,
            &input::InputTranslator::buttonReleased,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                bool old{};
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::capital:                                               \
        old = keyStates[static_cast<int>(input::BmsKey::capital)];             \
        keyStates[static_cast<int>(input::BmsKey::capital)] = false;           \
        if (old) {                                                             \
            emit key##Changed();                                               \
        }                                                                      \
        if (isEnabled()) {                                                     \
            emit key##Released(time);                                          \
        }                                                                      \
        break;
                    CASE(col11, Col11)
                    CASE(col12, Col12)
                    CASE(col13, Col13)
                    CASE(col14, Col14)
                    CASE(col15, Col15)
                    CASE(col16, Col16)
                    CASE(col17, Col17)
                    CASE(col1sUp, Col1sUp)
                    CASE(col21, Col21)
                    CASE(col22, Col22)
                    CASE(col23, Col23)
                    CASE(col24, Col24)
                    CASE(col25, Col25)
                    CASE(col26, Col26)
                    CASE(col27, Col27)
                    CASE(col2sUp, Col2sUp)
                    CASE(col1sDown, Col1sDown)
                    CASE(col2sDown, Col2sDown)
                    CASE(start1, Start1)
                    CASE(select1, Select1)
                    CASE(start2, Start2)
                    CASE(select2, Select2)
#undef CASE
                }
            });
}

auto
InputAttached::qmlAttachedProperties(QObject* object) -> InputAttached*
{
    return new InputAttached(object);
}

input::InputTranslator* InputAttached::inputSignalProvider = nullptr;
} // namespace qml_components