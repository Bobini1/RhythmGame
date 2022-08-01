//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_SCENESTATEMACHINEIMPL_H
#define RHYTHMGAME_SCENESTATEMACHINEIMPL_H
#include "state_transitions/SceneStateMachine.h"
namespace state_transitions {
class SceneStateMachineImpl : public SceneStateMachine
{
    std::shared_ptr<drawing::Scene> current;

  public:
    explicit SceneStateMachineImpl(
      std::shared_ptr<drawing::Scene> startingScene);
    auto changeScene(std::shared_ptr<drawing::Scene> scene) -> void override;
    auto update(std::chrono::nanoseconds delta) -> void override;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SCENESTATEMACHINEIMPL_H
