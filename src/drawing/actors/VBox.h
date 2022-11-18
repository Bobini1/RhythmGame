//
// Created by bobini on 13.10.22.
//

#ifndef RHYTHMGAME_VBOX_H
#define RHYTHMGAME_VBOX_H
#include "Parent.h"
#include "AbstractBox.h"
#include <SFML/Graphics/Texture.hpp>
namespace drawing::actors {
/**
 * A VBox is a parent that arranges its children vertically.
 */
class VBox : public AbstractBox
{
  public:
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    enum class ContentAlignment
    {
        Left,
        Center,
        Right
    };
    auto setContentAlignment(ContentAlignment alignment) -> void;
    auto getContentAlignment() const -> ContentAlignment;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  public:
    auto setTransform(sf::Transform newTransform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    ContentAlignment contentAlignment{};
    auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize();
};
} // namespace drawing::actors
#endif // RHYTHMGAME_VBOX_H
