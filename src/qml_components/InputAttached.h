//
// Created by PC on 25/09/2024.
//

#ifndef INPUT_H
#define INPUT_H
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
    input::InputTranslator* inputTranslator;
    QList<QMetaObject::Connection> connections;

    friend class InputAttached;

  public:
    explicit InputSignalProvider(input::InputTranslator* inputTranslator,
                                 QObject* parent = nullptr);
  signals:
    void buttonPressed(input::BmsKey button, double value, int64_t time);
    void buttonReleased(input::BmsKey button, int64_t time);
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

    Q_PROPERTY(input::BmsKey col11 READ col11 NOTIFY col11Changed)
    Q_PROPERTY(input::BmsKey col12 READ col12 NOTIFY col12Changed)
    Q_PROPERTY(input::BmsKey col13 READ col13 NOTIFY col13Changed)
    Q_PROPERTY(input::BmsKey col14 READ col14 NOTIFY col14Changed)
    Q_PROPERTY(input::BmsKey col15 READ col15 NOTIFY col15Changed)
    Q_PROPERTY(input::BmsKey col16 READ col16 NOTIFY col16Changed)
    Q_PROPERTY(input::BmsKey col17 READ col17 NOTIFY col17Changed)
    Q_PROPERTY(input::BmsKey col1sUp READ col1sUp NOTIFY col1sUpChanged)
    Q_PROPERTY(input::BmsKey col1sDown READ col1sDown NOTIFY col1sDownChanged)
    Q_PROPERTY(input::BmsKey col21 READ col21 NOTIFY col21Changed)
    Q_PROPERTY(input::BmsKey col22 READ col22 NOTIFY col22Changed)
    Q_PROPERTY(input::BmsKey col23 READ col23 NOTIFY col23Changed)
    Q_PROPERTY(input::BmsKey col24 READ col24 NOTIFY col24Changed)
    Q_PROPERTY(input::BmsKey col25 READ col25 NOTIFY col25Changed)
    Q_PROPERTY(input::BmsKey col26 READ col26 NOTIFY col26Changed)
    Q_PROPERTY(input::BmsKey col27 READ col27 NOTIFY col27Changed)
    Q_PROPERTY(input::BmsKey col2sUp READ col2sUp NOTIFY col2sUpChanged)
    Q_PROPERTY(input::BmsKey col2sDown READ col2sDown NOTIFY col2sDownChanged)
    Q_PROPERTY(input::BmsKey start1 READ start1 NOTIFY start1Changed)
    Q_PROPERTY(input::BmsKey select1 READ select1 NOTIFY select1Changed)
    Q_PROPERTY(input::BmsKey start2 READ start2 NOTIFY start2Changed)
    Q_PROPERTY(input::BmsKey select2 READ select2 NOTIFY select2Changed)

    auto isAttachedToCurrentScene() const -> bool;

  public:
    explicit InputAttached(QObject* obj = nullptr);

    static InputSignalProvider* inputSignalProvider;
    static std::function<QQuickItem*()>* findCurrentScene;
    static auto qmlAttachedProperties(QObject* object) -> InputAttached*;

    std::array<bool, magic_enum::enum_count<input::BmsKey>()> keyStates{};

  signals:
    void buttonPressed(input::BmsKey button, double value, int64_t time);
    void buttonReleased(input::BmsKey button, int64_t time);

#define KEY(name)                                                              \
    void name##Pressed(double value, int64_t time);                            \
    void name##Released(int64_t time);                                         \
    auto name() const -> input::BmsKey;

    KEY(start1)
    KEY(select1)
    KEY(start2)
    KEY(select2)
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

  signals:
    void col11Changed();
    void col12Changed();
    void col13Changed();
    void col14Changed();
    void col15Changed();
    void col16Changed();
    void col17Changed();
    void col1sUpChanged();
    void col1sDownChanged();
    void col21Changed();
    void col22Changed();
    void col23Changed();
    void col24Changed();
    void col25Changed();
    void col26Changed();
    void col27Changed();
    void col2sUpChanged();
    void col2sDownChanged();
    void start1Changed();
    void select1Changed();
    void start2Changed();
    void select2Changed();
};

} // namespace qml_components

#endif // INPUT_H
