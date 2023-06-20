//
// Created by bobini on 20.06.23.
//

#ifndef RHYTHMGAME_INPUTQUEUE_H
#define RHYTHMGAME_INPUTQUEUE_H

#include <SFML/Window/Window.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <SFML/Window/Event.hpp>
#include <chrono>

/**
 * @brief namespace input provides classes for managing input events
 */
namespace input {
/**
 * @brief A class providing access to the events that happened and their
 * timestamps.
 * @warning Only a single thread can read from this class at a time!
 */
class InputQueue
{
  public:
    static constexpr auto maxEventsStored = 1024;

  private:
    boost::lockfree::spsc_queue<
      std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
                sf::Event>,
      boost::lockfree::capacity<maxEventsStored>>
      queue;

  public:
    void update(sf::Window& window);
    auto pop() -> std::optional<
      std::pair<std::chrono::time_point<std::chrono::high_resolution_clock>,
                sf::Event>>;
};
} // namespace input

#endif // RHYTHMGAME_INPUTQUEUE_H
