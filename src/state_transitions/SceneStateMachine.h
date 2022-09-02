//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_SCENESTATEMACHINE_H
#define RHYTHMGAME_SCENESTATEMACHINE_H

#include <drawing/Scene.h>
#include <chrono>
namespace state_transitions {
/**
 * @brief Concept representing all scene state machines, which are responsible for
 * managing scenes and passing updates and draw requests to the active ones.
 */
template<typename T>
concept SceneStateMachine = requires(T sceneStateMachine, std::shared_ptr<drawing::Scene> scene, std::chrono::nanoseconds delta)
{
    std::is_base_of_v<sf::Drawable, T>;
    /**
     * @brief Changes the active scene to the given one.
     * @param scene The scene to change to.
     */
    {sceneStateMachine.changeScene(scene)} -> std::same_as<void>;
    /**
     * @brief Updates the active scene.
     * @param delta The time since the last update.
     */
    {sceneStateMachine.update(delta)} -> std::same_as<void>;
    /**
     * @brief Returns the currently active scene.
     * @return The currently active scene.
     */
    {std::as_const(sceneStateMachine).getCurrentScene()} -> std::convertible_to<std::shared_ptr<drawing::Scene>>;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SCENESTATEMACHINE_H
