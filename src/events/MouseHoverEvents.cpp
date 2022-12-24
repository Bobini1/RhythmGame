//
// Created by bobini on 24.12.22.
//

#include "MouseHoverEvents.h"
events::MouseHoverEvents::OnMouseEnter::OnMouseEnter(sol::state* target)
  : target(target)
{
}
auto
events::MouseHoverEvents::OnMouseEnter::subscribe(
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
events::MouseHoverEvents::OnMouseEnter::getSubscription(
  const std::weak_ptr<drawing::actors::Actor>& actor) const -> sol::function
{
    if (listeners.find(actor) != listeners.end()) {
        return listeners.at(actor);
    }
    return sol::nil;
}
void
events::MouseHoverEvents::OnMouseEnter::run(
  std::set<std::weak_ptr<const drawing::actors::Actor>,
           std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
    hoveredPreviously,
  std::set<std::weak_ptr<const drawing::actors::Actor>,
           std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
    hoveredNow,
  sf::Vector2f position)
{
    decltype(hoveredNow) onlyHoveredPreviously;
    std::set_difference(
      hoveredNow.begin(),
      hoveredNow.end(),
      hoveredPreviously.begin(),
      hoveredPreviously.end(),
      std::inserter(onlyHoveredPreviously, onlyHoveredPreviously.begin()),
      std::owner_less<std::weak_ptr<const drawing::actors::Actor>>());
    for (const auto& [listener, callback] : listeners) {
        if (auto actor = listener.lock()) {
            if (onlyHoveredPreviously.find(actor) !=
                onlyHoveredPreviously.end()) {
                callback(actor->getLuaSelf(*target), position);
            }
        } else {
            listeners.erase(listener);
        }
    }
}
events::MouseHoverEvents::OnMouseLeave::OnMouseLeave(sol::state* target)
  : target(target)
{
}
auto
events::MouseHoverEvents::OnMouseLeave::subscribe(
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
events::MouseHoverEvents::OnMouseLeave::getSubscription(
  const std::weak_ptr<drawing::actors::Actor>& actor) const -> sol::function
{
    if (listeners.find(actor) != listeners.end()) {
        return listeners.at(actor);
    }
    return sol::nil;
}
void
events::MouseHoverEvents::OnMouseLeave::run(
  std::set<std::weak_ptr<const drawing::actors::Actor>,
           std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
    hoveredPreviously,
  std::set<std::weak_ptr<const drawing::actors::Actor>,
           std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
    hoveredNow,
  sf::Vector2f position)
{
    decltype(hoveredPreviously) onlyHoveredNow;
    std::set_difference(
      hoveredPreviously.begin(),
      hoveredPreviously.end(),
      hoveredNow.begin(),
      hoveredNow.end(),
      std::inserter(onlyHoveredNow, onlyHoveredNow.begin()),
      std::owner_less<std::weak_ptr<const drawing::actors::Actor>>());
    for (const auto& [listener, callback] : listeners) {
        if (auto actor = listener.lock()) {
            if (onlyHoveredNow.find(actor) != onlyHoveredNow.end()) {
                callback(actor->getLuaSelf(*target), position);
            }
        } else {
            listeners.erase(listener);
        }
    }
}
events::MouseHoverEvents::MouseHoverEvents(sol::state* target)
  : onMouseEnter(target)
  , onMouseLeave(target)
{
}
void
events::MouseHoverEvents::update(drawing::actors::Actor& root,
                                 sf::Vector2f position)
{
    std::set<std::weak_ptr<const drawing::actors::Actor>,
             std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
      hoveredNow;
    root.getAllActorsAtMousePosition(position, hoveredNow);
    onMouseEnter.run(hoveredPreviously, hoveredNow, position);
    onMouseLeave.run(hoveredPreviously, hoveredNow, position);

    hoveredPreviously = hoveredNow;
}
