//
// Created by bobini on 21.04.23.
//

#include "Frame.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <spdlog/spdlog.h>
auto
drawing::actors::Frame::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<drawing::actors::Frame>>{},
             sharedFromBase<Frame>() };
}
void
drawing::actors::Frame::setTransform(sf::Transform newTransform)
{
    this->transform = newTransform;
    newTransform.translate(offset);
    if (child) {
        child->setTransform(newTransform);
    }
}
auto
drawing::actors::Frame::getTransform() const -> sf::Transform
{
    return transform;
}
auto
drawing::actors::Frame::getIsWidthManaged() const -> bool
{
    return isWidthManaged;
}
auto
drawing::actors::Frame::getIsHeightManaged() const -> bool
{
    return isHeightManaged;
}
auto
drawing::actors::Frame::getMinWidth() const -> float
{
    return minSize.x;
}
auto
drawing::actors::Frame::getMinHeight() const -> float
{
    return minSize.y;
}
auto
drawing::actors::Frame::getWidth() const -> float
{
    return size.x;
}
auto
drawing::actors::Frame::getHeight() const -> float
{
    return size.y;
}
void
drawing::actors::Frame::setWidthImpl(float width)
{
    size.x = width;
}
void
drawing::actors::Frame::setHeightImpl(float height)
{
    size.y = height;
}
auto
drawing::actors::Frame::draw(sf::RenderTarget& target,
                             sf::RenderStates states) const -> void
{
    if (child) {
        sf::RenderTexture texture;
        texture.create(static_cast<unsigned int>(size.x),
                       static_cast<unsigned int>(size.y));
        texture.clear(sf::Color::Transparent);
        texture.draw(*child, transform.getInverse());
        texture.display();
        auto sprite = sf::Sprite(texture.getTexture());
        states.transform *= transform;
        target.draw(sprite, states);
    }
}
auto
drawing::actors::Frame::setIsWidthManaged(bool managed) -> void
{
    isWidthManaged = managed;
}
auto
drawing::actors::Frame::setIsHeightManaged(bool managed) -> void
{
    isHeightManaged = managed;
}
auto
drawing::actors::Frame::setOffset(sf::Vector2f newOffset) -> void
{
    this->offset = newOffset;
}
auto
drawing::actors::Frame::getOffset() const -> sf::Vector2f
{
    return offset;
}
void
drawing::actors::Frame::removeChild(std::shared_ptr<Actor> removedChild)
{
    if (child && child == removedChild) {
        this->child = nullptr;
        removedChild->setParent(nullptr);
    }
}
auto
drawing::actors::Frame::setChild(std::shared_ptr<Actor> newChild) -> void
{
    removeChild(child);
    if (newChild) {
        newChild->setParent(sharedFromBase<Frame>());
    }
    child = std::move(newChild);
}
auto
drawing::actors::Frame::make(std::shared_ptr<Actor> child)
  -> std::shared_ptr<Frame>
{
    auto frame = std::shared_ptr<Frame>(new Frame);
    frame->setChild(std::move(child));
    return frame;
}
auto
drawing::actors::Frame::getChild() const -> const std::shared_ptr<Actor>&
{
    return child;
}
auto
drawing::actors::Frame::setMinWidth(float minWidth) -> void
{
    minSize.x = minWidth;
}
auto
drawing::actors::Frame::setMinHeight(float minHeight) -> void
{
    minSize.y = minHeight;
}
auto
drawing::actors::Frame::getAllChildrenAtMousePosition(
  sf::Vector2f position,
  std::set<std::weak_ptr<const Actor>,
           std::owner_less<std::weak_ptr<const Actor>>>& result) const -> void
{
    if (child) {
        child->getAllActorsAtMousePosition(position, result);
    }
}
