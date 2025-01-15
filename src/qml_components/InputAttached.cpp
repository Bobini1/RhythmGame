//
// Created by PC on 25/09/2024.
//

#include "InputAttached.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace qml_components {

InputSignalProvider::InputSignalProvider(
  input::InputTranslator* inputTranslator,
  QObject* parent)
  : QObject(parent)
  , profileList(profileList)
  , inputTranslator(inputTranslator)

{
    connect(inputTranslator,
            &input::InputTranslator::buttonPressed,
            this,
            &InputSignalProvider::buttonPressed);
    connect(inputTranslator,
            &input::InputTranslator::buttonReleased,
            this,
            &InputSignalProvider::buttonReleased);
}
auto
InputAttached::isAttachedToCurrentScene() const -> bool
{
    const auto* const currentScene = (*findCurrentScene)();
    const auto* current = parent();
    while (current != nullptr) {
        if (current == currentScene) {
            return true;
        }
        current = current->parent();
    }
    return false;
}
InputAttached::InputAttached(QObject* obj)
  : QObject(obj)
{
    connect(inputSignalProvider,
            &InputSignalProvider::buttonPressed,
            this,
            [this](const input::BmsKey button,
                   const double value,
                   const int64_t time) {
                if (isAttachedToCurrentScene()) {
                    emit buttonPressed(button, value, time);
                }
            });
    connect(inputSignalProvider,
            &InputSignalProvider::buttonReleased,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                if (isAttachedToCurrentScene()) {
                    emit buttonReleased(button, time);
                }
            });
    connect(this,
            &InputAttached::buttonPressed,
            this,
            [this](const input::BmsKey button,
                   const double value,
                   const int64_t time) {
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::capital:                                               \
        emit key##Pressed(value, time);                                        \
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
    connect(this,
            &InputAttached::buttonReleased,
            this,
            [this](const input::BmsKey button, const int64_t time) {
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::capital:                                               \
        emit key##Released(time);                                              \
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

InputSignalProvider* InputAttached::inputSignalProvider = nullptr;
std::function<QQuickItem*()>* InputAttached::findCurrentScene = nullptr;
} // namespace qml_components