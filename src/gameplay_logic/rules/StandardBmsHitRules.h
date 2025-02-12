//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_STANDARDBMSHITRULES_H
#define RHYTHMGAME_STANDARDBMSHITRULES_H
#include <chrono>
#include "charts/gameplay_models/BmsNotesData.h"
#include "BmsHitRules.h"
#include "TimingWindows.h"
#include "BmsGauge.h"
#include "gameplay_logic/HitEvent.h"

namespace gameplay_logic::rules {
class StandardBmsHitRules : public BmsHitRules
{
  protected:
    TimingWindows timingWindows;
    std::function<double(std::chrono::nanoseconds, Judgement judgement)>
      hitValueFactory;
    std::array<int, charts::gameplay_models::BmsNotesData::columnNumber>
      currentNotes{};
    std::array<int, charts::gameplay_models::BmsNotesData::columnNumber>
      currentMines{};

  public:
    explicit StandardBmsHitRules(
      TimingWindows timingWindows,
      std::function<double(std::chrono::nanoseconds, Judgement judgement)>
        hitValueFactory);
    auto press(std::span<Note> notes,
               int column,
               std::chrono::nanoseconds hitOffset) -> HitEvent override;

    // We don't modify currentNoteIndex here, just mark mines as not hittable
    // anymore
    auto processMisses(std::span<Note> notes,
                       int column,
                       std::chrono::nanoseconds offsetFromStart)
      -> std::vector<HitEvent> override;
    // We don't modify currentNoteIndex here, just mark notes as not hittable
    // anymore
    auto processMines(std::span<Mine> mines,
                      int column,
                      std::chrono::nanoseconds offsetFromStart,
                      bool pressed,
                      sounds::OpenALSound* mineHitSound)
      -> std::vector<HitEvent> override;
    auto release(std::span<Note> notes,
                 int column,
                 std::chrono::nanoseconds hitOffset) -> HitEvent override;
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_STANDARDBMSHITRULES_H
