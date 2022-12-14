//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_SIGNALS2EVENT_H
#define RHYTHMGAME_SIGNALS2EVENT_H
#include <boost/signals2/signal.hpp>
#include "events/Event.h"
#include "Signals2Connection.h"

namespace events {
namespace detail {

/**
 * @brief An implementation for Signals2Event
 */
template<typename... Args>
class Signals2Event
{
  public:
    template<std::invocable<Args...> Fun>
    [[nodiscard]] auto subscribe(Fun callback) -> std::unique_ptr<Connection>
    {
        return std::make_unique<Signals2Connection>(
          signal.connect(std::move(callback)));
    }

    auto operator()(Args&&... args) const -> void
    {
        signal(std::forward<Args>(args)...);
    }

  private:
    boost::signals2::signal<void(Args...)> signal;
};
} // namespace priv

/**
 * @brief The public interface for Signals2Event
 */
template<typename... Args>
    requires Event<detail::Signals2Event<Args...>,
                   decltype([](Args...) {}),
                   Args...>
using Signals2Event = detail::Signals2Event<Args...>;
} // namespace events

#endif // RHYTHMGAME_SIGNALS2EVENT_H
