//
// Created by bobini on 31.07.2022.
//

#ifndef RHYTHMGAME_SCENE_H
#define RHYTHMGAME_SCENE_H

#include <SFML/Graphics/Drawable.hpp>
#include <chrono>
#include <sol/state.hpp>
#include "lua/Bootstrapper.h"
namespace drawing {
/**
 * @brief Base class for all scenes, which are window states holding actors and
 * optionally managed by scene managers.
 */
class Scene : public sf::Drawable
{
  public:
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    /**
     * @brief Registers all events that this scene uses in lua.
     * @param target The state to which the events should be added.
     * @param bootstrapper The bootstrapper that adds the events.
     */
    virtual auto defineEvents(sol::state& target,
                              lua::Bootstrapper& bootstrapper) -> void = 0;
    /**
     * @brief Sets the root actor of this scene.
     * @param root
     */
    virtual auto setRoot(std::shared_ptr<actors::Actor> root) -> void = 0;
};
} // namespace drawing

#endif // RHYTHMGAME_SCENE_H
