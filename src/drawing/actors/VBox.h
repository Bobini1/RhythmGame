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
 *
 * When height is managed, children with managed height are stretched equally to
 * fill space.
 */
class VBox : public AbstractBox
{
  public:
    static auto make() -> std::shared_ptr<VBox>;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    [[nodiscard]] auto getMinWidth() const -> float override;
    [[nodiscard]] auto getMinHeight() const -> float override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;
    enum class ContentAlignment
    {
        Left,
        Center,
        Right
    };
    auto setContentAlignment(ContentAlignment alignment) -> void;
    [[nodiscard]] auto getContentAlignment() const -> ContentAlignment;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  public:
    auto setTransform(sf::Transform newTransform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    VBox() = default;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    ContentAlignment contentAlignment{};
    [[nodiscard]] auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize();
};
} // namespace drawing::actors
#endif // RHYTHMGAME_VBOX_H
