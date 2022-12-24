//
// Created by bobini on 30.07.2022.
//

#include "Parent.h"
#include "Actor.h"

#include <utility>

auto
drawing::actors::Actor::getParent() const -> std::shared_ptr<Parent>
{
    return parent.lock();
}
auto
drawing::actors::Actor::setParent(const std::shared_ptr<Parent>& newParent)
  -> void
{
    // defensive programming
    if (auto parentPtr = parent.lock()) {
        this->parent = newParent;
        parentPtr->removeChild(shared_from_this());
    } else {
        this->parent = newParent;
    }
}
auto
drawing::actors::Actor::setWidth(float width) -> void
{
    if (width < getMinWidth()) {
        width = getMinWidth();
    }
    setWidthImpl(width);
}
auto
drawing::actors::Actor::setHeight(float height) -> void
{
    if (height < getMinHeight()) {
        height = getMinHeight();
    }
    setHeightImpl(height);
}

drawing::actors::Actor::Actor(const Actor& /*unused*/) {} // do not default this
auto
drawing::actors::Actor::operator=(const Actor& /*unused*/)
  -> Actor& // do not default this
{
    return *this;
}
auto
drawing::actors::Actor::getGlobalBounds() const -> sf::FloatRect
{
    return getTransform().transformRect({ 0, 0, getWidth(), getHeight() });
}
auto
drawing::actors::Actor::getIsObstructing() const -> bool
{
    return isObstructing;
}
auto
drawing::actors::Actor::setIsObstructing(bool newIsObstructing) -> void
{
    isObstructing = newIsObstructing;
}
auto
drawing::actors::Actor::handleEvent(sf::Vector2f position,
                                    drawing::actors::EventType eventType)
  -> bool
{
    if (auto eventHandler = eventHandlers.find(eventType);
        eventHandler != eventHandlers.end() && eventHandler->second) {
        eventHandler->second(*this, position);
        return true;
    }
    return false;
}
auto
drawing::actors::Actor::setEvent(
  drawing::actors::EventType eventType,
  std::function<void(drawing::actors::Actor&, sf::Vector2f)> eventHandler)
  -> void
{
    if (!eventHandler) {
        // remove event
        eventHandlers.erase(eventType);
    }
    eventHandlers[eventType] = std::move(eventHandler);
}
auto
drawing::actors::Actor::getEvent(drawing::actors::EventType eventType) const
  -> std::function<void(drawing::actors::Actor&, sf::Vector2f)>
{
    if (auto eventHandler = eventHandlers.find(eventType);
        eventHandler != eventHandlers.end()) {
        return eventHandler->second;
    }
    return nullptr;
}
auto
drawing::actors::Actor::getAllChildrenAtMousePosition(
  sf::Vector2f /*position*/,
  std::set<Actor*>& /*result*/) -> void
{
}
void
drawing::actors::Actor::getAllActorsAtMousePosition(sf::Vector2f position,
                                                    std::set<Actor*>& result)
{
    if (getGlobalBounds().contains(position)) {
        result.insert(this);
        getAllChildrenAtMousePosition(position, result);
    }
}
