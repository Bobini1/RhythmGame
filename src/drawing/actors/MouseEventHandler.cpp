//
// Created by PC on 18/12/2022.
//

#include "MouseEventHandler.h"
auto
drawing::actors::MouseEventHandler::handleEvent(
  drawing::actors::EventType eventType) -> bool
{
    if (auto eventHandler = eventHandlers.find(eventType);
        eventHandler != eventHandlers.end() && eventHandler->second) {
        eventHandler->second();
        return true;
    }
    return false;
}
auto
drawing::actors::MouseEventHandler::setEvent(
  drawing::actors::EventType eventType,
  std::function<void()> eventHandler) -> void
{
    if (!eventHandler) {
        // remove event
        eventHandlers.erase(eventType);
    }
    eventHandlers[eventType] = std::move(eventHandler);
}
