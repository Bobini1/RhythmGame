//
// Created by bobini on 01.08.2022.
//

#include "WindowStateMachineImpl.h"
void
state_transitions::WindowStateMachineImpl::update(
  std::chrono::nanoseconds delta)
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
