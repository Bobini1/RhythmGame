//
// Created by bobini on 31.07.2022.
//

#include "SplashWindow.h"
#include <SFML/Window/Event.hpp>
namespace drawing {
auto
SplashWindow::draw() -> void
{
    window.clear();
    window.draw(*splashScene);
    window.display();
}
auto
SplashWindow::update(std::chrono::nanoseconds delta) -> void
{
    splashScene->update(delta, *this);
}
SplashWindow::SplashWindow(std::shared_ptr<drawing::Scene> splashScene,
                           const sf::VideoMode& mode,
                           const std::string& title,
                           const sf::ContextSettings& settings)
  : Window(mode, title, sf::Style::Default, settings)
  , splashScene(std::move(splashScene))
{
    window.setPosition({ 0, 0 });
}
} // namespace drawing