//
// Created by bobini on 21.06.23.
//

#include <ranges>
#include <algorithm>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
             charts::gameplay_models::BmsNotesData::columnNumber> visibleNotes,
  std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
             charts::gameplay_models::BmsNotesData::columnNumber>
    invisibleNotes,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time,
                        std::string>> bgmNotes,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
    bpmChanges,
  sounds::OpenALSound* mineHitSound,
  BmsScore* score,
  std::unordered_map<std::string, sounds::OpenALSound> sounds,
  std::unique_ptr<rules::BmsHitRules> hitRules)
  : bpmChanges(std::move(bpmChanges))
  , sounds(std::move(sounds))
  , hitRules(std::move(hitRules))
  , score(score)
  , mineHitSound(mineHitSound)
{
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        for (const auto& note : visibleNotes[i]) {
            addVisibleNote(i, note);
        }
        for (const auto& note : invisibleNotes[i]) {
            auto soundId = note.sound;
            if (auto sound = this->sounds.find(soundId);
                sound != this->sounds.end()) {
                this->invisibleNotes[i].emplace_back(rules::BmsHitRules::Note{
                  &sound->second, note.time.timestamp });
            }
        }
    }
    for (const auto& bgmNote : bgmNotes) {
        auto soundId = bgmNote.second;
        if (auto sound = this->sounds.find(soundId);
            sound != this->sounds.end()) {
            this->bgms.emplace_back(bgmNote.first.timestamp, &sound->second);
        }
    }
    // fill lastKeysound with the first note with sound
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        if (!this->visibleNotes[i].empty()) {
            auto& note = this->visibleNotes[i].front();
            if (std::holds_alternative<rules::BmsHitRules::Note>(note)) {
                auto& normal =
                  std::get<rules::BmsHitRules::Note>(this->visibleNotes[i][0]);
                if (normal.sound != nullptr) {
                    lastKeysound[i] = std::make_pair(normal.time, normal.sound);
                }
            } else if (std::holds_alternative<rules::BmsHitRules::LnBegin>(
                         note)) {
                auto& lnBegin = std::get<rules::BmsHitRules::LnBegin>(
                  this->visibleNotes[i][0]);
                if (lnBegin.sound != nullptr) {
                    lastKeysound[i] =
                      std::make_pair(lnBegin.time, lnBegin.sound);
                }
            }
        }
    }
    currentBgms = bgms;
    currentBpmChanges = this->bpmChanges;
}
void
gameplay_logic::BmsGameReferee::addVisibleNote(
  int i,
  const charts::gameplay_models::BmsNotesData::Note& note)
{
    if (note.noteType ==
        charts::gameplay_models::BmsNotesData::NoteType::Normal) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            visibleNotes[i].emplace_back(
              rules::BmsHitRules::Note{ &sound->second, note.time.timestamp });
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            visibleNotes[i].emplace_back(
              rules::BmsHitRules::Note{ nullptr, note.time.timestamp });
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::LongNoteBegin) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            visibleNotes[i].emplace_back(rules::BmsHitRules::LnBegin{
              &sound->second, note.time.timestamp });
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            visibleNotes[i].emplace_back(
              rules::BmsHitRules::LnBegin{ nullptr, note.time.timestamp });
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::LongNoteEnd) {
        auto soundId = note.sound;
        if (auto sound = sounds.find(soundId); sound != sounds.end()) {
            visibleNotes[i].emplace_back(
              rules::BmsHitRules::LnEnd{ &sound->second, note.time.timestamp });
        } else {
            // we still want to be able to hit those notes, even if they
            // don't make a sound
            visibleNotes[i].emplace_back(
              rules::BmsHitRules::LnEnd{ nullptr, note.time.timestamp });
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::Landmine) {
        auto idx = std::size_t{ 0 };
        auto penalty = -std::stoi(note.sound, &idx, 36) / 2;
        if (idx != note.sound.size()) {
            spdlog::warn("Invalid landmine penalty value: {}", note.sound);
            penalty = 0;
        }
        visibleNotes[i].emplace_back(rules::BmsHitRules::Mine{
          note.time.timestamp, static_cast<double>(penalty) });
    }
}

