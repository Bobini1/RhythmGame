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
Quad::measure(MeasurementSpec widthSpec, MeasurementSpec heightSpec) const
  -> sf::Vector2f
{
    auto size = rect.getSize();
    if (widthSpec.has_value()) {
        size.x = std::min(size.x, widthSpec.value());
    }
    if (heightSpec.has_value()) {
        size.y = std::min(size.y, heightSpec.value());
    }

    return size;
}
void
Quad::setLayout(sf::FloatRect layout)
{
    rect.setPosition(layout.left, layout.top);
    rect.setSize({ layout.width, layout.height });
}
auto
Quad::getLayout() const -> sf::FloatRect
{
    return rect.getGlobalBounds();
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
} // namespace drawing::actors