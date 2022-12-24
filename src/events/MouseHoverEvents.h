//
// Created by bobini on 24.12.22.
//

#ifndef RHYTHMGAME_MOUSEHOVEREVENTS_H
#define RHYTHMGAME_MOUSEHOVEREVENTS_H

#include <memory>
#include <algorithm>
#include "Event.h"
namespace events {
class MouseHoverEvents
{
  public:
    class OnMouseEnter
    {
        std::map<std::weak_ptr<drawing::actors::Actor>,
                 sol::function,
                 std::owner_less<std::weak_ptr<drawing::actors::Actor>>>
          listeners;
        sol::state* target;

      public:
        explicit OnMouseEnter(sol::state* target);
        auto subscribe(const std::weak_ptr<drawing::actors::Actor>& actor,
                       sol::function callback) -> void;

        [[nodiscard]] auto getSubscription(
          const std::weak_ptr<drawing::actors::Actor>& actor) const
          -> sol::function;

        void run(
          std::set<std::weak_ptr<const drawing::actors::Actor>,
                   std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
            hoveredPreviously,
          std::set<std::weak_ptr<const drawing::actors::Actor>,
                   std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
            hoveredNow,
          sf::Vector2f position);
    };
    class OnMouseLeave
    {
        std::map<std::weak_ptr<drawing::actors::Actor>,
                 sol::function,
                 std::owner_less<std::weak_ptr<drawing::actors::Actor>>>
          listeners;
        sol::state* target;

      public:
        explicit OnMouseLeave(sol::state* target);
        auto subscribe(const std::weak_ptr<drawing::actors::Actor>& actor,
                       sol::function callback) -> void;

        [[nodiscard]] auto getSubscription(
          const std::weak_ptr<drawing::actors::Actor>& actor) const
          -> sol::function;

        void run(
          std::set<std::weak_ptr<const drawing::actors::Actor>,
                   std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
            hoveredPreviously,
          std::set<std::weak_ptr<const drawing::actors::Actor>,
                   std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
            hoveredNow,
          sf::Vector2f position);
    };
    OnMouseEnter onMouseEnter;
    OnMouseLeave onMouseLeave;

    explicit MouseHoverEvents(sol::state* target);

    void update(drawing::actors::Actor& root, sf::Vector2f position);

  private:
    std::set<std::weak_ptr<const drawing::actors::Actor>,
             std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
      hoveredPreviously;
};
} // namespace events
#endif // RHYTHMGAME_MOUSEHOVEREVENTS_H
