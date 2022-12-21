//
// Created by PC on 20/12/2022.
//

#ifndef RHYTHMGAME_SIGNALS2SIGNALSUBSCRIPTIONWITHSETTABLEFUNCTION_H
#define RHYTHMGAME_SIGNALS2SIGNALSUBSCRIPTIONWITHSETTABLEFUNCTION_H

#include <memory>
#include <any>
#include "drawing/actors/Actor.h"
template<std::derived_from<drawing::actors::Actor> ActorType, typename... Args>
class Signals2SignalSubscriptionWithSettableFunction
{
    std::weak_ptr<ActorType> actor;

  public:
    explicit Signals2SignalSubscriptionWithSettableFunction(
      std::weak_ptr<ActorType> actor)
      : actor(actor){};

    std::function<void(ActorType&, Args...)> currentFunction; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    auto operator()(Args&&... args) const -> void
    {
        if (auto actorShared = actor.lock()) {
            currentFunction(*actorShared, std::forward<Args>(args)...);
        }
    }
};

#endif // RHYTHMGAME_SIGNALS2SIGNALSUBSCRIPTIONWITHSETTABLEFUNCTION_H
