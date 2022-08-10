//
// Created by bobini on 01.08.2022.
//

#include <SFML/Window/Event.hpp>
#include "WindowStateMachineImpl.h"
auto
state_transitions::WindowStateMachineImpl::update(
  std::chrono::nanoseconds delta) -> void
{
    current->update(delta);
}
auto
state_transitions::WindowStateMachineImpl::changeWindow(
  std::shared_ptr<drawing::Window> window) -> void
{
    current = std::move(window);
}
state_transitions::WindowStateMachineImpl::WindowStateMachineImpl(
  std::shared_ptr<drawing::Window> startingWindow)
  : current(std::move(startingWindow))
{
}
auto
state_transitions::WindowStateMachineImpl::isOpen() -> bool
{
    return current->isOpen();
}
auto
state_transitions::WindowStateMachineImpl::pollEvents() -> void
{
    sf::Event event{};
    while (current->pollEvent(event)) {
        // "close requested" event: we close the window
        if (event.type == sf::Event::Closed) {
            current->close();
        }
    }
}
auto
state_transitions::WindowStateMachineImpl::draw() -> void
{
    current->draw();
}
