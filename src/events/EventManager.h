//
// Created by bobini on 24.09.22.
//

#ifndef RHYTHMGAME_EVENTMANAGER_H
#define RHYTHMGAME_EVENTMANAGER_H
#include <string>
#include <span>
#include <any>
#include <memory>
#include "Listener.h"
namespace events {

enum class Event
{
    Init,
    Exit,
    Update,
    Draw,
    KeyPressed,
    KeyReleased,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseWheelScrolled
};

template<typename T>
concept EventManager = requires(T eventManager)
{
    { eventManager.connect(Event::Init, std::weak_ptr<Listener>()) };
    { eventManager.call(Event::Init, std::span<std::any>()) };
    { eventManager.disconnect(Event::Init, std::weak_ptr<Listener>()) };
};


auto
eventNameToEnum(const std::string& event) -> Event;

auto
enumToEventName(Event event) -> std::string;

} // namespace events
#endif // RHYTHMGAME_EVENTMANAGER_H