void
playSound(const gameplay_logic::rules::BmsHitRules::NoteType& note)
{
    if (std::holds_alternative<gameplay_logic::rules::BmsHitRules::Note>(
          note)) {
        auto& normal = std::get<gameplay_logic::rules::BmsHitRules::Note>(note);
        if (normal.sound != nullptr) {
            normal.sound->play();
        }
    } else if (std::holds_alternative<
                 gameplay_logic::rules::BmsHitRules::LnBegin>(note)) {
        auto& lnBegin =
          std::get<gameplay_logic::rules::BmsHitRules::LnBegin>(note);
        if (lnBegin.sound != nullptr) {
            lnBegin.sound->play();
        }
    } else if (std::holds_alternative<
                 gameplay_logic::rules::BmsHitRules::LnEnd>(note)) {
        auto& lnEnd = std::get<gameplay_logic::rules::BmsHitRules::LnEnd>(note);
        if (lnEnd.sound != nullptr) {
            lnEnd.sound->play();
        }
    }
}

auto
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart,
                                       bool lastUpdate) -> Position
{
    auto misses = QVector<HitEvent>{};
    auto lnEndSkips = QVector<HitEvent>{};
    auto lnEndMisses = QVector<HitEvent>{};
    auto lnEndHits = QVector<HitEvent>{};
    auto mineHits = QVector<MineHit>{};
    for (auto columnIndex = 0; columnIndex < currentVisibleNotes.size();
         columnIndex++) {
        auto& column = visibleNotes[columnIndex];
        auto newMisses = hitRules->getMissesAndLnEndHits(
          column, currentVisibleNotes[columnIndex], offsetFromStart);
        for (auto [points, noteIndex, lnEndSkip] : newMisses.first) {
            auto noteTime = std::visit([](auto& note) { return note.time; },
                                       visibleNotes[columnIndex][noteIndex]);
            if (std::holds_alternative<rules::BmsHitRules::LnEnd>(
                  visibleNotes[columnIndex][noteIndex])) {
                lnEndMisses.append(
                  { columnIndex, noteIndex, noteTime.count(), points });
            } else {
                misses.append(
                  { columnIndex, noteIndex, noteTime.count(), points });
                if (lnEndSkip) {
                    lnEndSkips.append({ columnIndex,
                                        noteIndex + 1,
                                        noteTime.count(),
                                        *lnEndSkip });
                }
            }
        }
        for (auto [points, noteIndex] : newMisses.second) {
            auto noteTime = std::visit([](auto& note) { return note.time; },
                                       visibleNotes[columnIndex][noteIndex]);
            lnEndHits.append(
              { columnIndex, noteIndex, noteTime.count(), points });
        }
        if (pressedState[columnIndex]) {
            auto res = hitRules->mineHit(visibleNotes[columnIndex],
                                         currentVisibleNotes[columnIndex],
                                         offsetFromStart);
            for (auto [offset, noteIndex, penalty] : res) {
                mineHits.append({ offsetFromStart.count(),
                                  offset.count(),
                                  penalty,
                                  columnIndex,
                                  noteIndex });
            }
        }
    }
    if (!misses.empty()) {
        for (const auto& miss : std::ranges::reverse_view(misses)) {
            auto columnIndex = miss.getColumn();
            auto noteIndex = miss.getNoteIndex();
            assignLastKeysound(columnIndex,
                               visibleNotes[columnIndex][noteIndex]);
        }

        score->addMisses(std::move(misses));
    }
    if (!mineHits.empty()) {
        score->addMineHits(std::move(mineHits));
        if (mineHitSound != nullptr) {
            mineHitSound->play();
        }
    }
    if (!lnEndSkips.empty()) {
        score->addLnEndSkips(std::move(lnEndSkips));
    }
    if (!lnEndMisses.empty()) {
        score->addLnEndMisses(std::move(lnEndMisses));
    }
    for (const auto& lnEndHit : lnEndHits) {
        score->addLnEndHit(lnEndHit);
    }
    for (auto columnIndex = 0; columnIndex < currentInvisibleNotes.size();
         columnIndex++) {
        auto previousInvNoteIndex = currentInvisibleNotes[columnIndex];
        hitRules->skipInvisible(invisibleNotes[columnIndex],
                                currentInvisibleNotes[columnIndex],
                                offsetFromStart);
        if (previousInvNoteIndex != currentInvisibleNotes[columnIndex]) {
            auto& note = invisibleNotes[columnIndex]
                                       [currentInvisibleNotes[columnIndex] - 1];
            assignLastKeysound(columnIndex, note);
        }
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
    return getPosition(offsetFromStart);
}
auto
gameplay_logic::BmsGameReferee::passPressed(
  std::chrono::nanoseconds offsetFromStart,
  input::BmsKey key) -> void
{
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return;
    }
    pressedState[columnIndex] = true;
    auto res = hitRules->visibleNoteHit(visibleNotes[columnIndex],
                                        currentVisibleNotes[columnIndex],
                                        offsetFromStart);
    if (!res || !res->points.getNoteRemoved()) {
        if (auto invisibleNoteIndex =
              hitRules->invisibleNoteHit(invisibleNotes[columnIndex],
                                         currentInvisibleNotes[columnIndex],
                                         offsetFromStart)) {
            invisibleNotes[columnIndex][res->noteIndex].sound->play();
            assignLastKeysound(
              columnIndex, invisibleNotes[columnIndex][*invisibleNoteIndex]);
        } else {
            playLastKeysound(columnIndex);
        }
        if (!res) {
            score->addEmptyHit({ columnIndex,
                                 std::nullopt,
                                 offsetFromStart.count(),
                                 std::nullopt });
            return;
        }
    }
    if (res->points.getNoteRemoved()) {
        playSound(visibleNotes[columnIndex][res->noteIndex]);
        assignLastKeysound(columnIndex,
                           visibleNotes[columnIndex][res->noteIndex]);
    }
    auto [points, noteIndex] = *res;
    score->addNoteHit(
      { columnIndex, noteIndex, offsetFromStart.count(), points });
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
    if (offsetFromStart.count() < 0) {
        bpmChange = currentBpmChanges.begin();
    } else {
        bpmChange--;
    }
    auto bpm = bpmChange->second;
    auto bpmChangeTime = bpmChange->first.timestamp;
    auto bpmChangePosition = bpmChange->first.position;
    auto bpmChangeOffset = offsetFromStart - bpmChangeTime;
    auto bpmChangeOffsetSeconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(
        bpmChangeOffset);
    auto bpmChangeOffsetBeats = bpmChangeOffsetSeconds.count() * bpm / 60.0;
    // scanNew the current bpm changes
    currentBpmChanges =
      currentBpmChanges.subspan(bpmChange - currentBpmChanges.begin());
    return bpmChangePosition + bpmChangeOffsetBeats;
}

