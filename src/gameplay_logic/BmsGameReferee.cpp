//
// Created by bobini on 21.06.23.
//

#include <ranges>
#include <algorithm>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  const charts::gameplay_models::BmsNotesData& notesData,
  BmsScore* score,
  std::unordered_map<std::string, sounds::OpenALSound>& sounds,
  gameplay_logic::BmsRules rules)
  : rules(rules)
  , score(score)
  , bpmChanges(notesData.bpmChanges)
{
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        for (const auto& note : notesData.visibleNotes[i]) {
            auto soundId = note.sound;
            if (auto sound = sounds.find(soundId); sound != sounds.end()) {
                visibleNotes[i].emplace_back(&sound->second,
                                             note.time.timestamp);
            } else {
                // we still want to be able to hit those notes, even if they
                // don't make a sound
                visibleNotes[i].emplace_back(nullptr, note.time.timestamp);
            }
        }
        currentVisibleNotes[i] = visibleNotes[i];
        for (const auto& note : notesData.invisibleNotes[i]) {
            auto soundId = note.sound;
            if (auto sound = sounds.find(soundId); sound != sounds.end()) {
                invisibleNotes[i].emplace_back(&sound->second,
                                               note.time.timestamp);
            }
        }
        currentInvisibleNotes[i] = invisibleNotes[i];
    }
    for (const auto& bgmNote : notesData.bgmNotes) {
        auto soundId = bgmNote.second;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            bgms.emplace_back(bgmNote.first.timestamp, &sound->second);
        }
    }
    currentBgms = bgms;
    currentBpmChanges = bpmChanges;
}
auto
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart)
  -> Position
{
    for (auto columnIndex = 0; columnIndex < currentVisibleNotes.size();
         columnIndex++) {
        auto& column = currentVisibleNotes[columnIndex];
        auto [newMisses, skipCount] = rules.getMisses(column, offsetFromStart);
        column = column.subspan(skipCount);
        for (auto miss : newMisses) {
            score->addMiss({ miss.count(), columnIndex });
        }
    }
    for (auto& column : currentInvisibleNotes) {
        auto skipped = rules.skipInvisible(column, offsetFromStart);
        column = column.subspan(skipped);
    }
    for (const auto& bgm : currentBgms) {
        auto played = 0;
        if (bgm.first < offsetFromStart) {
            bgm.second->play();
            played++;
        } else {
            break;
        }
        currentBgms = currentBgms.subspan(played);
    }
    return getPosition(offsetFromStart);
}
auto
gameplay_logic::BmsGameReferee::passInput(
  std::chrono::nanoseconds offsetFromStart,
  input::BmsKey key) -> std::optional<int>
{
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return std::nullopt;
    }
    auto& column = currentVisibleNotes[columnIndex];
    auto& invisibleColumn = currentInvisibleNotes[columnIndex];
    auto res = rules.visibleNoteHit(column, offsetFromStart);
    if (!res) {
        if (!rules.invisibleNoteHit(invisibleColumn, offsetFromStart)) {
            playLastKeysound(columnIndex, offsetFromStart);
        }
        score->addTap(
          { columnIndex, std::nullopt, offsetFromStart.count(), std::nullopt });
        return std::nullopt;
    }
    auto [points, iter] = *res;
    score->addTap(
      { columnIndex, iter - column.begin(), offsetFromStart.count(), points });
    return iter - column.begin();
}
auto
gameplay_logic::BmsGameReferee::isOver() const -> bool
{
    if (!bgms.empty()) {
        return false;
    }
    return !std::ranges::any_of(
      currentVisibleNotes, [](const auto& column) { return !column.empty(); });
}
auto
gameplay_logic::BmsGameReferee::getPosition(
  std::chrono::nanoseconds offsetFromStart) -> Position
{
    // find the last bpm change that happened before the current time
    auto bpmChange = std::ranges::find_if(
      currentBpmChanges, [offsetFromStart](const auto& bpmChange) {
          return bpmChange.first.timestamp >= offsetFromStart;
      });
    bpmChange--;
    auto bpm = bpmChange->second;
    auto bpmChangeTime = bpmChange->first.timestamp;
    auto bpmChangePosition = bpmChange->first.position;
    auto bpmChangeOffset = offsetFromStart - bpmChangeTime;
    auto bpmChangeOffsetSeconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        bpmChangeOffset);
    auto bpmChangeOffsetBeats = bpmChangeOffsetSeconds.count() * bpm / 60.0;
    // update the current bpm changes
    currentBpmChanges =
      currentBpmChanges.subspan(bpmChange - currentBpmChanges.begin());
    return bpmChangePosition + bpmChangeOffsetBeats;
}
void
gameplay_logic::BmsGameReferee::playLastKeysound(
  int index,
  std::chrono::nanoseconds offsetFromStart)
{
    using namespace std::chrono_literals;
    auto& visibleColumn = currentVisibleNotes[index];
    auto& invisibleColumn = currentInvisibleNotes[index];
    auto lastNote = std::ranges::find_if(
      visibleColumn, [offsetFromStart](const BmsRules::NoteType& note) {
          return note.time > offsetFromStart - 135ms;
      });
    auto lastInvisibleNote = std::ranges::find_if(
      invisibleColumn, [offsetFromStart](const BmsRules::NoteType& note) {
          return note.time > offsetFromStart - 135ms;
      });
    if (lastNote == visibleColumn.begin() &&
        lastInvisibleNote == invisibleColumn.begin()) {
        if (lastNote != visibleColumn.end()) {
            if (lastNote->sound != nullptr) {
                lastNote->sound->play();
            }
        } else if (lastInvisibleNote != invisibleColumn.end()) {
            lastInvisibleNote->sound->play();
        }
        return;
    }
    if (lastNote == visibleColumn.begin()) {
        lastInvisibleNote--;
        lastInvisibleNote->sound->play();
        return;
    }
    if (lastInvisibleNote == invisibleColumn.begin()) {
        do {
            lastNote--;
        } while (lastNote->sound == nullptr &&
                 lastNote != visibleColumn.begin());
        if (lastNote->sound != nullptr) {
            lastNote->sound->play();
        }
        return;
    }

    do {
        lastNote--;
    } while (lastNote->sound == nullptr && lastNote != visibleColumn.begin());
    lastInvisibleNote--;
    if (lastNote->time > lastInvisibleNote->time &&
        lastNote->sound != nullptr) {
        lastNote->sound->play();
    } else {
        lastInvisibleNote->sound->play();
    }
}
