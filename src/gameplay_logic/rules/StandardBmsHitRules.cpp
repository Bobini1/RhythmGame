//
// Created by bobini on 22.06.23.
//

#include "StandardBmsHitRules.h"
auto
gameplay_logic::rules::StandardBmsHitRules::visibleNoteHit(
  std::span<NoteType> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds hitOffset) -> std::optional<HitResult>
{
    auto emptyPoor = std::optional<HitResult>{};
    notes = notes.subspan(currentNoteIndex);
    for (auto iter = notes.begin(); iter < notes.end(); iter++) {
        auto& [sound, noteTime, hit] = *iter;
        if (hit) {
            continue;
        }
        if (hitOffset <= noteTime + timingWindows.begin()->first.lower()) {
            continue;
        }
        if (hitOffset >= noteTime + timingWindows.rbegin()->first.upper()) {
            return emptyPoor;
        };
        auto result = timingWindows.find(hitOffset - noteTime)->second;
        if (result != Judgement::EmptyPoor) {
            hit = true;
            if (sound != nullptr) {
                sound->play();
            }
            return { { BmsPoints(hitValueFactory(hitOffset - noteTime),
                                 result,
                                 (hitOffset - noteTime).count(),
                                 /*noteRemoved=*/true),
                       static_cast<int>(iter - notes.begin() +
                                        currentNoteIndex) } };
        }
        emptyPoor = { { BmsPoints(hitValueFactory(hitOffset - noteTime),
                                  result,
                                  (hitOffset - noteTime).count(),
                                  /*noteRemoved=*/false),
                        static_cast<int>(iter - notes.begin() +
                                         currentNoteIndex) } };
    }
    return emptyPoor;
}
auto
gameplay_logic::rules::StandardBmsHitRules::getMisses(
  std::span<NoteType> notes,
  int& currentNoteIndex,
  std::chrono::nanoseconds offsetFromStart) -> std::vector<MissData>
{
    auto misses = std::vector<MissData>{};
    notes = notes.subspan(currentNoteIndex);
    auto upperBound = timingWindows.rbegin()->first.upper();
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            currentNoteIndex++;
            continue;
        }
        if (offsetFromStart >= noteTime + upperBound) {
            misses.emplace_back(noteTime,
                                BmsPoints(hitValueFactory(upperBound),
                                          Judgement::Poor,
                                          (noteTime + upperBound).count(),
                                          /*noteRemoved=*/true),
                                currentNoteIndex);
            currentNoteIndex++;
        } else {
            break;
        }
    }
    return misses;
}
auto
gameplay_logic::rules::StandardBmsHitRules::invisibleNoteHit(
  std::span<NoteType> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds hitOffset) -> bool
{
    notes = notes.subspan(currentNoteIndex);
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            continue;
        }
        if (hitOffset <= noteTime + timingWindows.begin()->first.lower()) {
            continue;
        }
        if (hitOffset >= noteTime + timingWindows.rbegin()->first.upper()) {
            return false;
        }
        hit = true;
        sound->play();
        return true;
    }
    return false;
}
void
gameplay_logic::rules::StandardBmsHitRules::skipInvisible(
  std::span<NoteType> notes,
  int& currentNoteIndex,
  std::chrono::nanoseconds offsetFromStart)
{
    notes = notes.subspan(currentNoteIndex);
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            currentNoteIndex++;
            continue;
        }
        if (noteTime <=
            offsetFromStart - timingWindows.begin()->first.lower()) {
            currentNoteIndex++;
        } else {
            break;
        }
    }
}
gameplay_logic::rules::StandardBmsHitRules::StandardBmsHitRules(
  gameplay_logic::rules::TimingWindows timingWindows,
  std::function<double(std::chrono::nanoseconds)> hitValueFactory)
  : timingWindows(std::move(timingWindows))
  , hitValueFactory(std::move(hitValueFactory))
{
}
