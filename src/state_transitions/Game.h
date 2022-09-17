//
// Created by bobini on 21.08.22.
//

#ifndef RHYTHMGAME_GAME_H
#define RHYTHMGAME_GAME_H
#include "state_transitions/WindowStateMachineImpl.h"
#include <future>
#include <thread>
namespace state_transitions {

template<WindowStateMachine WindowStateMachineType>
class Game
{
    WindowStateMachineType windowManager;

  public:
    /**
     * @brief Constructs "the game". The RhythmGame. Or Whatever. The starting
     * window is immediately inserted into the window state machine.
     */
    Game(WindowStateMachineType windowStateMachine,
         std::shared_ptr<drawing::Window> startingWindow)
      : windowManager(std::move(windowStateMachine))
    {
        windowManager.changeWindow(std::move(startingWindow));
    }
    /**
     * @brief Runs the game, managing the update-draw loop. This function will
     * block until the game is closed.
     */
    auto run() -> void
    {
        std::atomic<bool> finished;
        auto eventManagement =
          std::thread{ [&windowManager = windowManager, &finished] {
              while (!finished.load(std::memory_order_acquire)) {
                  windowManager.pollEvents();
              }
          } };
        auto start = std::chrono::high_resolution_clock::now();
        while (windowManager.isOpen()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto delta = now - start;
            start = now;
            windowManager.update(std::chrono::nanoseconds{ delta });
            windowManager.draw();
        }
        finished.store(true, std::memory_order_release);
        eventManagement.join();
    }
};
} // namespace state_transitions
#endif // RHYTHMGAME_GAME_H
