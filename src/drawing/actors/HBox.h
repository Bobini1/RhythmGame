//
// Created by bobini on 05.11.22.
//

#ifndef RHYTHMGAME_HBOX_H
#define RHYTHMGAME_HBOX_H
#include "AbstractBox.h"
namespace drawing::actors {
/**
 * A HBox is a parent that arranges its children horizontally.
 *
 * When width is managed, children with managed width are stretched equally to
 * fill space.
 */
class HBox : public AbstractBox
{
  public:
    static auto make() -> std::shared_ptr<HBox>;
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    [[nodiscard]] auto getMinWidth() const -> float override;
    [[nodiscard]] auto getMinHeight() const -> float override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;
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
    [[nodiscard]] auto getContentAlignment() const -> ContentAlignment;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
    explicit HBox() = default;

  public:
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;

  private:
    sf::Transform transform;
    sf::Vector2f size;
    ContentAlignment contentAlignment{};
    [[nodiscard]] auto getMinimumSizeOfChildren() const -> sf::Vector2f;
    void recalculateSize();
};
} // namespace drawing::actors

#endif // RHYTHMGAME_HBOX_H
