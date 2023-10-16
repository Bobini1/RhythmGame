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
    std::fill(firstNoteWithSound.begin(), firstNoteWithSound.end(), -1);
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
        if (firstNoteWithSound[i] == -1) {
            firstNoteWithSound[i] = visibleNotes[i].size() - 1;
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
        if (firstNoteWithSound[i] == -1) {
            firstNoteWithSound[i] = visibleNotes[i].size() - 1;
        }
    } else if (note.noteType ==
               charts::gameplay_models::BmsNotesData::NoteType::LongNoteEnd) {
        visibleNotes[i].emplace_back(
          rules::BmsHitRules::LnEnd{ note.time.timestamp });
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
playSound(const gameplay_logic::rules::BmsHitRules::NoteType note)
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
    }
}

auto
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart,
                                       bool lastUpdate) -> Position
{
    auto misses = QVector<HitEvent>{};
    auto lnEndSkips = QVector<HitEvent>{};
    auto mineHits = QVector<MineHit>{};
    for (auto columnIndex = 0; columnIndex < currentVisibleNotes.size();
         columnIndex++) {
        auto column = visibleNotes[columnIndex];
        auto newMisses = hitRules->getMisses(
          column, currentVisibleNotes[columnIndex], offsetFromStart);
        for (auto [points, noteIndex, lnEndSkip] : newMisses) {
            auto noteTime = std::visit([](auto& note) { return note.time; },
                                       visibleNotes[columnIndex][noteIndex]);
            misses.append({ columnIndex, noteIndex, noteTime.count(), points });
            if (lnEndSkip) {
                lnEndSkips.append(
                  { columnIndex, noteIndex + 1, noteTime.count(), *lnEndSkip });
            }
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
    for (auto columnIndex = 0; columnIndex < currentInvisibleNotes.size();
         columnIndex++) {
        hitRules->skipInvisible(invisibleNotes[columnIndex],
                                currentInvisibleNotes[columnIndex],
                                offsetFromStart);
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
    if (!res) {
        if (!hitRules->invisibleNoteHit(invisibleNotes[columnIndex],
                                        currentInvisibleNotes[columnIndex],
                                        offsetFromStart)) {
            playLastKeysound(columnIndex, offsetFromStart);
        }
        invisibleNotes[columnIndex][res->noteIndex].sound->play();
        score->addEmptyHit(
          { columnIndex, std::nullopt, offsetFromStart.count(), std::nullopt });
        return;
    }
    playSound(visibleNotes[columnIndex][res->noteIndex]);
    auto [points, noteIndex] = *res;
    if (std::holds_alternative<rules::BmsHitRules::LnBegin>(
          visibleNotes[columnIndex][noteIndex])) {
        score->addNoteHit(
          { columnIndex, noteIndex, offsetFromStart.count(), points });
    } else if (std::holds_alternative<rules::BmsHitRules::Note>(
                 visibleNotes[columnIndex][noteIndex])) {
        score->addNoteHit(
          { columnIndex, noteIndex, offsetFromStart.count(), points });
    }
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
gameplay_logic::BmsGameReferee::playLastKeysound(
  int index,
  std::chrono::nanoseconds offsetFromStart)
{
    using namespace std::chrono_literals;
    auto visibleColumn =
      std::span(visibleNotes[index]).subspan(currentVisibleNotes[index]);
    auto invisibleColumn =
      std::span(invisibleNotes[index]).subspan(currentInvisibleNotes[index]);
    auto lastNote = std::ranges::find_if(
      visibleColumn,
      [offsetFromStart](const rules::BmsHitRules::NoteType& note) {
          return std::visit(
            [offsetFromStart](const auto& note) {
                return note.time > offsetFromStart - 135ms;
            },
            note);
      });
    auto firstNoteWithSoundIndex = firstNoteWithSound[index];

    auto lastInvisibleNote = std::ranges::find_if(
      invisibleColumn, [offsetFromStart](const rules::BmsHitRules::Note& note) {
          return note.time > offsetFromStart - 135ms;
      });
    if (firstNoteWithSoundIndex != -1 &&
        lastNote <= (visibleColumn.begin() + firstNoteWithSoundIndex) &&
        lastInvisibleNote == invisibleColumn.begin()) {
        lastNote = visibleColumn.begin() + firstNoteWithSoundIndex;
        if (std::holds_alternative<rules::BmsHitRules::LnBegin>(*lastNote)) {
            auto lnBegin = std::get<rules::BmsHitRules::LnBegin>(*lastNote);
            if (lnBegin.sound != nullptr) {
                lnBegin.sound->play();
            }
        } else if (std::holds_alternative<rules::BmsHitRules::Note>(
                     *lastNote)) {
            auto note = std::get<rules::BmsHitRules::Note>(*lastNote);
            if (note.sound != nullptr) {
                note.sound->play();
            }
        }
        return;
    }
    if (firstNoteWithSoundIndex == -1 ||
        lastNote <= (visibleColumn.begin() + firstNoteWithSoundIndex)) {
        lastInvisibleNote--;
        lastInvisibleNote->sound->play();
        return;
    }
    if (lastInvisibleNote == invisibleColumn.begin()) {
        do {
            lastNote--;
        } while (std::holds_alternative<rules::BmsHitRules::LnEnd>(*lastNote) ||
                 std::holds_alternative<rules::BmsHitRules::Mine>(*lastNote));
        if (std::holds_alternative<rules::BmsHitRules::LnBegin>(*lastNote)) {
            auto lnBegin = std::get<rules::BmsHitRules::LnBegin>(*lastNote);
            if (lnBegin.sound != nullptr) {
                lnBegin.sound->play();
            }
        } else if (std::holds_alternative<rules::BmsHitRules::Note>(
                     *lastNote)) {
            auto note = std::get<rules::BmsHitRules::Note>(*lastNote);
            if (note.sound != nullptr) {
                note.sound->play();
            }
        }
        return;
    }

    do {
        lastNote--;
    } while (std::holds_alternative<rules::BmsHitRules::LnEnd>(*lastNote) ||
             std::holds_alternative<rules::BmsHitRules::Mine>(*lastNote));
    lastInvisibleNote--;
    if (std::visit([](const auto& note) { return note.time; }, *lastNote) >
        lastInvisibleNote->time) {
        if (std::holds_alternative<rules::BmsHitRules::LnBegin>(*lastNote)) {
            auto lnBegin = std::get<rules::BmsHitRules::LnBegin>(*lastNote);
            if (lnBegin.sound != nullptr) {
                lnBegin.sound->play();
            }
        } else if (std::holds_alternative<rules::BmsHitRules::Note>(
                     *lastNote)) {
            auto note = std::get<rules::BmsHitRules::Note>(*lastNote);
            if (note.sound != nullptr) {
                note.sound->play();
            }
        }
    } else {
        lastInvisibleNote->sound->play();
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
        score->addLnEndMiss(
          { columnIndex, noteIndex, offsetFromStart.count(), points });
    } else {
        score->addLnEndHit(
          { columnIndex, noteIndex, offsetFromStart.count(), points });
    }
}