//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_EVENT_H
#define RHYTHMGAME_EVENT_H

#include "events/Connection.h"
#include <utility>
#include <memory>
namespace events {
template<typename T, typename Fun, typename... Args>
concept Event = std::invocable<Fun, Args...> &&
                requires(T event, Fun&& callback, Args&&... args) {
                    {
                        event.subscribe(callback)
                    } -> std::convertible_to<std::unique_ptr<Connection>>;
                    {
                        std::as_const(event)(args...)
                    };
                };
} // namespace events
#endif // RHYTHMGAME_EVENT_H
