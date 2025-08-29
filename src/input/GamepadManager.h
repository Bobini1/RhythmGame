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
    /**
     * @brief The index of the gamepad. It will get incremented if two gamepads
     * with the same guid are connected to the computer.
     * Connect them in the same order to get the same index.
     */
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

    auto operator==(const Gamepad& gamepad) const -> bool;

    friend auto operator>>(QDataStream& stream, Gamepad& gamepad)
      -> QDataStream&;
    friend auto operator<<(QDataStream& stream, const Gamepad& gamepad)
      -> QDataStream&;
};
} // namespace input

template<>
struct std::hash<input::Gamepad>
{
    auto operator()(const input::Gamepad& s) const noexcept -> std::size_t;
};
namespace input {
class GamepadManager final : public QObject
{
    Q_OBJECT

    QTimer loopTimer;
    std::unordered_map<
      SDL_JoystickID,
      std::unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)>>
      controllers;
    std::unordered_map<SDL_JoystickID, Gamepad> gamepads;
    uint64_t startTime;

    void addController(int index);
    void loop();

  public:
    ~GamepadManager() override;
    explicit GamepadManager(QObject* parent = nullptr);

  signals:

    void axisMoved(Gamepad gamepad, Uint8 axis, double value, uint64_t time);
    void buttonPressed(Gamepad gamepad, Uint8 uint8, uint64_t time);
    void buttonReleased(Gamepad gamepad, Uint8 uint8, uint64_t time);
    void gamepadRemoved(Gamepad gamepad);
    void gamepadAdded(Gamepad gamepad);
};

} // namespace input

#endif // GAMEPADMANAGER_H
