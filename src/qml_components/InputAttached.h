//
// Created by PC on 25/09/2024.
//

#ifndef INPUT_H
#define INPUT_H
#include "ProfileList.h"
#include "resource_managers/Profile.h"

#include <QQuickItem>
#include <qqmlintegration.h>

namespace qml_components {

/**
 * @brief The class that provides the InputSignalProvider attached property.
 * This is not exposed to QML, use the Input attached property instead.
 */
class InputSignalProvider final : public QObject
{
    Q_OBJECT
    ProfileList* profileList;

    void connectProfile(resource_managers::Profile* profile);

  public:
    explicit InputSignalProvider(ProfileList* profileList);
  signals:
    void buttonPressed(resource_managers::Profile* profile,
                       input::BmsKey button,
                       double value,
                       int64_t time);
    void buttonReleased(resource_managers::Profile* profile,
                        input::BmsKey button,
                        int64_t time);
};

/**
 * @brief The class that provides the Input attached property.
 * QML components that wish react to game key events, like scratch or
 * buttons, should use Input.
 */
class InputAttached final : public QObject
{
    Q_OBJECT
    QML_ATTACHED(InputAttached)

    auto isAttachedToCurrentScene() const -> bool;

  public:
    explicit InputAttached(QObject* obj = nullptr);

    static InputSignalProvider* inputSignalProvider;
    static std::function<QQuickItem*()>* findCurrentScene;
    static auto qmlAttachedProperties(QObject* object) -> InputAttached*;

  signals:
    void buttonPressed(resource_managers::Profile* profile,
                       input::BmsKey button,
                       double value,
                       int64_t time);
    void buttonReleased(resource_managers::Profile* profile,
                        input::BmsKey button,
                        int64_t time);

    // too lazy to write these out
#define KEY(name)                                                              \
    void name##Pressed(                                                        \
      resource_managers::Profile* profile, double value, int64_t time);        \
    void name##Released(resource_managers::Profile* profile, int64_t time);

    KEY(start)
    KEY(select)
    KEY(col11)
    KEY(col12)
    KEY(col13)
    KEY(col14)
    KEY(col15)
    KEY(col16)
    KEY(col17)
    KEY(col1sUp)
    KEY(col1sDown)
    KEY(col21)
    KEY(col22)
    KEY(col23)
    KEY(col24)
    KEY(col25)
    KEY(col26)
    KEY(col27)
    KEY(col2sUp)
    KEY(col2sDown)
#undef KEY
};

} // namespace qml_components

#endif // INPUT_H
