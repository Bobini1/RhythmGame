//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_SCENESTATEMACHINE_H
#define RHYTHMGAME_SCENESTATEMACHINE_H

#include <drawing/Scene.h>
#include <chrono>
namespace state_transitions {
template<typename T>
concept SceneStateMachine = requires(T sceneStateMachine, std::shared_ptr<drawing::Scene> scene, std::chrono::nanoseconds delta)
{
    std::is_base_of_v<sf::Drawable, T>;
    {sceneStateMachine.changeScene(scene)} -> std::same_as<void>;
    {sceneStateMachine.update(delta)} -> std::same_as<void>;
    {sceneStateMachine.getCurrentScene()} -> std::convertible_to<std::shared_ptr<drawing::Scene>>;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SCENESTATEMACHINE_H
