//
// Created by bobini on 10.08.2022.
//

#ifndef RHYTHMGAME_BOOTSTRAPPER_H
#define RHYTHMGAME_BOOTSTRAPPER_H

#include <sol/state.hpp>
namespace lua {
class Bootstrapper
{
  public:
    virtual ~Bootstrapper() = default;
    virtual auto defineTypes(sol::state& target) -> void = 0;
};
} // namespace lua
#endif // RHYTHMGAME_BOOTSTRAPPER_H
