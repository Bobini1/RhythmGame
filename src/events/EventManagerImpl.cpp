//
// Created by bobini on 23.09.22.
//

#include "EventManagerImpl.h"
void
events::EventManagerImpl::connect(events::Event event,
                                  std::weak_ptr<Listener> listener)
{
    callbacks[event].insert(std::move(listener));
}
void
events::EventManagerImpl::call(events::Event event, std::span<std::any> args)
{
    for (const auto& listener : callbacks[event]) {
        listener.lock()->onEvent(args);
    }
}
void
events::EventManagerImpl::disconnect(events::Event event,
                                     const std::weak_ptr<Listener>& listener)
{
    callbacks[event].erase(listener);
}
