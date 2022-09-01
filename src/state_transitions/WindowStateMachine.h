//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOWSTATEMACHINE_H
#define RHYTHMGAME_WINDOWSTATEMACHINE_H

#include <memory>
#include <SFML/Graphics/RenderWindow.hpp>
#include <chrono>
#include <utility>
#include "drawing/Window.h"
namespace state_transitions {
template <typename T>
concept WindowStateMachine = requires(T windowStateMachine, std::shared_ptr<drawing::Window> window, std::chrono::nanoseconds delta)
{
    {windowStateMachine.changeWindow(window)} -> std::same_as<void>;
    {windowStateMachine.update(delta)}  -> std::same_as<void>;
    {std::as_const(windowStateMachine).draw()} -> std::same_as<void>;
    {windowStateMachine.pollEvents()} -> std::same_as<void>;
    {std::as_const(windowStateMachine).isOpen()} -> std::convertible_to<bool>;
    {std::as_const(windowStateMachine).getCurrentWindow()} -> std::convertible_to<std::shared_ptr<drawing::Window>>;
};
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOWSTATEMACHINE_H
