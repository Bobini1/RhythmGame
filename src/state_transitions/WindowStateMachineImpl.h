//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
#define RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H

#include "WindowStateMachine.h"
namespace state_transitions {
class WindowStateMachineImpl : public WindowStateMachine
{
    std::shared_ptr<drawing::Window> current;

  public:
    explicit WindowStateMachineImpl();
    auto changeWindow(std::shared_ptr<drawing::Window> window) -> void override;
    auto update(std::chrono::nanoseconds delta) -> void override;
    auto isOpen() -> bool override;
    auto pollEvents() -> void override;
    auto draw() const -> void override;
    auto getCurrentWindow() -> std::shared_ptr<drawing::Window> override;
};
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
