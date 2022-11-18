//
// Created by bobini on 16.11.22.
//

#ifndef RHYTHMGAME_ALIGN_H
#define RHYTHMGAME_ALIGN_H
#include "Parent.h"
namespace drawing::actors {
/**
 * @brief Align fills its parent's space and aligns its children according to
 * the specified mode (center by default).
 */
class Align : public Parent
{
    std::shared_ptr<Actor> child;
    sf::Transform transform;
    sf::Vector2f size;

  public:
    enum class Mode
    {
        TopLeft,
        Top,
        TopRight,
        Left,
        Center,
        Right,
        BottomLeft,
        Bottom,
        BottomRight
    };
    explicit Align(Mode mode = Mode::Center);
    void setChild(std::shared_ptr<Actor> child);
    auto getChild() const -> std::shared_ptr<Actor>;
    auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto setTransform(sf::Transform transform) -> void override;
    auto getTransform() const -> sf::Transform override;
    auto getIsWidthManaged() const -> bool override;
    auto getIsHeightManaged() const -> bool override;
    auto getMinWidth() const -> float override;
    auto getMinHeight() const -> float override;
    auto getWidth() const -> float override;
    auto getHeight() const -> float override;
    auto setMode(Mode mode) -> void;
    auto getMode() const -> Mode;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  public:
    auto removeChild(std::shared_ptr<Actor> child) -> void override;

  private:
    Mode mode;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_ALIGN_H
