//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_GLOBALEVENT_H
#define RHYTHMGAME_GLOBALEVENT_H
#include "Event.h"

namespace events {

template<typename... Args>
class GlobalEvent
{
  public:
    explicit GlobalEvent(sol::state* target)
      : target(target)
    {
    }
    auto subscribe(const std::weak_ptr<drawing::actors::Actor>& actor,
                   sol::function callback) -> void
    {
        if (callback && actor.lock()) {
            listeners[actor] = std::move(callback);
        } else {
            listeners.erase(actor);
        }
    }

    [[nodiscard]] auto getSubscription(
      const std::weak_ptr<drawing::actors::Actor>& actor) const -> sol::function
    {
        if (listeners.find(actor) != listeners.end()) {
            return listeners.at(actor);
        }
        return sol::lua_nil;
    }

    auto operator()(Args... args) -> void
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
static_assert(Event<GlobalEvent<>>);
} // namespace events

#endif // RHYTHMGAME_GLOBALEVENT_H
