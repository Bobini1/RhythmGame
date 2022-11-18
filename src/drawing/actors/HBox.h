//
// Created by bobini on 05.11.22.
//

#ifndef RHYTHMGAME_HBOX_H
#define RHYTHMGAME_HBOX_H
#include "AbstractBox.h"
namespace drawing::actors {
/**
 * A HBox is a parent that arranges its children horizontally.
 */
class HBox : public AbstractBox
{
  public:
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;
    enum class ContentAlignment
    {
        Top,
        Center,
        Bottom
    };
    auto setContentAlignment(ContentAlignment alignment) -> void;
    auto getContentAlignment() const -> ContentAlignment;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
  public:
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    ContentAlignment contentAlignment{};
    auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize();
};
} // namespace drawing::actors

#endif // RHYTHMGAME_HBOX_H
