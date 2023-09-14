//
// Created by bobini on 21.06.23.
//

#include <ranges>
#include <algorithm>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  const charts::gameplay_models::BmsNotesData& notesData,
  BmsScore* score,
  std::unordered_map<std::string, sounds::OpenALSound> sounds,
  std::unique_ptr<rules::BmsHitRules> hitRules,
  charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart)
  : bpmChanges(notesData.bpmChanges)
  , sounds(std::move(sounds))
  , timeBeforeChartStart(timeBeforeChartStart)
  , hitRules(std::move(hitRules))
  , score(score)
{
    bpmChanges[0].first -= timeBeforeChartStart;
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        for (const auto& note : notesData.visibleNotes[i]) {
            auto soundId = note.sound;
            if (auto sound = this->sounds.find(soundId);
                sound != this->sounds.end()) {
                visibleNotes[i].emplace_back(&sound->second,
                                             note.time.timestamp);
            } else {
                // we still want to be able to hit those notes, even if they
                // don't make a sound
                visibleNotes[i].emplace_back(nullptr, note.time.timestamp);
            }
        }
        for (const auto& note : notesData.invisibleNotes[i]) {
            auto soundId = note.sound;
            if (auto sound = this->sounds.find(soundId);
                sound != this->sounds.end()) {
                invisibleNotes[i].emplace_back(&sound->second,
                                               note.time.timestamp);
            }
        }
    }
    for (const auto& bgmNote : notesData.bgmNotes) {
        auto soundId = bgmNote.second;
        if (auto sound = this->sounds.find(soundId);
            sound != this->sounds.end()) {
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
    offsetFromStart -= timeBeforeChartStart.timestamp;
    auto misses = QVector<Miss>{};
    for (auto columnIndex = 0; columnIndex < currentVisibleNotes.size();
         columnIndex++) {
        auto column = visibleNotes[columnIndex];
        auto newMisses =
          hitRules->getMisses(column, currentVisibleNotes[columnIndex], offsetFromStart);
        for (auto [offset, points, noteIndex] : newMisses) {
            misses.append(
              { offset.count(),
                points,
                columnIndex, noteIndex });
        }
    }
    if (!misses.empty()) {
        score->addMisses(std::move(misses));
    }
    for (auto columnIndex = 0; columnIndex < currentInvisibleNotes.size();
         columnIndex++) {
        hitRules->skipInvisible(invisibleNotes[columnIndex], currentInvisibleNotes[columnIndex], offsetFromStart);
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
  input::BmsKey key) -> void
{
    offsetFromStart -= timeBeforeChartStart.timestamp;
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return;
    }
    auto res = hitRules->visibleNoteHit(visibleNotes[columnIndex],  currentVisibleNotes[columnIndex], offsetFromStart);
    if (!res) {
        if (!hitRules->invisibleNoteHit(invisibleNotes[columnIndex], currentInvisibleNotes[columnIndex], offsetFromStart)) {
            playLastKeysound(columnIndex, offsetFromStart);
        }
        score->addTap(
          { columnIndex, std::nullopt, offsetFromStart.count(), std::nullopt });
        return;
    }
    auto [points, noteIndex] = *res;
    score->addTap({ columnIndex,
                    noteIndex,
                    offsetFromStart.count(),
                    points });
}
auto
gameplay_logic::BmsGameReferee::isOver() const -> bool
{
    for (auto column = 0; column < visibleNotes.size(); column++)
    {
        if (visibleNotes[column].size() != currentVisibleNotes[column]) {
            return false;
        }
    }
    return true;
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
    auto visibleColumn = std::span(visibleNotes[index]).subspan(currentVisibleNotes[index]);
    auto invisibleColumn = std::span(invisibleNotes[index]).subspan(currentInvisibleNotes[index]);
    auto lastNote = std::ranges::find_if(
      visibleColumn,
      [offsetFromStart](const rules::BmsHitRules::NoteType& note) {
          return note.time > offsetFromStart - 135ms;
      });
    auto lastInvisibleNote = std::ranges::find_if(
      invisibleColumn,
      [offsetFromStart](const rules::BmsHitRules::NoteType& note) {
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
        lastNote--;
        if (lastNote->sound != nullptr) {
            lastNote->sound->play();
        }
        return;
    }

    lastNote--;
    lastInvisibleNote--;
    if (lastNote->time > lastInvisibleNote->time &&
        lastNote->sound != nullptr) {
        lastNote->sound->play();
    } else {
        lastInvisibleNote->sound->play();
    }
}
