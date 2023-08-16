//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_BMSRULES_H
#define RHYTHMGAME_BMSRULES_H
#include <iterator>
#include <optional>
#include <chrono>
#include <list>
#include "BmsPoints.h"
#include "TimePoint.h"
#include "charts/gameplay_models/BmsChart.h"

namespace gameplay_logic {
class BmsRules
{
  public:
    struct NoteType
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        bool hit = false;
    };

    auto visibleNoteHit(std::span<NoteType>& notes,
                        gameplay_logic::TimePoint hitTime,
                        gameplay_logic::TimePoint chartStart)
      -> std::optional<std::pair<BmsPoints, std::span<NoteType>::iterator>>;

    auto getMisses(std::span<NoteType> notes,
                   gameplay_logic::TimePoint time,
                   gameplay_logic::TimePoint chartStart)
      -> std::vector<gameplay_logic::TimePoint>;

    void invisibleNoteHit(std::span<NoteType>& notes,
                          gameplay_logic::TimePoint hitTime,
                          gameplay_logic::TimePoint chartStart);
    auto skipInvisible(std::span<NoteType> notes,
                       gameplay_logic::TimePoint time,
                       gameplay_logic::TimePoint chartStart) -> int;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRULES_H
