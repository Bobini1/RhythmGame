//
// Created by bobini on 24.12.22.
//

#ifndef RHYTHMGAME_MOUSECLICKEVENT_H
#define RHYTHMGAME_MOUSECLICKEVENT_H

#include <memory>
#include <algorithm>
#include "Event.h"
namespace events {
class MouseClickEvent
{
  public:
    explicit MouseClickEvent(sol::state* target);
    auto subscribe(const std::weak_ptr<drawing::actors::Actor>& actor,
                   sol::function callback) -> void;

    [[nodiscard]] auto getSubscription(
      const std::weak_ptr<drawing::actors::Actor>& actor) const
      -> sol::function;

    auto operator()(const drawing::actors::Actor& root, sf::Vector2f position)
      -> void;

  private:
    std::map<std::weak_ptr<drawing::actors::Actor>,
             sol::function,
             std::owner_less<std::weak_ptr<drawing::actors::Actor>>>
      listeners;
    sol::state* target;
};
} // namespace events
#endif // RHYTHMGAME_MOUSECLICKEVENT_H