void
gameplay_logic::BmsGameReferee::playLastKeysound(int index)
{
    if (lastKeysound[index]) {
        auto [time, sound] = *lastKeysound[index];
        if (sound != nullptr) {
            sound->play();
        }
    }
}
auto
gameplay_logic::BmsGameReferee::passReleased(
  std::chrono::nanoseconds offsetFromStart,
  input::BmsKey key) -> void
{
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsNotesData::columnNumber) {
        return;
    }
    pressedState[columnIndex] = false;
    auto res = hitRules->lnReleaseHit(visibleNotes[columnIndex],
                                      currentVisibleNotes[columnIndex],
                                      offsetFromStart);
    if (!res) {
        score->addEmptyRelease(
          { columnIndex, std::nullopt, offsetFromStart.count(), std::nullopt });
        return;
    }
    auto [points, noteIndex] = *res;
    auto judgement = points.getJudgement();
    if (judgement == Judgement::Poor) {
        score->addLnEndMisses(
          { { columnIndex, noteIndex, offsetFromStart.count(), points } });
        
        auto prevNoteIndex = noteIndex - 1;
        auto& prevNote = std::get<rules::BmsHitRules::LnBegin>(
          visibleNotes[columnIndex][prevNoteIndex]);
        if (prevNote.sound) {
            prevNote.sound->stop();
        }
    } else {
        score->addLnEndHit(
          { columnIndex, noteIndex, offsetFromStart.count(), points });
    }
}
void
gameplay_logic::BmsGameReferee::assignLastKeysound(
  int columnIndex,
  const gameplay_logic::rules::BmsHitRules::NoteType& note)
{
    if (std::holds_alternative<rules::BmsHitRules::Note>(note)) {
        auto& normal = std::get<rules::BmsHitRules::Note>(note);
        if (!lastKeysound[columnIndex] ||
            lastKeysound[columnIndex]->first < normal.time) {
            lastKeysound[columnIndex] =
              std::make_pair(normal.time, normal.sound);
        }
    } else if (std::holds_alternative<rules::BmsHitRules::LnBegin>(note)) {
        auto& lnBegin = std::get<rules::BmsHitRules::LnBegin>(note);
        if (!lastKeysound[columnIndex] ||
            lastKeysound[columnIndex]->first < lnBegin.time) {
            lastKeysound[columnIndex] =
              std::make_pair(lnBegin.time, lnBegin.sound);
        }
    }
}
