//
// Created by bobini on 21.08.22.
//

#ifndef RHYTHMGAME_GAME_H
#define RHYTHMGAME_GAME_H
#include "state_transitions/WindowStateMachineImpl.h"
namespace state_transitions {
class Game
{
    std::shared_ptr<state_transitions::WindowStateMachine> windowManager;

  public:
    Game(
      std::shared_ptr<state_transitions::WindowStateMachine> windowStateMachine,
      std::shared_ptr<drawing::Window> startingWindow);
    auto run() -> void;
};
} // namespace state_transitions
#endif // RHYTHMGAME_GAME_H
