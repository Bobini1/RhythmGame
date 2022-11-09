//
// Created by bobini on 30.07.2022.
//

#ifndef RHYTHMGAME_ACTOR_H
#define RHYTHMGAME_ACTOR_H

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Transformable.hpp>
#include <chrono>
#include <vector>
#include <memory>
#include <sol/state.hpp>
#include "support/EnableSharedFromBase.h"
#include "drawing/MeasurementTypes.h"
namespace drawing::actors {
class Parent;
/**
 * @brief Base class for all drawable objects.
 */
class Actor // NOLINT(fuchsia-multiple-inheritance)
  : public sf::Drawable
  , public support::EnableSharedFromBase<Actor>
{
    std::weak_ptr<Parent> parent;

  public:
    [[nodiscard]] virtual auto getParent() const -> std::shared_ptr<Parent>;
    virtual auto setParent(const std::shared_ptr<Parent>& parent) -> void;
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    [[nodiscard]] virtual auto getLuaSelf(sol::state& lua) -> sol::object = 0;
    virtual auto setTransform(sf::Transform transform) -> void = 0;
    [[nodiscard]] virtual auto getTransform() const -> sf::Transform = 0;
    [[nodiscard]] virtual auto matchParentWidth() const -> bool = 0;
    [[nodiscard]] virtual auto matchParentHeight() const -> bool = 0;
    [[nodiscard]] virtual auto getMinWidth() const -> float = 0;
    [[nodiscard]] virtual auto getMinHeight() const -> float = 0;
    [[nodiscard]] virtual auto getWidth() const -> float = 0;
    [[nodiscard]] virtual auto getHeight() const -> float = 0;
    auto setWidth(float width) -> void;
    auto setHeight(float height) -> void;

  private:
    virtual auto setWidthImpl(float width) -> void = 0;
    virtual auto setHeightImpl(float height) -> void = 0;

};
} // namespace drawing::actors
#endif // RHYTHMGAME_ACTOR_H
