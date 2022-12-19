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
void
drawing::actors::Actor::setEventSubscription(
  const std::string& eventName,
  std::unique_ptr<events::Connection> connection)
{
    eventSubscriptions[eventName] = std::move(connection);
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
drawing::actors::Actor::setOnMouse1Down(std::function<void()> callback) -> void
{
    onMouse1Down = std::move(callback);
}
auto
drawing::actors::Actor::setOnMouseEnter(std::function<void()> callback) -> void
{
    onMouseEnter = std::move(callback);
}
auto
drawing::actors::Actor::setOnMouseLeave(std::function<void()> callback) -> void
{
    onMouseLeave = std::move(callback);
}
auto
drawing::actors::Actor::getOnMouse1Down() const -> const std::function<void()>&
{
    return onMouse1Down;
}
auto
drawing::actors::Actor::getOnMouseEnter() const -> const std::function<void()>&
{
    return onMouseEnter;
}
auto
drawing::actors::Actor::getOnMouseLeave() const -> const std::function<void()>&
{
    return onMouseLeave;
}
