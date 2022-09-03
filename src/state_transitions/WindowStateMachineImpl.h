//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
#define RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H

#include "WindowStateMachine.h"
namespace state_transitions {
/**
 * @brief A simple window manager that is only able to hold one window.
 */
class WindowStateMachineImpl
{
    std::shared_ptr<drawing::Window> current;

  public:
    explicit WindowStateMachineImpl();
    /**
     * @brief Changes the active window to the given one.
     * @param window
     */
    auto changeWindow(std::shared_ptr<drawing::Window> window) -> void;
    /**
     * @brief Updates the single window.
     * @param delta
     */
    auto update(std::chrono::nanoseconds delta) -> void;
    /**
     * @brief returns true if the active window is open (not closed by the
     * user by any means).
     */
    [[nodiscard]] auto isOpen() const -> bool;
    /**
     * @brief Closes the window if it was requested by the user.
     */
    auto pollEvents() -> void;
    /**
     * @brief Draws the active window.
     */
    auto draw() const -> void;
    /**
     * @brief Returns the active window.
     */
    [[nodiscard]] auto getCurrentWindow() const
      -> std::shared_ptr<drawing::Window>;
};

static_assert(WindowStateMachine<WindowStateMachineImpl>);
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOWSTATEMACHINEIMPL_H
