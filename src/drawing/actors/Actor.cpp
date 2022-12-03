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
drawing::actors::Actor::update(std::chrono::nanoseconds delta) -> void
{
}