//
// Created by PC on 25/09/2024.
//

#include "InputAttached.h"

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

namespace qml_components {
void
InputSignalProvider::connectProfile(resource_managers::Profile* profile)
{
    connect(profile->getInputTranslator(),
            &input::InputTranslator::buttonPressed,
            this,
            [profile, this](const input::BmsKey button,
                            const double value,
                            const int64_t time) {
                emit buttonPressed(profile, button, value, time);
            });
    connect(profile->getInputTranslator(),
            &input::InputTranslator::buttonReleased,
            this,
            [profile, this](const input::BmsKey button, const int64_t time) {
                emit buttonReleased(profile, button, time);
            });
}
InputSignalProvider::
InputSignalProvider(ProfileList* profileList)
  : profileList(profileList)
{
    for (auto* profile : profileList->getProfiles()) {
        connectProfile(profile);
    }
    connect(profileList,
            &ProfileList::rowsInserted,
            this,
            [this, profileList](const QModelIndex& model, int begin, int end) {
                for (int i = begin; i <= end; ++i) {
                    auto* profile = profileList->at(i);
                    connectProfile(profile);
                }
            });
    // Disconnecting manually shouldn't be necessary.
}
auto
InputAttached::isAttachedToCurrentScene() const -> bool
{
    const auto* current = parent();
    while (current != nullptr) {
        if (current == (*findCurrentScene)()) {
            return true;
        }
        current = current->parent();
    }
    return false;
}
InputAttached::
InputAttached(QObject* obj)
  : QObject(obj)
{
    connect(inputSignalProvider,
            &InputSignalProvider::buttonPressed,
            this,
            [this](resource_managers::Profile* profile,
                   const input::BmsKey button,
                   const double value,
                   const int64_t time) {
                if (isAttachedToCurrentScene()) {
                    emit buttonPressed(profile, button, value, time);
                }
            });
    connect(inputSignalProvider,
            &InputSignalProvider::buttonReleased,
            this,
            [this](resource_managers::Profile* profile,
                   const input::BmsKey button,
                   const int64_t time) {
                if (isAttachedToCurrentScene()) {
                    emit buttonReleased(profile, button, time);
                }
            });
    connect(this,
            &InputAttached::buttonPressed,
            this,
            [this](resource_managers::Profile* profile,
                   const input::BmsKey button,
                   const double value,
                   const int64_t time) {
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::##capital:                                             \
        emit key##Pressed(profile, value, time);                               \
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
                    CASE(start, Start)
                    CASE(select, Select)
#undef CASE
                }
            });
    connect(this,
            &InputAttached::buttonReleased,
            this,
            [this](resource_managers::Profile* profile,
                   const input::BmsKey button,
                   const int64_t time) {
                switch (button) {
#define CASE(key, capital)                                                     \
    case input::BmsKey::##capital:                                             \
        emit key##Released(profile, time);                                     \
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
                    CASE(start, Start)
                    CASE(select, Select)
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