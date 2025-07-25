//
// Created by bobini on 21.06.23.
//

#include <algorithm>
#include <ranges>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
             charts::gameplay_models::BmsNotesData::columnNumber> notes,
  const std::vector<
    std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>& bgmNotes,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
    bpmChanges,
  sounds::OpenALSound* mineHitSound,
  BmsLiveScore* score,
  std::unordered_map<uint16_t, sounds::OpenALSound> sounds,
  std::unique_ptr<rules::BmsHitRules> hitRules)
  : bpmChanges(std::move(bpmChanges))
  , sounds(std::move(sounds))
  , hitRules(std::move(hitRules))
  , score(score)
  , mineHitSound(mineHitSound)
{
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        for (const auto& [index, note] :
             std::ranges::views::enumerate(notes[i])) {
            addNote(this->notes[i], this->mines[i], note, index);
        }
    }
    for (const auto& bgmNote : bgmNotes) {
        auto soundId = bgmNote.second;
        if (auto sound = this->sounds.find(soundId);
            sound != this->sounds.end()) {
            this->bgms.emplace_back(bgmNote.first.timestamp, &sound->second);
        }
    }
    currentBgms = bgms;
}
void
gameplay_logic::BmsGameReferee::addNote(
  decltype(notes)::value_type& column,
  decltype(mines)::value_type& minesColumn,
  const charts::gameplay_models::BmsNotesData::Note& note,
  int index)
{
    if (note.noteType ==
        charts::gameplay_models::BmsNotesData::NoteType::Normal) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(&sound->second,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::Normal,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::Normal,
                                index);
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::LongNoteBegin) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(&sound->second,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::LnBegin,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::LnBegin,
                                index);
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::LongNoteEnd) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(&sound->second,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::LnEnd,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::BmsHitRules::NoteType::LnEnd,
                                index);
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::Landmine) {
        auto penalty = -note.sound / 2;
        minesColumn.emplace_back(
          static_cast<double>(penalty), note.time.timestamp, index);
    }
}

void
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart,
                                       bool lastUpdate)
{
    auto events = std::vector<HitEvent>{};
    for (auto columnIndex = 0; columnIndex < notes.size(); columnIndex++) {
        auto& column = notes[columnIndex];
        std::ranges::copy(
          hitRules->processMisses(column, columnIndex, offsetFromStart),
          std::back_inserter(events));
        std::ranges::copy(hitRules->processMines(mines[columnIndex],
                                                 columnIndex,
                                                 offsetFromStart,
                                                 pressedState[columnIndex],
                                                 mineHitSound),
                          std::back_inserter(events));
    }
    std::ranges::sort(events, [](const auto& left, const auto& right) {
        return left.getHitOffset() < right.getHitOffset();
    });
    for (const auto& event : events) {
        score->addHit(event, !lastUpdate);
    }
    if (lastUpdate) {
        currentBgms = {};
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
}
auto
gameplay_logic::BmsGameReferee::passPressed(
  std::chrono::nanoseconds offsetFromStart,
  input::BmsKey key) -> void
{
    if (key == input::BmsKey::Col1sDown) {
        key = input::BmsKey::Col1sUp;
    }
    if (key == input::BmsKey::Col2sDown) {
        key = input::BmsKey::Col2sUp;
    }
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return;
    }
    pressedState[columnIndex] = true;
    for (const auto& hit : hitRules->press(notes[columnIndex], columnIndex, offsetFromStart)) {
        score->addHit(hit);
    }
}
auto
gameplay_logic::BmsGameReferee::getPosition(
  std::chrono::nanoseconds offsetFromStart) const -> Position
{
    // find the last bpm change that happened before the current time
    auto bpmChange = std::ranges::find_if(
      bpmChanges, [offsetFromStart](const auto& bpmChange) {
          return bpmChange.first.timestamp >= offsetFromStart;
      });
    if (offsetFromStart.count() < 0) {
        bpmChange = bpmChanges.begin();
    } else {
        --bpmChange;
    }
    auto bpm = bpmChange->second;
    auto bpmChangeTime = bpmChange->first.timestamp;
    auto bpmChangePosition = bpmChange->first.position;
    auto bpmChangeOffset = offsetFromStart - bpmChangeTime;
    auto bpmChangeOffsetSeconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        bpmChangeOffset);
    auto bpmChangeOffsetBeats = bpmChangeOffsetSeconds.count() * bpm / 60.0;
    return bpmChangePosition + bpmChangeOffsetBeats;
}

auto
gameplay_logic::BmsGameReferee::passReleased(
  std::chrono::nanoseconds offsetFromStart,
  input::BmsKey key) -> void
{
    if (key == input::BmsKey::Col1sDown) {
        key = input::BmsKey::Col1sUp;
    }
    if (key == input::BmsKey::Col2sDown) {
        key = input::BmsKey::Col2sUp;
    }
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return;
    }
    pressedState[columnIndex] = false;
    score->addHit(
      hitRules->release(notes[columnIndex], columnIndex, offsetFromStart));
}