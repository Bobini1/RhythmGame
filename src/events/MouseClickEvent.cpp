//
// Created by bobini on 24.12.22.
//

#include "MouseClickEvent.h"
events::MouseClickEvent::MouseClickEvent(sol::state* target)
  : target(target)
{
}
auto
events::MouseClickEvent::subscribe(
  const std::weak_ptr<drawing::actors::Actor>& actor,
  sol::function callback) -> void
{
    if (callback && actor.lock()) {
        listeners[actor] = std::move(callback);
    } else {
        listeners.erase(actor);
    }
}
auto
events::MouseClickEvent::getSubscription(
  const std::weak_ptr<drawing::actors::Actor>& actor) const -> sol::function
{
    if (listeners.find(actor) != listeners.end()) {
        return listeners.at(actor);
    }
    return sol::lua_nil;
}
auto
events::MouseClickEvent::operator()(const drawing::actors::Actor& root,
                                    sf::Vector2f position) -> void
{
    std::set<std::weak_ptr<const drawing::actors::Actor>,
             std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
      actors;
    root.getAllActorsAtMousePosition(position, actors);
    for (const auto& [listener, callback] : listeners) {
        if (auto actor = listener.lock()) {
            if (actors.find(actor) != actors.end()) {
                callback(actor->getLuaSelf(*target), position);
            }
        } else {
            listeners.erase(listener);
        }
    }
}
