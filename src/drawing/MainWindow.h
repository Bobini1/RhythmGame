//
// Created by bobini on 30.07.2022.
//

#ifndef RHYTHMGAME_MAINWINDOW_H
#define RHYTHMGAME_MAINWINDOW_H

#include <stack>
#include "Window.h"
#include "Actor.h"
namespace drawing {
class MainWindow : public Window
{
    std::stack<std::unique_ptr<drawing::Actor>> scenes;

  public:
    MainWindow(const sf::VideoMode& mode,
               const std::string& title,
               sf::Uint32 style = sf::Style::Default,
               const sf::ContextSettings& settings = sf::ContextSettings());

  private:
    auto update(std::chrono::nanoseconds delta) -> void override;
};
} // namespace state_transitions

#endif // RHYTHMGAME_MAINWINDOW_H
