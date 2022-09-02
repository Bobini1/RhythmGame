//
// Created by bobini on 01.08.2022.
//

#include "SceneStateMachineImpl.h"
#include <SFML/Graphics/RenderTarget.hpp>

#include <utility>
namespace state_transitions {
void
SceneStateMachineImpl::changeScene(std::shared_ptr<drawing::Scene> scene)
{
    current = std::move(scene);
}
auto
SceneStateMachineImpl::update(std::chrono::nanoseconds delta) -> void
{
    current->update(delta);
}

SceneStateMachineImpl::SceneStateMachineImpl(
  std::shared_ptr<drawing::Scene> startingScene)
  : current(std::move(startingScene))
{
}
auto
SceneStateMachineImpl::getCurrentScene() const
  -> std::shared_ptr<drawing::Scene>
{
    return current;
}
void
SceneStateMachineImpl::draw(sf::RenderTarget& target,
                            sf::RenderStates states) const
{
    target.draw(*current, states);
}
} // namespace state_transitions