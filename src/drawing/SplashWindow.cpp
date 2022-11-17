//
// Created by bobini on 31.07.2022.
//

#include "SplashWindow.h"
#include <SFML/Window/Event.hpp>
namespace drawing {
auto
SplashWindow::draw() -> void
{
    clear();
    sf::RenderWindow::draw(*splashScene);
    display();
}
auto
SplashWindow::update(std::chrono::nanoseconds delta) -> void
{
    splashScene->update(delta);
}
SplashWindow::SplashWindow(std::shared_ptr<drawing::Scene> splashScene,
                           const sf::VideoMode& mode,
                           const std::string& title,
                           const sf::ContextSettings& settings)
  : Window(mode, title, sf::Style::None, settings)
  , splashScene(std::move(splashScene))
{
    this->setPosition({ 0, 0 });
}
} // namespace drawing