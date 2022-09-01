//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
#define RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H

#include "WindowStateMachine.h"
namespace state_transitions {
class WindowStateMachineImpl
{
    std::shared_ptr<drawing::Window> current;

  public:
    explicit WindowStateMachineImpl();
    auto changeWindow(std::shared_ptr<drawing::Window> window) -> void;
    auto update(std::chrono::nanoseconds delta) -> void;
    [[nodiscard]] auto isOpen() const -> bool;
    auto pollEvents() -> void;
    auto draw() const -> void;
    [[nodiscard]] auto getCurrentWindow() const
      -> std::shared_ptr<drawing::Window>;
};
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
