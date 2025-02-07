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
namespace sounds {
class OpenALSound;
}
namespace gameplay_logic::rules {

class BmsHitRules
{
  public:
    struct Note
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        bool hit = false;
    };
    struct Mine
    {
        std::chrono::nanoseconds time;
        double penalty;
        bool hit = false;
    };
    struct LnBegin
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        bool hit = false;
    };
    struct LnEnd
    {
        sounds::OpenALSound* sound;
        std::chrono::nanoseconds time;
        bool hit = false;
    };
    using NoteType = std::variant<Note, Mine, LnBegin, LnEnd>;
    struct HitResult
    {
        BmsPoints points;
        int noteIndex;
    };
    struct MissData
    {
        BmsPoints points;
        int noteIndex;
        std::optional<BmsPoints> lnEndSkip;
    };
    struct MineHitData
    {
        std::chrono::nanoseconds offsetFromStart;
        int noteIndex;
        double penalty;
    };
    virtual ~BmsHitRules() = default;

    virtual auto visibleNoteHit(std::span<NoteType> notes,
                                int currentNoteIndex,
                                std::chrono::nanoseconds hitOffset)
      -> std::optional<HitResult> = 0;

    virtual auto getMisses(std::span<NoteType> notes,
                                       int& currentNoteIndex,
                                       std::chrono::nanoseconds offsetFromStart)
      -> std::vector<MissData> = 0;

    virtual auto invisibleNoteHit(std::span<Note> notes,
                                  int currentNoteIndex,
                                  std::chrono::nanoseconds hitOffset)
      -> std::optional<int> = 0;

    virtual auto mineHit(std::span<NoteType> notes,
                         int currentNoteIndex,
                         std::chrono::nanoseconds hitOffset)
      -> std::vector<MineHitData> = 0;

    virtual auto lnReleaseHit(std::span<NoteType> notes,
                              int currentNoteIndex,
                              std::chrono::nanoseconds hitOffset)
      -> std::optional<HitResult> = 0;

    virtual void skipInvisible(std::span<Note> notes,
                               int& currentNoteIndex,
                               std::chrono::nanoseconds offsetFromStart) = 0;
};

} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSHITRULES_H
