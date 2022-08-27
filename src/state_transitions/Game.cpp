//
// Created by bobini on 21.08.22.
//

#include "Game.h"
#include <future>
#include <thread>

namespace state_transitions {
auto
Game::run() -> void
{
    std::atomic<bool> finished;
    auto eventManagement =
      std::thread{ [&windowManager = windowManager, &finished] {
          while (!finished.load(std::memory_order_acquire)) {
              windowManager->pollEvents();
          }
      } };
    auto start = std::chrono::high_resolution_clock::now();
    while (windowManager->isOpen()) {
        auto now = std::chrono::high_resolution_clock::now();
        auto delta = now - start;
        start = now;
        windowManager->update(std::chrono::nanoseconds{ delta });
        windowManager->draw();
    }
    finished.store(true, std::memory_order_release);
    eventManagement.join();
}
Game::Game(
  std::shared_ptr<state_transitions::WindowStateMachine> windowStateMachine,
  std::shared_ptr<drawing::Window> startingWindow)
  : windowManager(std::move(windowStateMachine))
{
    windowManager->changeWindow(std::move(startingWindow));
}
} // namespace state_transitions