//
// Created by bobini on 24.09.22.
//

#ifndef RHYTHMGAME_LISTENER_H
#define RHYTHMGAME_LISTENER_H
#include <any>
#include <vector>
#include <span>

namespace events {
class Listener
{
  public:
    virtual void onEvent(std::span<std::any> args) = 0;
    virtual ~Listener() = default;
};
} // namespace events

#endif // RHYTHMGAME_LISTENER_H
