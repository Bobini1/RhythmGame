//
// Created by bobini on 26.09.22.
//

#include "Quad.h"
#include <SFML/Graphics/RenderTarget.hpp>

namespace drawing::actors {
void
Quad::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(rect, states);
}
void
Quad::update(std::chrono::nanoseconds delta)
{
}
void
Quad::setSize(const sf::Vector2f& size)
{
    rect.setSize(size);
}
auto
Quad::getSize() const -> const sf::Vector2f&
{
    return rect.getSize();
}
auto
Quad::getPoint(std::size_t index) const -> sf::Vector2f
{
    return rect.getPoint(index);
}
void
Quad::setFillColor(const sf::Color& color)
{
    rect.setFillColor(color);
}
void
Quad::setOutlineColor(const sf::Color& color)
{
    rect.setOutlineColor(color);
}
void
Quad::setOutlineThickness(float thickness)
{
    rect.setOutlineThickness(thickness);
}
auto
Quad::getFillColor() const -> const sf::Color&
{
    return rect.getFillColor();
}
auto
Quad::getOutlineColor() const -> const sf::Color&
{
    return rect.getOutlineColor();
}
auto
Quad::getOutlineThickness() const -> float
{
    return rect.getOutlineThickness();
}
auto
Quad::getLuaSelf(sol::state& lua) -> sol::object
{
    return { lua,
             sol::in_place_type_t<std::shared_ptr<Quad>>(),
             sharedFromBase<Quad>() };
}
auto
Quad::matchParentWidth() const -> bool
{
    return false;
}
auto
Quad::matchParentHeight() const -> bool
{
    return false;
}
void
Quad::setTransform(sf::Transform newTransform)
{
    this->transform = newTransform;
}
auto
Quad::getTransform() const -> sf::Transform
{
    return transform;
}
auto
Quad::getMinWidth() const -> float
{
    return 0;
}
auto
Quad::getMinHeight() const -> float
{
    return 0;
}
auto
Quad::getWidth() const -> float
{
    return rect.getSize().x;
}
auto
Quad::getHeight() const -> float
{
    return rect.getSize().y;
}
void
Quad::setWidthImpl(float width)
{
    rect.setSize({ width, rect.getSize().y });
}
void
Quad::setHeightImpl(float height)
{
    rect.setSize({ rect.getSize().x, height });
}
} // namespace drawing::actors