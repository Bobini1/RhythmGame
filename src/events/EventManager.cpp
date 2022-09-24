//
// Created by bobini on 24.09.22.
//

#include "EventManager.h"
auto
events::eventNameToEnum(const std::string& event) -> events::Event
{
    if (event == "init")
        return Event::Init;
    if (event == "exit")
        return Event::Exit;
    if (event == "update")
        return Event::Update;
    if (event == "draw")
        return Event::Draw;
    if (event == "keyPressed")
        return Event::KeyPressed;
    if (event == "keyReleased")
        return Event::KeyReleased;
    if (event == "mouseMoved")
        return Event::MouseMoved;
    if (event == "mouseButtonPressed")
        return Event::MouseButtonPressed;
    if (event == "mouseButtonReleased")
        return Event::MouseButtonReleased;
    if (event == "mouseWheelScrolled")
        return Event::MouseWheelScrolled;
    throw std::invalid_argument{ "Unknown event type" };
}
auto
events::enumToEventName(events::Event event) -> std::string
{
    switch (event) {
        case Event::Init:
            return "init";
        case Event::Exit:
            return "exit";
        case Event::Update:
            return "update";
        case Event::Draw:
            return "draw";
        case Event::KeyPressed:
            return "keyPressed";
        case Event::KeyReleased:
            return "keyReleased";
        case Event::MouseMoved:
            return "mouseMoved";
        case Event::MouseButtonPressed:
            return "mouseButtonPressed";
        case Event::MouseButtonReleased:
            return "mouseButtonReleased";
        case Event::MouseWheelScrolled:
            return "mouseWheelScrolled";
    }
    throw std::invalid_argument{ "Unknown event type" };
}
