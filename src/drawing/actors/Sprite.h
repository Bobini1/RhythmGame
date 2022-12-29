//
// Created by bobini on 11.11.22.
//

#ifndef RHYTHMGAME_SPRITE_H
#define RHYTHMGAME_SPRITE_H

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include "AbstractRectLeaf.h"
namespace drawing::actors {
/**
 * @brief A sprite is a leaf actor that displays a texture.
 */
class Sprite : public AbstractRectLeaf
{
    sf::Sprite sprite;
    sf::Transform transform;

  public:
    static auto make(const sf::Texture& texture) -> std::shared_ptr<Sprite>;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    void setTexture(const sf::Texture& texture);
    void setTextureRect(const sf::IntRect& rect);
    void setColor(const sf::Color& color);
    [[nodiscard]] auto getTexture() const -> const sf::Texture*;
    [[nodiscard]] auto getTextureRect() const -> const sf::IntRect&;
    [[nodiscard]] auto getColor() const -> const sf::Color&;
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    explicit Sprite(const sf::Texture& texture);
};
} // namespace drawing::actors
#endif // RHYTHMGAME_SPRITE_H
