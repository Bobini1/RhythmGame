//
// Created by bobini on 16.11.22.
//

#ifndef RHYTHMGAME_PADDING_H
#define RHYTHMGAME_PADDING_H

#include "Parent.h"
namespace drawing::actors {
/**
 * @brief A parent that adds padding around its child.
 */
class Padding : public Parent
{
    float top{};
    float bottom{};
    float left{};
    float right{};
    std::shared_ptr<Actor> child;
    sf::Transform transform;

  public:
    [[nodiscard]] auto getLuaSelf(sol::state& lua) -> sol::object override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto setTransform(sf::Transform transform) -> void override;
    [[nodiscard]] auto getTransform() const -> sf::Transform override;
    [[nodiscard]] auto getIsWidthManaged() const -> bool override;
    [[nodiscard]] auto getIsHeightManaged() const -> bool override;
    [[nodiscard]] auto getMinWidth() const -> float override;
    [[nodiscard]] auto getMinHeight() const -> float override;
    [[nodiscard]] auto getWidth() const -> float override;
    [[nodiscard]] auto getHeight() const -> float override;
    auto removeChild(std::shared_ptr<Actor> child) -> void override;
    auto setChild(std::shared_ptr<Actor> child) -> void;
    [[nodiscard]] auto getChild() const -> std::shared_ptr<Actor>;

  protected:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  private:
    auto setWidthImpl(float width) -> void override;
    auto setHeightImpl(float height) -> void override;

  public:
    explicit Padding(float top = 0,
                     float bottom = 0,
                     float left = 0,
                     float right = 0);

    [[nodiscard]] auto getTop() const -> float;
    [[nodiscard]] auto getBottom() const -> float;
    [[nodiscard]] auto getLeft() const -> float;
    [[nodiscard]] auto getRight() const -> float;
    auto setTop(float top) -> void;
    auto setBottom(float bottom) -> void;
    auto setLeft(float left) -> void;
    auto setRight(float right) -> void;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_PADDING_H
