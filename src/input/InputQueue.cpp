//
// Created by bobini on 20.06.23.
//

#include "InputQueue.h"
void
input::InputQueue::update(sf::Window& window)
{
    auto now = std::chrono::high_resolution_clock::now();
    auto event = sf::Event{};
    while (window.pollEvent(event)) {
        queue.push({ now, event });
    }
}
auto
input::InputQueue::pop() -> std::optional<
  std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
            sf::Event>>
{
    auto value =
      std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
                sf::Event>{};
    if (queue.pop(value)) {
        return value;
    }
    return std::nullopt;
}
