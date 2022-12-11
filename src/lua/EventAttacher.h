//
// Created by bobini on 11.12.22.
//

#ifndef RHYTHMGAME_EVENTATTACHER_H
#define RHYTHMGAME_EVENTATTACHER_H

#include <memory>
#include <map>
#include <functional>
#include <sol/function.hpp>
#include "drawing/actors/Actor.h"
#include "events/Event.h"
namespace lua {

class EventAttacher
{
  public:
    using CppEventInterface =
      std::function<void(std::shared_ptr<drawing::actors::Actor>,
                         sol::function)>;

    auto attachAllEvents(const std::shared_ptr<drawing::actors::Actor>& actor,
                         const sol::table& events) const -> void;

    /**
     * @brief Goes through all registered events and adds them to the actor type
     * as properties.
     * @param actorType The actor type object to which the properties should be
     * added
     */
    auto registerAllEventProperties(
      sol::usertype<drawing::actors::Actor> actorType) const -> void;

  private:
    std::shared_ptr<std::map<std::string, CppEventInterface>> eventRegistrators;

    auto attachEvent(std::string eventName,
                     std::shared_ptr<drawing::actors::Actor> actor,
                     sol::function function) const -> void;

    sol::state* target;

  public:
    explicit EventAttacher(sol::state* target);

    /**
     * @brief Registers an event in lua.
     * @tparam EventType The event's type.
     * @tparam Args The event's invocation arguments.
     * @param target The state to which the event should be added.
     * @param event The event to be added.
     * @param name The name of the event.
     */
    template<typename EventType, typename... Args>
        requires events::Event<EventType, std::function<void(Args...)>, Args...>
    auto addEvent(EventType& event, std::string name) -> void
    {
        (*eventRegistrators)[name + "Event"] =
          [luaTarget = target, &event, name](
            const std::shared_ptr<drawing::actors::Actor>& actor,
            sol::function function) {
              actor->setEventSubscription(
                name + "Event",
                event.subscribe(
                  [luaTarget,
                   actorWeak = std::weak_ptr<drawing::actors::Actor>(actor),
                   function = std::move(function)](Args&&... args) {
                      auto actor = actorWeak.lock();
                      function(actor->getLuaSelf(*luaTarget), args...);
                  }));
          };
    }
};
} // namespace lua
#endif // RHYTHMGAME_EVENTATTACHER_H
