//
// Created by bobini on 21.06.23.
//

#include <algorithm>
#include <ranges>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  std::array<std::vector<charts::BmsNotesData::Note>,
             charts::BmsNotesData::columnNumber> notes,
  const std::vector<std::pair<charts::BmsNotesData::Time, uint16_t>>& bgmNotes,
  std::vector<std::pair<charts::BmsNotesData::Time, double>> bpmChanges,
  std::shared_ptr<sounds::Sound> mineHitSound,
  BmsLiveScore* score,
  std::unordered_map<uint16_t, std::shared_ptr<sounds::Sound>> sounds,
  rules::HitRules hitRules)
  : bpmChanges(std::move(bpmChanges))
  , sounds(std::move(sounds))
  , hitRules(std::move(hitRules))
  , score(score)
  , mineHitSound(std::move(mineHitSound))
{
    for (int i = 0; i < charts::BmsNotesData::columnNumber; i++) {
        for (const auto& [index, note] :
             std::ranges::views::enumerate(notes[i])) {
            addNote(this->notes[i], this->mines[i], note, index);
        }
    }
    for (const auto& bgmNote : bgmNotes) {
        auto soundId = bgmNote.second;
        if (auto sound = this->sounds.find(soundId);
            sound != this->sounds.end()) {
            this->bgms.emplace_back(bgmNote.first.timestamp, sound->second.get());
        }
    }
    currentBgms = bgms;
}
void
gameplay_logic::BmsGameReferee::addNote(
  decltype(notes)::value_type& column,
  decltype(mines)::value_type& minesColumn,
  const charts::BmsNotesData::Note& note,
  int index)
{
    if (note.noteType == charts::BmsNotesData::NoteType::Normal) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(sound->second.get(),
                                note.time.timestamp,
                                rules::HitRules::NoteType::Normal,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::HitRules::NoteType::Normal,
                                index);
        }
    } else if (note.noteType == charts::BmsNotesData::NoteType::LongNoteBegin) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(sound->second.get(),
                                note.time.timestamp,
                                rules::HitRules::NoteType::LnBegin,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::HitRules::NoteType::LnBegin,
                                index);
        }
    } else if (note.noteType == charts::BmsNotesData::NoteType::LongNoteEnd) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            column.emplace_back(sound->second.get(),
                                note.time.timestamp,
                                rules::HitRules::NoteType::LnEnd,
                                index);
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            column.emplace_back(nullptr,
                                note.time.timestamp,
                                rules::HitRules::NoteType::LnEnd,
                                index);
        }
    } else if (note.noteType == charts::BmsNotesData::NoteType::Landmine) {
        auto penalty = -note.sound / 2;
        minesColumn.emplace_back(
          static_cast<double>(penalty), note.time.timestamp, index);
    }
}

void
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart,
                                       bool lastUpdate)
{
    if (lastUpdate) {
        hitRules.disableSound();
        currentBgms = {};
    }
    auto events = std::vector<HitEvent>{};
    for (auto columnIndex = 0; columnIndex < notes.size(); columnIndex++) {
        auto& column = notes[columnIndex];
        std::ranges::copy(
          hitRules.processMisses(column, columnIndex, offsetFromStart),
          std::back_inserter(events));
        std::ranges::copy(hitRules.processMines(mines[columnIndex],
                                                columnIndex,
                                                offsetFromStart,
                                                pressedState[columnIndex],
                                                mineHitSound.get()),
                          std::back_inserter(events));
    }
    std::ranges::sort(events, [](const auto& left, const auto& right) {
        return left.getHitOffset() < right.getHitOffset();
    });
    for (const auto& event : events) {
        score->addHit(event);
    }
    auto played = 0;
    for (const auto& bgm : currentBgms) {
        if (bgm.first < offsetFromStart) {
            bgm.second->play();
            played++;
        } else {
            break;
        }
    }
    currentBgms = currentBgms.subspan(played);
}
auto
gameplay_logic::BmsGameReferee::getBpm(
  std::chrono::nanoseconds offsetFromStart) const
  -> std::pair<charts::BmsNotesData::Time, double>
{
    auto bpmChange = std::upper_bound(
        bpmChanges.begin(), bpmChanges.end(),
        offsetFromStart,
        [](const std::chrono::nanoseconds& offset, const auto& change) {
            return offset < change.first.timestamp;
        });
    --bpmChange;
    return *bpmChange;
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
    if (columnIndex < 0 || columnIndex >= charts::BmsNotesData::columnNumber) {
        return;
    }
    if (pressedState[columnIndex]) {
        return;
    }
    pressedState[columnIndex] = true;
    for (const auto& hit :
         hitRules.press(notes[columnIndex], columnIndex, offsetFromStart)) {
        score->addHit(hit);
    }
}
auto
gameplay_logic::BmsGameReferee::getPosition(std::pair<charts::BmsNotesData::Time, double> bpmChange,
  std::chrono::nanoseconds offsetFromStart) -> Position
{
    auto bpm = bpmChange.second;
    auto bpmChangeTime = bpmChange.first.timestamp;
    auto bpmChangePosition = bpmChange.first.position;
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
    if (columnIndex < 0 || columnIndex >= charts::BmsNotesData::columnNumber) {
        return;
    }
    if (!pressedState[columnIndex]) {
        return;
    }
    pressedState[columnIndex] = false;
    score->addHit(
      hitRules.release(notes[columnIndex], columnIndex, offsetFromStart));
}