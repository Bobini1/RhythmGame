//
// Created by bobini on 01.08.2022.
//

#include "Window.h"
drawing::Window::Window(const sf::VideoMode& mode,
                        const std::string& title,
                        const sf::Uint32& style,
                        const sf::ContextSettings& settings)
  : sf::RenderWindow(mode, title, style, settings)
{
}
