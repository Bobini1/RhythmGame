//
// Created by bobini on 07.12.23.
//

#include "GamepadManager.h"
#include <SDL2/SDL.h>
#include <SDL_joystick.h>
#include <ranges>
#include <spdlog/spdlog.h>

namespace input {

GamepadManager::
GamepadManager(QObject* parent)
  : QObject(parent)
{
    connect(&loopTimer, &QTimer::timeout, this, &GamepadManager::loop);

    if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK)) {
        throw std::runtime_error(SDL_GetError());
    }
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch());
    startTime = (now - std::chrono::milliseconds{ SDL_GetTicks64() }).count();

    loopTimer.start(1);
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        addController(i);
}

void
GamepadManager::loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_JOYAXISMOTION) {
            const auto axisEvent = event.jaxis;
            const double value =
              axisEvent.value / static_cast<double>(-(SDL_JOYSTICK_AXIS_MIN));
            emit axisMoved(gamepads[axisEvent.which],
                           axisEvent.axis,
                           value,
                           axisEvent.timestamp + startTime);
        } else if (event.type == SDL_JOYBUTTONDOWN) {
            const auto buttonEvent = event.jbutton;
            emit buttonPressed(gamepads[buttonEvent.which],
                               buttonEvent.button,
                               buttonEvent.timestamp + startTime);
        } else if (event.type == SDL_JOYBUTTONUP) {
            const auto buttonEvent = event.jbutton;
            emit buttonReleased(gamepads[buttonEvent.which],
                                buttonEvent.button,
                                buttonEvent.timestamp + startTime);
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            const auto deviceEvent = event.jdevice;
            addController(deviceEvent.which);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            const auto deviceEvent = event.jdevice;
            emit gamepadRemoved(gamepads[deviceEvent.which]);
            controllers.erase(deviceEvent.which);
            gamepads.erase(deviceEvent.which);
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

GamepadManager::~
GamepadManager()
{
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

    auto gamepad = Gamepad{ name, guid, index };
    for (const auto& [name, guid, index] :
         std::ranges::views::values(gamepads)) {
        if (gamepad.guid == guid) {
            gamepad.index++;
        }
    }
    gamepads[instanceID] = gamepad;
    emit gamepadAdded(gamepad);
}

} // namespace input