//
// Created by bobini on 24.12.22.
//

#ifndef RHYTHMGAME_EVENT_H
#define RHYTHMGAME_EVENT_H

#include <memory>
#include "lua/Lua.h"
#include "drawing/actors/Actor.h"
namespace events {
template<typename T>
concept Event = requires(T event,
                         const std::weak_ptr<drawing::actors::Actor> actor,
                         sol::function callback) {
                    {
                        event.subscribe(actor, callback)
                    };
                    {
                        event.getSubscription(actor)
                    } -> std::same_as<sol::function>;
                };
} // namespace events

#endif // RHYTHMGAME_EVENT_H
