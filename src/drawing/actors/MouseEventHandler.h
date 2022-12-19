//
// Created by PC on 18/12/2022.
//

#ifndef RHYTHMGAME_MOUSEEVENTHANDLER_H
#define RHYTHMGAME_MOUSEEVENTHANDLER_H
#include <unordered_map>
#include <functional>
namespace drawing::actors {
enum class EventType
{
    MouseEnter,
    MouseLeave,
    Mouse1Down,
    Mouse1Up,
    Mouse2Down,
    Mouse2Up,
};
class MouseEventHandler
{
  public:
    auto handleEvent(EventType eventType) -> bool;
    auto setEvent(EventType eventType, std::function<void()> eventHandler)
      -> void;

  protected:
    ~MouseEventHandler() = default;

  private:
    std::unordered_map<EventType, std::function<void()>>
      eventHandlers;
};
} // namespace drawing::actors
#endif // RHYTHMGAME_MOUSEEVENTHANDLER_H
