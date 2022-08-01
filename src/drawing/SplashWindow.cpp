//
// Created by bobini on 31.07.2022.
//

#include "SplashWindow.h"
#include <SFML/Window/Event.hpp>
namespace drawing {
auto
SplashWindow::update(std::chrono::nanoseconds delta) -> void
{
    auto event = sf::Event{};
    while (pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            close();
        }
    }
    splashScene->update(delta);
}
SplashWindow::SplashWindow(std::unique_ptr<Scene> splashScene,
                           const sf::VideoMode& mode,
                           const std::string& title,
                           const sf::ContextSettings& settings)
  : Window(mode, title, sf::Style::None, settings)
  , splashScene(std::move(splashScene))
{
}
} // namespace drawing