//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_STANDARDBMSHITRULES_H
#define RHYTHMGAME_STANDARDBMSHITRULES_H
#include <iterator>
#include <optional>
#include <chrono>
#include <list>
#include "gameplay_logic/BmsPoints.h"
#include "gameplay_logic/TimePoint.h"
#include "charts/gameplay_models/BmsNotesData.h"
#include "BmsHitRules.h"
#include "TimingWindows.h"
#include "BmsGauge.h"

namespace gameplay_logic::rules {
class StandardBmsHitRules : public BmsHitRules
{
    TimingWindows timingWindows;
    std::function<double(std::chrono::nanoseconds)> hitValueFactory;

  public:
    explicit StandardBmsHitRules(
      TimingWindows timingWindows,
      std::function<double(std::chrono::nanoseconds)> hitValueFactory);
    auto visibleNoteHit(std::span<NoteType>& notes,
                        std::chrono::nanoseconds hitOffset)
      -> std::optional<HitResult> override;

    auto getMisses(std::span<NoteType> notes,
                   std::chrono::nanoseconds offsetFromStart)
      -> std::pair<std::vector<MissData>, int> override;

    auto invisibleNoteHit(std::span<NoteType>& notes,
                          std::chrono::nanoseconds hitOffset) -> bool override;
    auto skipInvisible(std::span<NoteType> notes,
                       std::chrono::nanoseconds offsetFromStart)
      -> int override;
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_STANDARDBMSHITRULES_H
