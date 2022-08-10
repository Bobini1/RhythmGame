//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOWSTATEMACHINE_H
#define RHYTHMGAME_WINDOWSTATEMACHINE_H

#include <memory>
#include <SFML/Graphics/RenderWindow.hpp>
#include <chrono>
#include "drawing/Window.h"
namespace state_transitions {
class WindowStateMachine
{
  public:
    virtual ~WindowStateMachine() = default;
    virtual auto changeWindow(std::shared_ptr<drawing::Window> window)
      -> void = 0;
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    virtual auto draw() const -> void = 0;
    virtual auto pollEvents() -> void = 0;
    virtual auto isOpen() -> bool = 0;
};
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOWSTATEMACHINE_H
