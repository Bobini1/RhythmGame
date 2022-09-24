//
// Created by bobini on 23.09.22.
//

#ifndef RHYTHMGAME_EVENTMANAGERIMPL_H
#define RHYTHMGAME_EVENTMANAGERIMPL_H
#include <sol/sol.hpp>
#include <boost/signals2.hpp>
#include <any>
#include <set>
#include "events/Listener.h"
#include "EventManager.h"

namespace events {

class EventManagerImpl
{
    std::map<Event,
             std::set<std::weak_ptr<Listener>,
                      std::owner_less<std::weak_ptr<Listener>>>>
      callbacks;

  public:
    void connect(Event event, std::weak_ptr<Listener> listener);

    void call(Event event, std::span<std::any> args);

    void disconnect(Event event, const std::weak_ptr<Listener>& listener);
};

static_assert(EventManager<EventManagerImpl>);

} // namespace events

#endif // RHYTHMGAME_EVENTMANAGERIMPL_H
