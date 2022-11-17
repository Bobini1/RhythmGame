//
// Created by bobini on 11.11.22.
//

#include "Sprite.h"
//
// Created by bobini on 26.09.22.
//

#include "Sprite.h"
#include <SFML/Graphics/RenderTarget.hpp>

namespace drawing::actors {
void
Sprite::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    states.transform *= getTransform();
    target.draw(sprite, states);
}
void
Sprite::update(std::chrono::nanoseconds delta)
{
}
auto
Sprite::matchParentWidth() const -> bool
{
    return false;
}
auto
Sprite::matchParentHeight() const -> bool
{
    return false;
}
void
Sprite::setTransform(sf::Transform newTransform)
{
    this->transform = newTransform;
}
auto
Sprite::getTransform() const -> sf::Transform
{
    return transform;
}
auto
Sprite::getMinWidth() const -> float
{
    return 0;
}
auto
Sprite::getMinHeight() const -> float
{
    return 0;
}
auto
Sprite::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<Sprite>>(),
             sharedFromBase<Sprite>() };
}
void
Sprite::setTexture(const sf::Texture& texture)
{
    sprite.setTexture(texture);
}
void
Sprite::setColor(const sf::Color& color)
{
    sprite.setColor(color);
}
void
Sprite::setTextureRect(const sf::IntRect& rect)
{
    sprite.setTextureRect(rect);
}
auto
Sprite::getTexture() const -> const sf::Texture*
{
    return sprite.getTexture();
}
auto
Sprite::getTextureRect() const -> const sf::IntRect&
{
    return sprite.getTextureRect();
}
auto
Sprite::getColor() const -> const sf::Color&
{
    return sprite.getColor();
}
auto
Sprite::getWidth() const -> float
{
    return sprite.getGlobalBounds().width + sprite.getGlobalBounds().left;
}
auto
Sprite::getHeight() const -> float
{
    return sprite.getGlobalBounds().height + sprite.getGlobalBounds().top;
}
auto
Sprite::setWidthImpl(float width) -> void
{
    sprite.setScale(width / sprite.getGlobalBounds().width,
                    sprite.getScale().y);
}
auto
Sprite::setHeightImpl(float height) -> void
{
    sprite.setScale(sprite.getScale().x,
                    height / sprite.getGlobalBounds().height);
}
Sprite::Sprite(const sf::Texture& texture)
{
    sprite.setTexture(texture);
}
} // namespace drawing::actors