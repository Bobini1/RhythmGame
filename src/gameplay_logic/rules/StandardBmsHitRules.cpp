//
// Created by bobini on 22.06.23.
//

#include "StandardBmsHitRules.h"
auto
gameplay_logic::rules::StandardBmsHitRules::visibleNoteHit(
  std::span<NoteType>& notes,
  std::chrono::nanoseconds hitOffset) -> std::optional<HitResult>
{
    auto emptyPoor = std::optional<HitResult>{};
    for (auto iter = notes.begin(); iter != notes.end(); iter++) {
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
                       iter } };
        }
        emptyPoor = { { BmsPoints(hitValueFactory(hitOffset - noteTime),
                                  result,
                                  (hitOffset - noteTime).count(),
                                  /*noteRemoved=*/false),
                        iter } };
    }
    return emptyPoor;
}
auto
gameplay_logic::rules::StandardBmsHitRules::getMisses(
  std::span<NoteType> notes,
  std::chrono::nanoseconds offsetFromStart)
  -> std::pair<std::vector<MissData>, int>
{
    auto misses = std::vector<MissData>{};
    auto count = 0;
    auto upperBound = timingWindows.rbegin()->first.upper();
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            count++;
            continue;
        }
        if (offsetFromStart >= noteTime + upperBound) {
            misses.emplace_back(noteTime,
                                BmsPoints(hitValueFactory(upperBound),
                                          Judgement::Poor,
                                          (noteTime + upperBound).count(),
                                          /*noteRemoved=*/true),
                                notes.begin() + count);
            count++;
        } else {
            break;
        }
    }
    return { std::move(misses), count };
}
auto
gameplay_logic::rules::StandardBmsHitRules::invisibleNoteHit(
  std::span<NoteType>& notes,
  std::chrono::nanoseconds hitOffset) -> bool
{
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
auto
gameplay_logic::rules::StandardBmsHitRules::skipInvisible(
  std::span<NoteType> notes,
  std::chrono::nanoseconds offsetFromStart) -> int
{
    auto count = 0;
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            count++;
            continue;
        }
        if (noteTime <=
            offsetFromStart - timingWindows.begin()->first.lower()) {
            count++;
        } else {
            break;
        }
    }
    return count;
}
gameplay_logic::rules::StandardBmsHitRules::StandardBmsHitRules(
  gameplay_logic::rules::TimingWindows timingWindows,
  std::function<double(std::chrono::nanoseconds)> hitValueFactory)
  : timingWindows(std::move(timingWindows))
  , hitValueFactory(std::move(hitValueFactory))
{
}
