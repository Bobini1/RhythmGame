//
// Created by bobini on 07.12.23.
//

#include "GamepadManager.h"
#include <SDL2/SDL.h>
#include <SDL_joystick.h>
#include <ranges>
#include <spdlog/spdlog.h>

namespace input {

GamepadManager::GamepadManager(QObject* parent)
  : QObject(parent)
{
    connect(&loopTimer, &QTimer::timeout, this, &GamepadManager::loop);

    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK)) {
        throw std::runtime_error(SDL_GetError());
    }

    loopTimer.start(0);
}

void
GamepadManager::loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch());
        auto nowMillis = now.count();
        auto startTime = (now - std::chrono::milliseconds{ SDL_GetTicks64() }).count();
        if (event.type == SDL_JOYAXISMOTION) {
            const auto axisEvent = event.jaxis;
            const double value =
              axisEvent.value / static_cast<double>(-(SDL_JOYSTICK_AXIS_MIN));
            emit axisMoved(gamepads[axisEvent.which],
                           axisEvent.axis,
                           value,
                           nowMillis);
        } else if (event.type == SDL_JOYBUTTONDOWN) {
            const auto buttonEvent = event.jbutton;
            emit buttonPressed(gamepads[buttonEvent.which],
                               buttonEvent.button,
                               nowMillis);
        } else if (event.type == SDL_JOYBUTTONUP) {
            const auto buttonEvent = event.jbutton;
            emit buttonReleased(gamepads[buttonEvent.which],
                                buttonEvent.button,
                                nowMillis);
        } else if (event.type == SDL_JOYDEVICEADDED) {
            const auto deviceEvent = event.jdevice;
            addController(deviceEvent.which);
        } else if (event.type == SDL_JOYDEVICEREMOVED) {
            const auto deviceEvent = event.jdevice;
            emit gamepadRemoved(gamepads[deviceEvent.which]);
            controllers.erase(deviceEvent.which);
            gamepads.erase(deviceEvent.which);
        }
    }
}

GamepadManager::~GamepadManager()
{
    controllers.clear();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
}

void
GamepadManager::addController(int index)
{
    char guid[100];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, 100);

    auto* joystick = SDL_JoystickOpen(index);
    if (!joystick) {
        return;
    }

    const int instanceID = SDL_JoystickInstanceID(joystick);

    controllers.emplace(
      instanceID,
      std::unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)>{
        joystick, &SDL_JoystickClose });

    const char* name = SDL_JoystickName(joystick);

    auto gamepad = Gamepad{ name, guid, 0 };
    for (const auto& [name, guid, index] :
         std::ranges::views::values(gamepads)) {
        if (gamepad.guid == guid) {
            gamepad.index++;
        }
    }
    gamepads[instanceID] = gamepad;
    emit gamepadAdded(gamepad);
}

auto
Gamepad::operator==(const Gamepad& gamepad) const-> bool
{
    return std::tie(name, guid, index) ==
           std::tie(gamepad.name, gamepad.guid, gamepad.index);
}
auto
operator>>(QDataStream& stream, Gamepad& gamepad) -> QDataStream&
{
    return stream >> gamepad.name >> gamepad.guid >> gamepad.index;
}
auto
operator<<(QDataStream& stream, const Gamepad& gamepad) -> QDataStream&
{
    return stream << gamepad.name << gamepad.guid << gamepad.index;
}
} // namespace input
auto
std::hash<input::Gamepad>::operator()(const input::Gamepad& s) const noexcept
  -> std::size_t
{
    return std::hash<QString>{}(s.name) ^ std::hash<QString>{}(s.guid) ^
           std::hash<int>{}(s.index);
}