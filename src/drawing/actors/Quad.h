//
// Created by bobini on 26.09.22.
//

#ifndef RHYTHMGAME_QUAD_H
#define RHYTHMGAME_QUAD_H

#include <SFML/Graphics/RectangleShape.hpp>
#include "AbstractRectLeaf.h"
namespace drawing::actors {
/**
 * A quad is a simple rectangle that can be drawn to the screen.
 */
class Quad : public AbstractRectLeaf
{
    sf::RectangleShape rect;

  public:
    explicit Quad(sf::Vector2f size = { 0, 0 },
                  sf::Color color = sf::Color::White);
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    [[nodiscard]] auto getPoint(std::size_t index) const -> sf::Vector2f;
    void setFillColor(const sf::Color& color);
    void setOutlineColor(const sf::Color& color);
    void setOutlineThickness(float thickness);
    [[nodiscard]] auto getFillColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineColor() const -> const sf::Color&;
    [[nodiscard]] auto getOutlineThickness() const -> float;
    void update(std::chrono::nanoseconds delta) override;
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;
    sf::Transform transform;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_QUAD_H
