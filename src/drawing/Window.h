//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOW_H
#define RHYTHMGAME_WINDOW_H

#include <SFML/Graphics/RenderWindow.hpp>
#include <chrono>
namespace drawing {
class Window : public sf::RenderWindow
{
  public:
    Window(const sf::VideoMode& mode,
           const std::string& title,
           const sf::Uint32& style,
           const sf::ContextSettings& settings = sf::ContextSettings());
    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    virtual auto draw() -> void = 0;
};
} // namespace state_transitions
#endif // RHYTHMGAME_WINDOW_H
