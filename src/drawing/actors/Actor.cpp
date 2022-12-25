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
drawing::actors::Actor::getAllChildrenAtMousePosition(
  sf::Vector2f /*position*/,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& /*result*/) const
  -> void
{
}
void
drawing::actors::Actor::getAllActorsAtMousePosition(
  sf::Vector2f position,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& result) const
{
    if (getGlobalBounds().contains(position)) {
        result.insert(weak_from_this());
        getAllChildrenAtMousePosition(position, result);
    }
}