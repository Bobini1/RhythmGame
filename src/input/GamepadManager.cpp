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

    loopTimer.start(1);
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        addController(i);
}

void
GamepadManager::loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_CONTROLLERAXISMOTION) {
            auto axisEvent = event.caxis;
            if (axisEvent.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
                axisEvent.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                if (axisEvent.value == 0) {
                    emit buttonReleased(gamepads[axisEvent.which],
                                        axisEvent.which,
                                        axisEvent.timestamp);
                } else {
                    emit buttonPressed(gamepads[axisEvent.which],
                                       axisEvent.which,
                                       axisEvent.value / 32768.0,
                                       axisEvent.timestamp);
                }
            } else {
                const uint32_t timestamp = axisEvent.timestamp;
                const double value =
                  axisEvent.value /
                  static_cast<double>(-(SDL_JOYSTICK_AXIS_MIN));
                emit axisMoved(
                  gamepads[axisEvent.which], axisEvent.axis, value, timestamp);
            }
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            const SDL_ControllerButtonEvent buttonEvent = event.cbutton;
            emit buttonPressed(gamepads[buttonEvent.which],
                               buttonEvent.button,
                               1.0,
                               buttonEvent.timestamp);
        } else if (event.type == SDL_CONTROLLERBUTTONUP) {
            const SDL_ControllerButtonEvent buttonEvent = event.cbutton;
            emit buttonReleased(gamepads[buttonEvent.which],
                                buttonEvent.button,
                                buttonEvent.timestamp);
        } else if (event.type == SDL_CONTROLLERDEVICEADDED) {
            const SDL_ControllerDeviceEvent deviceEvent = event.cdevice;
            addController(deviceEvent.which);
        } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            const SDL_ControllerDeviceEvent deviceEvent = event.cdevice;
            emit gamepadRemoved(gamepads[deviceEvent.which]);
            controllers.erase(deviceEvent.which);
            gamepads.erase(deviceEvent.which);
        }
    }
}

auto
Gamepad::toVariantMap() const -> QVariantMap
{
    return { { "name", name }, { "guid", guid }, { "index", index } };
}
auto
Gamepad::fromVariantMap(const QVariantMap& map) -> Gamepad
{
    Gamepad gamepad;
    gamepad.name = map["name"].toString();
    gamepad.guid = map["guid"].toString();
    gamepad.index = map["index"].toInt();
    return gamepad;
}
GamepadManager::~
GamepadManager()
{
    loopTimer.stop();
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
}

void
GamepadManager::addController(int index)
{
    char guid[100];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), guid, 100);
    if (!SDL_IsGameController(index)) {
        return;
    }

    auto* controller = SDL_GameControllerOpen(index);
    if (!controller) {
        return;
    }

    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);

    const int instanceID = SDL_JoystickInstanceID(joystick);

    controllers.emplace(
      instanceID,
      std::unique_ptr<SDL_GameController, decltype(&SDL_GameControllerClose)>{
        controller, &SDL_GameControllerClose });

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