//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_EVENT_H
#define RHYTHMGAME_EVENT_H
#include <sol/function.hpp>
#include "drawing/actors/Actor.h"

namespace events {

template<typename... Args>
class Event
{
  public:
    explicit Event(sol::state* target)
      : target(target)
    {
    }
    auto subscribe(std::weak_ptr<drawing::actors::Actor> actor,
                   sol::function callback) -> void
    {
        if (callback && actor.lock()) {
            listeners.emplace(std::move(actor), std::move(callback));
        } else {
            listeners.erase(actor);
        }
    }

    auto getSubscription(const std::weak_ptr<drawing::actors::Actor>& actor)
      -> sol::function
    {
        if (listeners.find(actor) != listeners.end()) {
            return listeners.at(actor);
        }
        return sol::nil;
    }

    auto operator()(Args&&... args) -> void
    {
        for (const auto& [listener, callback] : listeners) {
            if (auto actor = listener.lock()) {
                callback(actor->getLuaSelf(*target),
                         std::forward<Args>(args)...);
            } else {
                listeners.erase(listener);
            }
        }
    }

  private:
    std::map<std::weak_ptr<drawing::actors::Actor>,
             sol::function,
             std::owner_less<std::weak_ptr<drawing::actors::Actor>>>
      listeners;
    sol::state* target;
};
} // namespace events

#endif // RHYTHMGAME_EVENT_H
