//
// Created by bobini on 11.12.22.
//

#include "EventAttacher.h"
#include <spdlog/spdlog.h>

namespace lua {
auto
EventAttacher::attachAllEvents(
  const std::shared_ptr<drawing::actors::Actor>& actor,
  const sol::table& events) const -> void
{
    for (auto& [key, value] : events) {
        if (value.get_type() != sol::type::function) {
            spdlog::error("Event handler {} is not a function, skipping",
                          key.as<std::string>());
            continue;
        }
        attachEvent(key.as<std::string>(), actor, value.as<sol::function>());
    }
}
auto
EventAttacher::attachEvent(std::string eventName,
                           std::shared_ptr<drawing::actors::Actor> actor,
                           sol::function function) const -> void
{
    if (eventRegistrators->find(eventName) == eventRegistrators->end()) {
        spdlog::error("Event {} not found", eventName);
        return;
    }
    eventRegistrators->at(eventName)(std::move(actor), std::move(function));
}
EventAttacher::EventAttacher(sol::state* target)
  : eventRegistrators(
      std::make_shared<std::map<std::string, CppEventInterface>>())
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
                  eventAttacher.attachEvent(nameCopy,
                                            actor->shared_from_this(),
                                            callback.as<sol::function>());
              } else if (callback.is<sol::lua_nil_t>()) {
                  actor->setEventSubscription(nameCopy, nullptr);
              } else {
                  spdlog::error("Invalid type for {}. Function or nil expected",
                                nameCopy);
              }
          });
    }
}
} // namespace lua