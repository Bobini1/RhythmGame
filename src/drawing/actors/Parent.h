//
// Created by bobini on 14.10.22.
//

#ifndef RHYTHMGAME_PARENT_H
#define RHYTHMGAME_PARENT_H

#include <memory>
#include "Actor.h"
namespace drawing::actors {
class Parent : public Actor
{
  public:
    virtual auto removeChild(std::shared_ptr<Actor> child) -> void = 0;
};
} // namespace drawing::actors

#endif // RHYTHMGAME_PARENT_H
