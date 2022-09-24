//
// Created by bobini on 24.09.22.
//

#include "EventManager.h"
#include <stdexcept>

auto
events::eventNameToEnum(const std::string& event) -> events::Event
{
    if (event == "Init")
        return Event::Init;
    if (event == "Exit")
        return Event::Exit;
    if (event == "Update")
        return Event::Update;
    if (event == "Draw")
        return Event::Draw;
    if (event == "KeyPressed")
        return Event::KeyPressed;
    if (event == "KeyReleased")
        return Event::KeyReleased;
    if (event == "MouseMoved")
        return Event::MouseMoved;
    if (event == "MouseButtonPressed")
        return Event::MouseButtonPressed;
    if (event == "MouseButtonReleased")
        return Event::MouseButtonReleased;
    if (event == "MouseWheelScrolled")
        return Event::MouseWheelScrolled;
    throw std::invalid_argument{ "Unknown event type" };
}
auto
events::enumToEventName(events::Event event) -> std::string
{
    switch (event) {
        case Event::Init:
            return "Init";
        case Event::Exit:
            return "Exit";
        case Event::Update:
            return "Update";
        case Event::Draw:
            return "Draw";
        case Event::KeyPressed:
            return "KeyPressed";
        case Event::KeyReleased:
            return "KeyReleased";
        case Event::MouseMoved:
            return "MouseMoved";
        case Event::MouseButtonPressed:
            return "MouseButtonPressed";
        case Event::MouseButtonReleased:
            return "MouseButtonReleased";
        case Event::MouseWheelScrolled:
            return "MouseWheelScrolled";
    }
    throw std::invalid_argument{ "Unknown event type" };
}
