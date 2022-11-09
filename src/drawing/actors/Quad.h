//
// Created by bobini on 26.09.22.
//

#ifndef RHYTHMGAME_QUAD_H
#define RHYTHMGAME_QUAD_H

#include <SFML/Graphics/RectangleShape.hpp>
#include "Actor.h"
namespace drawing::actors {
class Quad : public Actor
{
    sf::RectangleShape rect;

  public:
    void setSize(const sf::Vector2f& size);
    [[nodiscard]] auto getSize() const -> const sf::Vector2f&;
    [[nodiscard]] auto getPoint(std::size_t index) const -> sf::Vector2f;
    void setFillColor(const sf::Color& color);
    void setOutlineColor(const sf::Color& color);
    void setOutlineThickness(float thickness);
    [[nodiscard]] auto getFillColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineThickness() const -> float;
    void update(std::chrono::nanoseconds delta) override;
    [[nodiscard]] auto matchParentWidth() const -> bool override;
    [[nodiscard]] auto matchParentHeight() const -> bool override;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
    sf::Transform transform;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_QUAD_H
