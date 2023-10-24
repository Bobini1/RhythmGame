//
// Created by bobini on 24.10.23.
//

#include "CnHitRules.h"
auto
gameplay_logic::rules::CnHitRules::getMissesAndLnEndHits(
  std::span<NoteType> notes,
  int& currentNoteIndex,
  std::chrono::nanoseconds offsetFromStart)
  -> std::pair<std::vector<MissData>, std::vector<HitResult>>
{
    auto misses = std::vector<MissData>{};
    notes = notes.subspan(currentNoteIndex);
    auto upperBound = timingWindows.rbegin()->first.upper();
    for (auto iter = notes.begin(); iter < notes.end(); iter++) {
        auto& hit =
          std::visit([](auto& note) -> bool& { return note.hit; }, *iter);
        if (hit) {
            currentNoteIndex++;
            continue;
        }
        auto noteTime = std::visit([](auto& note) { return note.time; }, *iter);
        if (offsetFromStart >= noteTime + upperBound) {
            if (std::holds_alternative<rules::BmsHitRules::Mine>(*iter)) {
                currentNoteIndex++;
                continue;
            }
            auto lnEndSkip = std::optional<BmsPoints>{};
            if (std::holds_alternative<rules::BmsHitRules::LnBegin>(*iter)) {
                auto nextNote = std::next(iter);
                auto nextNoteTime =
                  std::visit([](auto& note) { return note.time; }, *nextNote);
                lnEndSkip =
                  BmsPoints(hitValueFactory(upperBound),
                            Judgement::Poor,
                            (nextNoteTime - noteTime + upperBound).count(),
                            /*noteRemoved=*/false);
                std::visit([](auto& note) { note.hit = true; }, *nextNote);
            }
            misses.emplace_back(BmsPoints(hitValueFactory(upperBound),
                                          Judgement::Poor,
                                          upperBound.count(),
                                          /*noteRemoved=*/true),
                                currentNoteIndex,
                                lnEndSkip);
            currentNoteIndex++;
        } else {
            break;
        }
    }
    return { misses, {} };
}
