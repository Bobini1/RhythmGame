//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSHITRULES_H
#define RHYTHMGAME_BMSHITRULES_H
#include <iterator>
#include <optional>
#include <chrono>
#include <list>
#include "gameplay_logic/BmsPoints.h"
#include "gameplay_logic/TimePoint.h"
#include "charts/gameplay_models/BmsNotesData.h"
namespace gameplay_logic::rules {

class BmsHitRules
{
  public:
    struct NoteType
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        bool hit = false;
    };
    struct HitResult
    {
        BmsPoints points;
        std::span<NoteType>::iterator note;
    };
    struct MissData
    {
        std::chrono::nanoseconds offsetFromStart;
        BmsPoints points;
        std::span<NoteType>::iterator note;
    };
    virtual ~BmsHitRules() = default;

    virtual auto visibleNoteHit(std::span<NoteType>& notes,
                                std::chrono::nanoseconds hitOffset)
      -> std::optional<HitResult> = 0;

    virtual auto getMisses(std::span<NoteType> notes,
                           std::chrono::nanoseconds offsetFromStart)
      -> std::pair<std::vector<MissData>, int> = 0;

    virtual auto invisibleNoteHit(std::span<NoteType>& notes,
                                  std::chrono::nanoseconds hitOffset)
      -> bool = 0;
    virtual auto skipInvisible(std::span<NoteType> notes,
                               std::chrono::nanoseconds offsetFromStart)
      -> int = 0;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSHITRULES_H