//
// Created by bobini on 16.11.22.
//

#include "Align.h"
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
sol::object
drawing::actors::Align::getLuaSelf(sol::state& lua)
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<drawing::actors::Align>>{},
             sharedFromBase<Align>() };
}
void
drawing::actors::Align::update(std::chrono::nanoseconds delta)
{
}
void
drawing::actors::Align::setTransform(sf::Transform newTransform)
{
    transform = newTransform;
    if (!child) {
        return;
    }
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
sf::Transform
drawing::actors::Align::getTransform() const
{
    return transform;
}
bool
drawing::actors::Align::matchParentWidth() const
{
    switch (mode) {
        case Mode::TopLeft:
        case Mode::Left:
        case Mode::BottomLeft:
            return false;
        default:
            return true;
    }
}
bool
drawing::actors::Align::matchParentHeight() const
{
    switch (mode) {
        case Mode::TopLeft:
        case Mode::Top:
        case Mode::TopRight:
            return false;
        default:
            return true;
    }
}
float
drawing::actors::Align::getMinWidth() const
{
    if (!child) {
        return 0;
    }
    return child->getMinWidth();
}
float
drawing::actors::Align::getMinHeight() const
{
    if (!child) {
        return 0;
    }
    return child->getMinHeight();
}
float
drawing::actors::Align::getWidth() const
{
    if (!child) {
        return 0;
    }
    if (child->matchParentWidth()) {
        return child->getWidth();
    }
    return std::max(size.x, child->getWidth());
}
float
drawing::actors::Align::getHeight() const
{
    if (!child) {
        return 0;
    }
    if (child->matchParentHeight()) {
        return child->getHeight();
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
    if (child == nullptr) {
        return;
    }
    if (child->matchParentWidth()) {
        child->setWidth(width);
    } else {
        child->setWidth(std::min(width, child->getWidth()));
    }
}
void
drawing::actors::Align::setHeightImpl(float height)
{
    size.y = height;
    if (child == nullptr) {
        return;
    }
    if (child->matchParentHeight()) {
        child->setHeight(height);
    } else {
        child->setHeight(std::min(height, child->getHeight()));
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
    this->child = newChild;
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