//
// Created by bobini on 30.07.2022.
//

#include "MainWindow.h"

namespace drawing {
MainWindow::MainWindow(const sf::VideoMode& mode,
                       const std::string& title,
                       sf::Uint32 style,
                       const sf::ContextSettings& settings)
  : Window(mode, title, style, settings)
{
}
void
MainWindow::update(std::chrono::nanoseconds delta)
{
}
} // namespace state_transitions