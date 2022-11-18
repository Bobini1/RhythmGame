//
// Created by bobini on 16.11.22.
//

#include "Padding.h"
#include "SFML/Graphics/RenderTarget.hpp"
drawing::actors::Padding::Padding(float top,
                                  float bottom,
                                  float left,
                                  float right)
  : top{ top }
  , bottom{ bottom }
  , left{ left }
  , right{ right }
{
}
auto
drawing::actors::Padding::getTop() const -> float
{
    return top;
}
auto
drawing::actors::Padding::getBottom() const -> float
{
    return bottom;
}
auto
drawing::actors::Padding::getLeft() const -> float
{
    return left;
}
auto
drawing::actors::Padding::getRight() const -> float
{
    return right;
}
auto
drawing::actors::Padding::setTop(float newTop) -> void
{
    top = newTop;
}
auto
drawing::actors::Padding::setBottom(float newBottom) -> void
{
    bottom = newBottom;
}
auto
drawing::actors::Padding::setLeft(float newLeft) -> void
{
    left = newLeft;
}
auto
drawing::actors::Padding::setRight(float newRight) -> void
{
    right = newRight;
}
auto
drawing::actors::Padding::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<drawing::actors::Padding>>{},
             sharedFromBase<Padding>() };
}
void
drawing::actors::Padding::update(std::chrono::nanoseconds delta)
{
}
void
drawing::actors::Padding::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    newTransform.translate(left, top);
    if (child) {
        child->setTransform(newTransform);
    }
}
auto
drawing::actors::Padding::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::Padding::getIsWidthManaged() const -> bool
{
    return child && child->getIsWidthManaged();
}
auto
drawing::actors::Padding::getIsHeightManaged() const -> bool
{
    return child && child->getIsHeightManaged();
}
auto
drawing::actors::Padding::getMinWidth() const -> float
{
    if (child) {
        return child->getMinWidth() + left + right;
    }
    return left + right;
}
auto
drawing::actors::Padding::getMinHeight() const -> float
{
    if (child) {
        return child->getMinHeight() + top + bottom;
    }
    return top + bottom;
}
auto
drawing::actors::Padding::getWidth() const -> float
{
    if (child) {
        return child->getWidth() + left + right;
    }
    return left + right;
}
auto
drawing::actors::Padding::getHeight() const -> float
{
    if (child) {
        return child->getHeight() + top + bottom;
    }
    return top + bottom;
}
void
drawing::actors::Padding::removeChild(std::shared_ptr<Actor> /* child */)
{
    if (!child) {
        return;
    }
    this->child->setParent(nullptr);
    this->child = nullptr;
}
void
drawing::actors::Padding::draw(sf::RenderTarget& target,
                               sf::RenderStates states) const
{
    if (child) {
        target.draw(*child, states);
    }
}
void
drawing::actors::Padding::setWidthImpl(float width)
{
    if (child) {
        child->setWidth(width - left - right);
    }
}
void
drawing::actors::Padding::setHeightImpl(float height)
{
    if (child) {
        child->setHeight(height - top - bottom);
    }
}
auto
drawing::actors::Padding::setChild(std::shared_ptr<Actor> newChild) -> void
{
    removeChild(child);
    child = std::move(newChild);
    if (child) {
        child->setParent(sharedFromBase<Parent>());
    }
}
auto
drawing::actors::Padding::getChild() const -> std::shared_ptr<Actor>
{
    return child;
}
