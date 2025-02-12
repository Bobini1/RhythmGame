//
// Created by bobini on 22.06.23.
//

#include "StandardBmsHitRules.h"

#include "sounds/OpenAlSound.h"

#include <ranges>
using namespace std::chrono_literals;
auto
gameplay_logic::rules::StandardBmsHitRules::press(
  std::span<Note> notes,
  const int column,
  const std::chrono::nanoseconds hitOffset) -> HitEvent
{
    auto currentNoteIndex = currentNotes[column];
    // by default, there is no empty poor, this is an empty hit
    auto emptyPoorOrNothing = HitEvent{ column,
                                        std::nullopt,
                                        hitOffset.count(),
                                        std::nullopt,
                                        HitEvent::Action::Press,
                                        /*noteRemoved=*/false };
    for (auto& note : notes.subspan(currentNoteIndex)) {
        if (note.hit) {
            currentNoteIndex++;
            continue;
        }
        if (note.type == NoteType::LnEnd) {
            currentNoteIndex++;
            continue;
        }
        auto noteTime = note.time;
        if (hitOffset <= noteTime + timingWindows.begin()->first.lower()) {
            currentNoteIndex++;
            continue;
        }
        if (hitOffset >= noteTime + timingWindows.rbegin()->first.upper()) {
            break;
        }
        if (const auto result =
              timingWindows.find(hitOffset - noteTime)->second;
            result != Judgement::EmptyPoor) {
            note.hit = true;
            if (note.sound != nullptr) {
                note.sound->play();
            }
            return { column,
                     note.index,
                     hitOffset.count(),
                     BmsPoints{
                       hitValueFactory(hitOffset - noteTime, result),
                       result,
                       (hitOffset - noteTime).count(),
                     },
                     HitEvent::Action::Press,
                     /*noteRemoved=*/true };
        }
        emptyPoorOrNothing = HitEvent(
          column,
          note.index,
          hitOffset.count(),
          BmsPoints{
            hitValueFactory(hitOffset - noteTime, Judgement::EmptyPoor),
            Judgement::EmptyPoor,
            (hitOffset - noteTime).count(),
          },
          HitEvent::Action::Press,
          /*noteRemoved=*/false);
        currentNoteIndex++;
    }
    // find the sound to play (the last note before the hit)
    // go backwards
    auto notesToCheck = notes.subspan(0, notes.size() - currentNoteIndex);
    auto found = false;
    for (auto& note : std::ranges::reverse_view(notesToCheck)) {
        if (note.time <= hitOffset) {
            if (note.sound != nullptr) {
                note.sound->play();
            }
            found = true;
            break;
        }
    }
    if (!found) {
        // play the first sound, if one exists
        if (!notes.empty() && notes.front().sound != nullptr) {
            notes.front().sound->play();
        }
    }
    return emptyPoorOrNothing;
}
auto
gameplay_logic::rules::StandardBmsHitRules::processMisses(
  std::span<Note> notes,
  int column,
  const std::chrono::nanoseconds offsetFromStart) -> std::vector<HitEvent>
{
    auto& currentNoteIndex = currentNotes[column];
    auto events = std::vector<HitEvent>{};
    notes = notes.subspan(currentNoteIndex);
    const auto upperBound = timingWindows.rbegin()->first.upper();
    for (auto iter = notes.begin(); iter < notes.end(); ++iter) {
        auto& hit = iter->hit;
        if (hit) {
            currentNoteIndex++;
            continue;
        }
        auto noteTime = iter->time;
        if (offsetFromStart < noteTime + upperBound) {
            break;
        }
        if (iter->type == NoteType::LnEnd) {
            if (iter->time <= offsetFromStart) {
                events.emplace_back(column,
                                    iter->index,
                                    noteTime.count(),
                                    BmsPoints{ 0.0, Judgement::LnEndMiss, 0 },
                                    HitEvent::Action::None,
                                    /*noteRemoved=*/false);
            }
        } else if (iter->type == NoteType::LnBegin ||
                   iter->type == NoteType::Normal) {
            events.emplace_back(
              column,
              iter->index,
              (noteTime + upperBound).count(),
              BmsPoints{ hitValueFactory(upperBound, Judgement::Poor),
                         Judgement::Poor,
                         (offsetFromStart - noteTime).count() },
              HitEvent::Action::None,
              /*noteRemoved=*/true);
            hit = true;
        }
        // register a skip event for the end of the long note
        if (iter->type == NoteType::LnBegin) {
            if (iter->sound != nullptr) {
                iter->sound->stop();
            }
            const auto nextNote = std::next(iter);
            auto nextNoteTime = nextNote->time;
            events.emplace_back(
              column,
              nextNote->index,
              (noteTime + upperBound).count(),
              BmsPoints{ hitValueFactory(upperBound, Judgement::LnEndSkip),
                         Judgement::LnEndSkip,
                         (noteTime + upperBound - nextNoteTime).count() },
              HitEvent::Action::None,
              /*noteRemoved=*/true);
            nextNote->hit = true;
            currentNoteIndex++;
        }
        currentNoteIndex++;
    }
    return events;
}
gameplay_logic::rules::StandardBmsHitRules::StandardBmsHitRules(
  TimingWindows timingWindows,
  std::function<double(std::chrono::nanoseconds, Judgement judgement)>
    hitValueFactory)
  : timingWindows(std::move(timingWindows))
  , hitValueFactory(std::move(hitValueFactory))
{
}
auto
gameplay_logic::rules::StandardBmsHitRules::processMines(
  std::span<Mine> mines,
  int column,
  std::chrono::nanoseconds offsetFromStart,
  bool pressed,
  sounds::OpenALSound* mineHitSound) -> std::vector<HitEvent>
{
    auto& currentMineIndex = currentMines[column];
    auto mineHits = std::vector<HitEvent>{};
    mines = mines.subspan(currentMineIndex);
    auto playSound = false;
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
    for (auto iter = mines.begin(); iter < mines.end(); ++iter) {
        if (iter->hit) {
            currentMineIndex++;
            continue;
        }
        auto& mine = *iter;
        auto noteTime = mine.time;
        auto hitOffset = offsetFromStart - noteTime;
        if (hitOffset <= windowLow) {
            iter->hit = true;
            mineHits.emplace_back(column,
                                  mine.index,
                                  noteTime.count(),
                                  BmsPoints{ 0.0,
                                             Judgement::MineAvoided,
                                             (noteTime + windowHigh).count() },
                                  HitEvent::Action::None,
                                  /*noteRemoved=*/true);
            currentMineIndex++;
            continue;
        }
        if (hitOffset >= windowHigh) {
            break;
        }
        if (!pressed) {
            break;
        }
        iter->hit = true;
        playSound = true;
        mineHits.emplace_back(
          column,
          mine.index,
          offsetFromStart.count(),
          BmsPoints{ mine.penalty, Judgement::MineHit, hitOffset.count() },
          HitEvent::Action::None,
          /*noteRemoved=*/true);
        currentMineIndex++;
    }
    if (playSound && mineHitSound != nullptr) {
        mineHitSound->play();
    }
    return mineHits;
}
auto
gameplay_logic::rules::StandardBmsHitRules::release(
  std::span<Note> notes,
  const int column,
  const std::chrono::nanoseconds hitOffset) -> HitEvent
{
    const auto currentNoteIndex = currentNotes[column];
    const auto windowLow = [&] {
        for (const auto& [window, judgement] : timingWindows) {
            if (judgement != Judgement::EmptyPoor) {
                return window.lower();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    const auto windowHigh = [&] {
        for (const auto& timingWindow :
             std::ranges::reverse_view(timingWindows)) {
            if (timingWindow.second != Judgement::EmptyPoor) {
                return timingWindow.first.upper();
            }
        }
        return std::chrono::nanoseconds{ 0 };
    }();
    for (auto iter = notes.begin() + currentNoteIndex; iter < notes.end();
         ++iter) {
        if (iter->hit) {
            continue;
        }
        if (iter->type != NoteType::LnEnd) {
            return { column,
                     std::nullopt,
                     hitOffset.count(),
                     std::nullopt,
                     HitEvent::Action::Release,
                     /*noteRemoved=*/false };
        }
        auto noteTime = iter->time;
        auto& lnBegin = *(iter - 1);
        if (!lnBegin.hit) {
            continue;
        }
        // dropped too early
        if (hitOffset <= noteTime + windowLow) {
            iter->hit = true;
            return { column,
                     iter->index,
                     noteTime.count(),
                     BmsPoints{ hitValueFactory(hitOffset - noteTime,
                                                Judgement::LnEndMiss),
                                Judgement::LnEndMiss,
                                (hitOffset - noteTime).count() },
                     HitEvent::Action::Release,
                     /*noteRemoved=*/true };
        }
        // dropped too late (handled in processMisses)
        if (hitOffset >= noteTime + windowHigh) {
            continue;
        }
        const auto result = timingWindows.find(hitOffset - noteTime)->second;
        iter->hit = true;
        return { column,
                 iter->index,
                 noteTime.count(),
                 BmsPoints{ 0.0, result, (hitOffset - noteTime).count() },
                 HitEvent::Action::Release,
                 /*noteRemoved=*/true };
    }
    return { column,
             std::nullopt,
             hitOffset.count(),
             std::nullopt,
             HitEvent::Action::Release,
             /*noteRemoved=*/false };
}
