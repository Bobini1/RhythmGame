//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_SCENESTATEMACHINEIMPL_H
#define RHYTHMGAME_SCENESTATEMACHINEIMPL_H
#include "state_transitions/SceneStateMachine.h"
namespace state_transitions {
/**
 * @brief A basic scene manager that is only able to hold one scene.
 */
class SceneStateMachineImpl : public sf::Drawable
{
    std::shared_ptr<drawing::Scene> current;

  public:
    explicit SceneStateMachineImpl(
      std::shared_ptr<drawing::Scene> startingScene);
    /**
     * @brief Changes the active scene to the given one.
     * @param scene
     */
    auto changeScene(std::shared_ptr<drawing::Scene> scene) -> void;
    /**
     * @brief Updates the active scene.
     * @param delta
     */
    auto update(std::chrono::nanoseconds delta) -> void;
    /**
     * @brief Returns the currently active scene.
     * @return
     */
    auto getCurrentScene() const -> std::shared_ptr<drawing::Scene>;

  private:
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
};
} // namespace state_transitions
#endif // RHYTHMGAME_SCENESTATEMACHINEIMPL_H
