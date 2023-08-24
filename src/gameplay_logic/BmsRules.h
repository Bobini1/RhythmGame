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
#include "charts/gameplay_models/BmsNotesData.h"

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
                        std::chrono::nanoseconds hitOffset)
      -> std::optional<std::pair<BmsPoints, std::span<NoteType>::iterator>>;

    auto getMisses(std::span<NoteType> notes,
                   std::chrono::nanoseconds offsetFromStart)
      -> std::vector<std::chrono::nanoseconds>;

    void invisibleNoteHit(std::span<NoteType>& notes,
                          std::chrono::nanoseconds hitOffset);
    auto skipInvisible(std::span<NoteType> notes,
                       std::chrono::nanoseconds offsetFromStart) -> int;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRULES_H
