//
// Created by bobini on 16.11.22.
//

#include "Align.h"

#include <utility>
#include "SFML/Graphics/RenderTarget.hpp"
void
drawing::actors::Align::removeChild(std::shared_ptr<Actor> /* child */)
{
    if (!child) {
        return;
    }
    this->child->setParent(nullptr);
    this->child = nullptr;
}
auto
drawing::actors::Align::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<drawing::actors::Align>>{},
             sharedFromBase<Align>() };
}
void
drawing::actors::Align::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    if (!child) {
        return;
    }
    setWidth(getWidth());
    setHeight(getHeight());
    switch (mode) {
        case Mode::TopLeft:
            child->setTransform(newTransform);
            break;
        case Mode::Top:
            child->setTransform(
              newTransform.translate((size.x - child->getWidth()) / 2, 0));
            break;
        case Mode::TopRight:
            child->setTransform(
              newTransform.translate(size.x - child->getWidth(), 0));
            break;
        case Mode::Left:
            child->setTransform(
              newTransform.translate(0, (size.y - child->getHeight()) / 2));
            break;
        case Mode::Center:
            child->setTransform(
              newTransform.translate((size.x - child->getWidth()) / 2,
                                     (size.y - child->getHeight()) / 2));
            break;
        case Mode::Right:
            child->setTransform(newTransform.translate(
              size.x - child->getWidth(), (size.y - child->getHeight()) / 2));
            break;
        case Mode::BottomLeft:
            child->setTransform(
              newTransform.translate(0, size.y - child->getHeight()));
            break;
        case Mode::Bottom:
            child->setTransform(newTransform.translate(
              (size.x - child->getWidth()) / 2, size.y - child->getHeight()));
            break;
        case Mode::BottomRight:
            child->setTransform(newTransform.translate(
              size.x - child->getWidth(), size.y - child->getHeight()));
            break;
    }
}
auto
drawing::actors::Align::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::Align::getIsWidthManaged() const -> bool
{
    return true;
}
auto
drawing::actors::Align::getIsHeightManaged() const -> bool
{
    return true;
}
auto
drawing::actors::Align::getMinWidth() const -> float
{
    if (!child) {
        return 0;
    }
    if (child->getIsWidthManaged()) {
        return child->getMinWidth();
    }
    return child->getWidth();
}
auto
drawing::actors::Align::getMinHeight() const -> float
{
    if (!child) {
        return 0;
    }
    if (child->getIsHeightManaged()) {
        return child->getMinHeight();
    }
    return child->getHeight();
}
auto
drawing::actors::Align::getWidth() const -> float
{
    if (!child) {
        return size.x;
    }
    return std::max(size.x, child->getWidth());
}
auto
drawing::actors::Align::getHeight() const -> float
{
    if (!child) {
        return size.y;
    }
    return std::max(size.y, child->getHeight());
}
void
drawing::actors::Align::draw(sf::RenderTarget& target,
                             sf::RenderStates states) const
{
    if (child) {
        target.draw(*child, states);
    }
}
void
drawing::actors::Align::setWidthImpl(float width)
{
    size.x = width;
    if (child && child->getIsWidthManaged()) {
        child->setWidth(width);
    }
}
void
drawing::actors::Align::setHeightImpl(float height)
{
    size.y = height;
    if (child && child->getIsHeightManaged()) {
        child->setHeight(height);
    }
}
auto
drawing::actors::Align::setMode(Mode newMode) -> void
{
    mode = newMode;
}
auto
drawing::actors::Align::getMode() const -> Mode
{
    return mode;
}
void
drawing::actors::Align::setChild(std::shared_ptr<Actor> newChild)
{
    if (this->child) {
        this->child->setParent(nullptr);
    }
    this->child = std::move(newChild);
    if (child) {
        child->setParent(sharedFromBase<Parent>());
    }
}
auto
drawing::actors::Align::getChild() const -> std::shared_ptr<Actor>
{
    return child;
}
drawing::actors::Align::Align(drawing::actors::Align::Mode mode)
  : mode(mode)
{
}
auto
drawing::actors::Align::make(drawing::actors::Align::Mode mode)
  -> std::shared_ptr<drawing::actors::Align>
{
    return std::shared_ptr<Align>(new Align{ mode });
}
auto
drawing::actors::Align::getAllChildrenAtMousePosition(
  sf::Vector2f position,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& result) const -> bool
{
    if (child) {
        return child->getAllActorsAtMousePosition(position, result);
    }
    return false;
}
