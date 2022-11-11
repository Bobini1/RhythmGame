//
// Created by bobini on 14.10.22.
//

#ifndef RHYTHMGAME_PARENT_H
#define RHYTHMGAME_PARENT_H

#include <memory>
#include "Actor.h"
namespace drawing::actors {
/**
 * A parent is an actor that can contain other actors.
 * It is responsible for managing the positioning of its children. There is no
 * addChild method here because every parent can implement it differently (some
 * parents may only allow one child, for example).
 */
class Parent : public Actor
{
  public:
    /**
     * @brief removes a child from the parent.
     * @param child The child to remove.
     * This function is also called when a child is added to a new parent.
     */
    virtual auto removeChild(std::shared_ptr<Actor> child) -> void = 0;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_PARENT_H
