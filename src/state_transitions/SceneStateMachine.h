//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_SCENESTATEMACHINE_H
#define RHYTHMGAME_SCENESTATEMACHINE_H

#include <drawing/Scene.h>
#include <chrono>
namespace state_transitions {
class SceneStateMachine
{
  public:
    virtual ~SceneStateMachine() = default;
    virtual auto changeScene(std::shared_ptr<drawing::Scene> scene) -> void = 0;
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    virtual auto getCurrentScene() -> std::shared_ptr<drawing::Scene> = 0;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SCENESTATEMACHINE_H
