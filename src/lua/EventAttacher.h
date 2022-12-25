//
// Created by bobini on 11.12.22.
//

#ifndef RHYTHMGAME_EVENTATTACHER_H
#define RHYTHMGAME_EVENTATTACHER_H

#include <memory>
#include <map>
#include <functional>
#include "events/Event.h"
namespace lua {

class EventAttacher
{
  public:
    using SetterFunction =
      std::function<void(std::weak_ptr<drawing::actors::Actor>, sol::function)>;

    using GetterFunction =
      std::function<sol::function(drawing::actors::Actor*)>;

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
    std::shared_ptr<
      std::map<std::string, std::pair<GetterFunction, SetterFunction>>>
      eventRegistrators;

    auto attachEvent(std::string eventName,
                     std::weak_ptr<drawing::actors::Actor> actor,
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
    auto addEvent(EventType& event, std::string name) -> void
        requires events::Event<EventType>
    {
        (*eventRegistrators)[name + "Event"] = {
            [&event, name](drawing::actors::Actor* actor) -> sol::function {
                return event.getSubscription(actor->weak_from_this());
            },
            [&event, name](const std::weak_ptr<drawing::actors::Actor>& actor,
                           sol::function function) {
                event.subscribe(actor, function);
            }
        };
    }
};
} // namespace lua
#endif // RHYTHMGAME_EVENTATTACHER_H