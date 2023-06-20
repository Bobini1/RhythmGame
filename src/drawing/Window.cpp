//
// Created by bobini on 01.08.2022.
//

#include "Window.h"
drawing::Window::Window(const sf::VideoMode& mode,
                        const std::string& title,
                        const sf::Uint32& style,
                        const sf::ContextSettings& settings)
  : window(mode, title, style, settings)
{
}
drawing::Window::Window() = default;
auto
drawing::Window::popEvent() -> std::optional<
  std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
            sf::Event>>
{
    return inputQueue.pop();
}
void
drawing::Window::updateInput()
{
    inputQueue.update(window);
}
auto
drawing::Window::mapPixelToCoords(const sf::Vector2i& point) const
  -> sf::Vector2f
{
    return window.mapPixelToCoords(point);
}
auto
drawing::Window::mapCoordsToPixel(const sf::Vector2f& point) const
  -> sf::Vector2i
{
    return window.mapCoordsToPixel(point);
}
auto
drawing::Window::getSize() const -> sf::Vector2u
{
    return window.getSize();
}
void
drawing::Window::close()
{
    window.close();
}
auto
drawing::Window::isOpen() -> bool
{
    return window.isOpen();
}
