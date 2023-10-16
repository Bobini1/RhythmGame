//
// Created by bobini on 22.06.23.
//

#include "StandardBmsHitRules.h"

#include <ranges>
auto
gameplay_logic::rules::StandardBmsHitRules::visibleNoteHit(
  std::span<NoteType> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds hitOffset) -> std::optional<HitResult>
{
    auto emptyPoor = std::optional<HitResult>{};
    notes = notes.subspan(currentNoteIndex);
    for (auto iter = notes.begin(); iter < notes.end(); iter++) {
        auto& hit =
          std::visit([](auto& note) -> bool& { return note.hit; }, *iter);
        if (hit) {
            continue;
        }
        if (std::holds_alternative<rules::BmsHitRules::Mine>(*iter) ||
            std::holds_alternative<rules::BmsHitRules::LnEnd>(*iter)) {
            continue;
        }
        auto noteTime = std::visit([](auto& note) { return note.time; }, *iter);
        if (hitOffset <= noteTime + timingWindows.begin()->first.lower()) {
            continue;
        }
        if (hitOffset >= noteTime + timingWindows.rbegin()->first.upper()) {
            return emptyPoor;
        };
        auto result = timingWindows.find(hitOffset - noteTime)->second;
        if (result != Judgement::EmptyPoor) {
            hit = true;
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
                            /*noteRemoved=*/true);
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
    return misses;
}
auto
gameplay_logic::rules::StandardBmsHitRules::invisibleNoteHit(
  std::span<Note> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds hitOffset) -> std::optional<int>
{
    notes = notes.subspan(currentNoteIndex);
    for (auto& [sound, noteTime, hit] : notes) {
        if (hit) {
            currentNoteIndex++;
            continue;
        }
        if (hitOffset <= noteTime + timingWindows.begin()->first.lower()) {
            currentNoteIndex++;
            continue;
        }
        if (hitOffset >= noteTime + timingWindows.rbegin()->first.upper()) {
            return std::nullopt;
        }
        hit = true;
        return currentNoteIndex;
    }
    return std::nullopt;
}
void
gameplay_logic::rules::StandardBmsHitRules::skipInvisible(
  std::span<Note> notes,
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
auto
gameplay_logic::rules::StandardBmsHitRules::mineHit(
  std::span<NoteType> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds offsetFromStart) -> std::vector<MineHitData>
{
    auto mineHits = std::vector<MineHitData>{};
    notes = notes.subspan(currentNoteIndex);
    auto windowLow = [&] {
        for (auto& [window, judgement] : timingWindows) {
            if (judgement != Judgement::EmptyPoor) {
                return window.lower();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    auto windowHigh = [&] {
        for (auto& timingWindow : std::ranges::reverse_view(timingWindows)) {
            if (timingWindow.second != Judgement::EmptyPoor) {
                return timingWindow.first.upper();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    for (auto iter = notes.begin(); iter < notes.end(); iter++) {
        auto& hit =
          std::visit([](auto& note) -> bool& { return note.hit; }, *iter);
        if (hit) {
            continue;
        }
        if (!std::holds_alternative<rules::BmsHitRules::Mine>(*iter)) {
            continue;
        }
        auto& mine = std::get<rules::BmsHitRules::Mine>(*iter);
        auto noteTime = mine.time;
        auto hitOffset = offsetFromStart - noteTime;
        if (hitOffset <= windowLow) {
            continue;
        }
        if (hitOffset >= windowHigh) {
            return mineHits;
        }
        hit = true;
        mineHits.emplace_back(MineHitData{
          offsetFromStart - noteTime,
          static_cast<int>(iter - notes.begin() + currentNoteIndex),
          mine.penalty });
    }
    return mineHits;
}
auto
gameplay_logic::rules::StandardBmsHitRules::lnReleaseHit(
  std::span<NoteType> notes,
  int currentNoteIndex,
  std::chrono::nanoseconds hitOffset) -> std::optional<HitResult>
{
    notes = notes.subspan(currentNoteIndex);
    auto windowLow = [&] {
        for (auto& [window, judgement] : timingWindows) {
            if (judgement != Judgement::EmptyPoor) {
                return window.lower();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    auto windowHigh = [&] {
        for (auto& timingWindow : std::ranges::reverse_view(timingWindows)) {
            if (timingWindow.second != Judgement::EmptyPoor) {
                return timingWindow.first.upper();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    for (auto iter = notes.begin(); iter < notes.end(); iter++) {
        auto& hit =
          std::visit([](auto& note) -> bool& { return note.hit; }, *iter);
        if (hit) {
            continue;
        }
        if (!std::holds_alternative<rules::BmsHitRules::LnEnd>(*iter)) {
            continue;
        }
        auto& lnEnd = std::get<rules::BmsHitRules::LnEnd>(*iter);
        auto noteTime = lnEnd.time;
        if (hitOffset <= noteTime + windowLow) {
            continue;
        }
        if (hitOffset >= noteTime + windowHigh) {
            return std::nullopt;
        };
        auto& lnBegin = std::get<rules::BmsHitRules::LnBegin>(*(iter - 1));
        if (!lnBegin.hit) {
            continue;
        }
        auto result = timingWindows.find(hitOffset - noteTime)->second;
        hit = true;
        return { { BmsPoints(0.0,
                             result,
                             (hitOffset - noteTime).count(),
                             /*noteRemoved=*/true),
                   static_cast<int>(iter - notes.begin() +
                                    currentNoteIndex) } };
    }
    return std::nullopt;
}
