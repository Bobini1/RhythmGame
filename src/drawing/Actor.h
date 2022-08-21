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
namespace drawing {
class Actor // NOLINT(fuchsia-multiple-inheritance)
  : public sf::Transformable
  , public sf::Drawable
{
  public:
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
};
} // namespace state_transitions
#endif // RHYTHMGAME_ACTOR_H
