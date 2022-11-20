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
    virtual auto defineEvents(sol::state& target,
                              lua::Bootstrapper& bootstrapper) -> void = 0;
    virtual auto setRoot(std::shared_ptr<actors::Actor> root) -> void = 0;
};
} // namespace drawing

#endif // RHYTHMGAME_SCENE_H
