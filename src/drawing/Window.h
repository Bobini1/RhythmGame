//
// Created by bobini on 01.08.2022.
//

#ifndef RHYTHMGAME_WINDOW_H
#define RHYTHMGAME_WINDOW_H

#include <SFML/Graphics/RenderWindow.hpp>
#include <chrono>
#include <SFML/Window/Event.hpp>
#include "input/InputQueue.h"
namespace drawing {
/**
 * @brief Base class for all windows, which are physical windows used for
 * displaying the game. Can be managed by window managers.
 */
class Window
{
    input::InputQueue inputQueue;

  protected:
    sf::RenderWindow window;

  public:
    Window(const sf::VideoMode& mode,
           const std::string& title,
           const sf::Uint32& style,
           const sf::ContextSettings& settings = sf::ContextSettings());
    Window();
    virtual ~Window() = default;
    Window(const Window&) = delete;
    Window(Window&&) = delete;
    auto operator=(const Window&) -> Window& = delete;
    auto operator=(Window&&) -> Window& = delete;

    auto popEvent() -> std::optional<
      std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
                sf::Event>>;
    void updateInput();
    auto mapPixelToCoords(const sf::Vector2i& point) const -> sf::Vector2f;
    auto mapCoordsToPixel(const sf::Vector2f& point) const -> sf::Vector2i;
    auto getSize() const -> sf::Vector2u;
    void close();
    auto isOpen() -> bool;

    virtual auto update(std::chrono::nanoseconds delta) -> void = 0;
    virtual auto draw() -> void = 0;
};
} // namespace drawing
#endif // RHYTHMGAME_WINDOW_H
