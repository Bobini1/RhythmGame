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
    auto defineTypes(sol::state& target) const -> void;
};
} // namespace lua
#endif // RHYTHMGAME_BOOTSTRAPPER_H
