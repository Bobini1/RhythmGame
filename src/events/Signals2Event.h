//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_SIGNALS2EVENT_H
#define RHYTHMGAME_SIGNALS2EVENT_H
#include <boost/signals2/signal.hpp>
#include "Signals2Connection.h"

namespace events {

class Signals2Event
{
  public:
    template<std::invocable<> Fun>
    [[nodiscard]] auto subscribe(Fun callback) -> std::unique_ptr<Connection>
    {
        return std::make_unique<Signals2Connection>(
          signal.connect(std::move(callback)));
    }

    auto operator()() const -> void { signal(); }

  private:
    boost::signals2::signal<void()> signal;
};
} // namespace events

#endif // RHYTHMGAME_SIGNALS2EVENT_H
