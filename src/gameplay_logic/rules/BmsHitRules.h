//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSHITRULES_H
#define RHYTHMGAME_BMSHITRULES_H
#include <optional>
#include <chrono>
#include <span>
#include "gameplay_logic/BmsPoints.h"
#include "charts/gameplay_models/BmsNotesData.h"
#include "gameplay_logic/HitEvent.h"
namespace sounds {
class OpenALSound;
} // namespace sounds
namespace gameplay_logic::rules {

class BmsHitRules
{
  public:
    enum class NoteType
    {
        Normal,
        LnBegin,
        LnEnd,
        Invisible
    };
    struct Note
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        NoteType type;
        int index;
        bool hit = false;
    };
    struct Mine
    {
        double penalty;
        std::chrono::nanoseconds time;
        int index;
        bool hit = false;
    };

    virtual ~BmsHitRules() = default;

    virtual auto press(std::span<Note> notes,
                       int column,
                       std::chrono::nanoseconds hitOffset) -> QList<HitEvent> = 0;

    virtual auto processMisses(std::span<Note> notes,
                               int column,
                               std::chrono::nanoseconds offsetFromStart)
      -> std::vector<HitEvent> = 0;

    virtual auto processMines(std::span<Mine> mines,
                              int column,
                              std::chrono::nanoseconds hitOffset,
                              bool pressed,
                              sounds::OpenALSound* mineHitSound)
      -> std::vector<HitEvent> = 0;

    virtual auto release(std::span<Note> notes,
                         int column,
                         std::chrono::nanoseconds hitOffset) -> HitEvent = 0;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSHITRULES_H
