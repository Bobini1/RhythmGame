//
// Created by bobini on 11.12.22.
//

#include "EventAttacher.h"
#include <spdlog/spdlog.h>

#include <utility>

namespace lua {
auto
EventAttacher::attachAllEvents(drawing::actors::Actor* actor,
                               const sol::table& events) const -> void
{
    for (auto& [eventName, propertyAccessors] : *eventRegistrators) {
        auto event = events[eventName];
        if (!event.valid()) {
            continue;
        }
        if (event.get_type() != sol::type::function) {
            spdlog::error("Event handler for {} is not a function, skipping",
                          eventName);
            continue;
        }
        attachEvent(eventName, actor, event.get<sol::function>());
    }
}
auto
EventAttacher::attachEvent(const std::string& eventName,
                           drawing::actors::Actor* actor,
                           sol::function function) const -> void
{
    eventRegistrators->at(eventName).second(actor, std::move(function));
}
EventAttacher::EventAttacher(sol::state* target)
  : eventRegistrators(
      std::make_shared<
        std::map<std::string, std::pair<GetterFunction, SetterFunction>>>())
  , target(target)
{
}
auto
EventAttacher::registerAllEventProperties(
  sol::usertype<drawing::actors::Actor> actorType) const -> void
{
    for (const auto& [name, registrator] : *eventRegistrators) {
        std::ignore = registrator;
        actorType[name] = sol::property(
          [nameCopy = name, eventAttacher = *this](
            drawing::actors::Actor* actor, const sol::object& callback) {
              if (callback.is<sol::function>()) {
                  eventAttacher.attachEvent(
                    nameCopy, actor, callback.as<sol::function>());
              } else if (callback.is<sol::lua_nil_t>()) {
                  eventAttacher.attachEvent(nameCopy, actor, sol::lua_nil);
              } else {
                  spdlog::error("Invalid type for {}. Function or nil expected",
                                nameCopy);
              }
          },
          [nameCopy = name,
           eventAttacher = *this](drawing::actors::Actor* actor) {
              return eventAttacher.eventRegistrators->at(nameCopy).first(actor);
          });
    }
}
} // namespace lua