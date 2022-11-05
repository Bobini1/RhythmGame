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
    [[nodiscard]] virtual auto measure(MeasurementSpec widthSpec,
                         MeasurementSpec heightSpec) const -> sf::Vector2f = 0;
    virtual auto setLayout(sf::FloatRect layout) -> void = 0;
    [[nodiscard]] virtual auto getLayout() const -> sf::FloatRect = 0;
    [[nodiscard]] virtual auto matchParentWidth() const -> bool = 0;
    [[nodiscard]] virtual auto matchParentHeight() const -> bool = 0;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_ACTOR_H
