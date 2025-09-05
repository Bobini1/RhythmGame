//
// Created by bobini on 22.06.23.
//

#ifndef RHYTHMGAME_HITRULES_H
#define RHYTHMGAME_HITRULES_H
#include "charts/BmsNotesData.h"
#include "TimingWindows.h"
#include "BmsGauge.h"
#include "gameplay_logic/HitEvent.h"

#include <span>
#include <chrono>

namespace sounds {
class Sound;
}
namespace gameplay_logic::rules {
/**
 * @brief The class that determines which notes get hit or missed.
 * @details Beatoraja defines three different "Judge algorithms":
 * - "Combo get priority" (default): When multiple notes are in the hit window,
 *   the earliest note that does result in the BAD judgement is hit. Used in
 *   Lunatic Rave 2.
 * - "Score get priority": The note that is the closest to the judge line is
 *   hit. Used in Stepmania.
 * - "Bottom notes get priority": The lowest (earlier timestamp) hittable note
 *   is hit. Used in osu!mania.
 *
 * This class implements the "Combo get priority" algorithm.
 * @note Define RHYTHMGAME_BOTTOM_NOTES_GET_PRIORITY to use the "Bottom notes
 * get priority" algorithm instead.
 */
class HitRules
{
    TimingWindows timingWindows;
    std::function<double(std::chrono::nanoseconds, Judgement judgement)>
      hitValueFactory;
    std::array<int, charts::BmsNotesData::columnNumber> currentNotes{};
    std::array<int, charts::BmsNotesData::columnNumber> currentMines{};
    std::array<BmsPoints, charts::BmsNotesData::columnNumber> lnBeginPoints{};
    bool soundDisabled = false;

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
        sounds::Sound* sound;
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
    explicit HitRules(
      TimingWindows timingWindows,
      std::function<double(std::chrono::nanoseconds, Judgement judgement)>
        hitValueFactory);
    void disableSound();
    auto press(std::span<Note> notes,
               int column,
               std::chrono::nanoseconds hitOffset) -> QList<HitEvent>;

    auto processMisses(std::span<Note> notes,
                       int column,
                       std::chrono::nanoseconds offsetFromStart)
      -> std::vector<HitEvent>;

    auto processMines(std::span<Mine> mines,
                      int column,
                      std::chrono::nanoseconds offsetFromStart,
                      bool pressed,
                      sounds::Sound* mineHitSound)
      -> std::vector<HitEvent>;
    auto release(std::span<Note> notes,
                 int column,
                 std::chrono::nanoseconds hitOffset) -> HitEvent;
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_HITRULES_H
