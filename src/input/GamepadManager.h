//
// Created by bobini on 07.12.23.
//

#ifndef GAMEPADMANAGER_H
#define GAMEPADMANAGER_H
#include <QObject>
#include <QTimer>
#include <SDL_gamecontroller.h>
#include <QVariantMap>
#include <functional>

namespace input {
class Gamepad
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString guid MEMBER guid)
    Q_PROPERTY(int index MEMBER index)

  public:
    QString name;
    QString guid;
    int index{};

    auto operator<=>(const Gamepad& gamepad) const
    {
        return std::tie(name, guid, index) <=>
               std::tie(gamepad.name, gamepad.guid, gamepad.index);
    }

    auto operator==(const Gamepad& gamepad) const
    {
        return std::tie(name, guid, index) ==
               std::tie(gamepad.name, gamepad.guid, gamepad.index);
    }

    auto toVariantMap() const -> QVariantMap;

    static auto fromVariantMap(const QVariantMap& map) -> Gamepad;
};
} // namespace input

template<>
struct std::hash<input::Gamepad>
{
    std::size_t operator()(const input::Gamepad& s) const noexcept
    {
        return std::hash<QString>{}(s.name) ^ std::hash<QString>{}(s.guid) ^
               std::hash<int>{}(s.index);
    }
};
namespace input {
class GamepadManager : public QObject
{
    Q_OBJECT

    QTimer loopTimer;
    std::unordered_map<
      SDL_JoystickID,
      std::unique_ptr<SDL_GameController, decltype(&SDL_GameControllerClose)>>
      controllers;
    std::unordered_map<SDL_JoystickID, Gamepad> gamepads;

    void addController(int index);
    void loop();

  public:
    ~GamepadManager() override;
    GamepadManager(QObject* parent = nullptr);

  signals:

    void axisMoved(Gamepad gamepad, Uint8 axis, double value, uint32_t time);
    void buttonPressed(Gamepad gamepad, Uint8 uint8, double x, Uint32 time);
    void buttonReleased(Gamepad gamepad, Uint8 uint8, Uint32 time);
    void gamepadRemoved(Gamepad gamepad);
    void gamepadAdded(Gamepad gamepad);
};

} // input

#endif // GAMEPADMANAGER_H
