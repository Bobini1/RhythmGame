//
// Created by bobini on 20.11.22.
//

#ifndef RHYTHMGAME_SIGNALS2EVENT_H
#define RHYTHMGAME_SIGNALS2EVENT_H
#include <boost/signals2/signal.hpp>
#include "Signals2Connection.h"
#include "Signals2SignalSubscriptionWithSettableFunction.h"

namespace events {
template<typename... Args>
class Signals2Event
{
  public:
    template<std::derived_from<drawing::actors::Actor> ActorType,
             std::invocable<drawing::actors::Actor&, Args...> Fun>
    [[nodiscard]] auto subscribe(std::shared_ptr<ActorType> actor, Fun callback)
      -> std::unique_ptr<Connection>
    {
        auto function = std::make_shared<
          Signals2SignalSubscriptionWithSettableFunction<ActorType, Args...>>(
          actor);
        function->currentFunction = callback;
        auto* functionPtr = function.get();
        auto connection =
          signal.connect([fun = std::move(function)](Args... args) {
              (*fun)(std::forward<Args>(args)...);
          });
        return std::make_unique<Signals2Connection>(
          std::move(connection),
          support::FunctionReference(functionPtr->currentFunction));
    }

    auto operator()(Args&&... args) const -> void
    {
        signal(std::forward<Args>(args)...);
    }

  private:
    boost::signals2::signal<void(Args...)> signal;
};
} // namespace events

#endif // RHYTHMGAME_SIGNALS2EVENT_H
