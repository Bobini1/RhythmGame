//
// Created by bobini on 21.08.22.
//

#ifndef RHYTHMGAME_GAME_H
#define RHYTHMGAME_GAME_H
#include "drawing/Window.h"
#include <future>
#include <thread>
namespace state_transitions {

class Game
{
  public:
    std::shared_ptr<drawing::Window> window;
    /**
     * @brief Constructs "the game". The RhythmGame. Or Whatever. The starting
     * window is immediately inserted into the window state machine.
     */
    explicit Game(std::shared_ptr<drawing::Window> startingWindow)
      : window(std::move(startingWindow))
    {
    }
    /**
     * @brief Runs the game, managing the update-draw loop. This function will
     * block until the game is closed.
     */
    auto run() -> void
    {
        // input thread must be the main thread
        auto drawThread = std::jthread{ &Game::drawLoop, this };
        while (window->isOpen()) {
            updateInput();
        }
    }

    auto updateInput() -> void { window->updateInput(); }
    auto drawLoop(std::stop_token stopToken) -> void
    {
        auto start = std::chrono::high_resolution_clock::now();
        while (!stopToken.stop_requested()) {
            auto now = std::chrono::high_resolution_clock::now();
            auto delta = now - start;
            start = now;
            window->update(std::chrono::nanoseconds{ delta });
            window->draw();
        }
    }
};
} // namespace state_transitions
#endif // RHYTHMGAME_GAME_H
