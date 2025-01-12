//
// Created by bobini on 21.06.23.
//

#include <algorithm>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::
BmsGameReferee(
  std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
             charts::gameplay_models::BmsNotesData::columnNumber> visibleNotes,
  std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
             charts::gameplay_models::BmsNotesData::columnNumber>
    invisibleNotes,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>
    bgmNotes,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
    bpmChanges,
  sounds::OpenALSound* mineHitSound,
  BmsScore* score,
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
        for (auto note : visibleNotes[i]) {
            addVisibleNote(i, note);
        }
        for (auto note : invisibleNotes[i]) {
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
        auto penalty = -note.sound / 2;
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
    enum Type
    {
        Miss,
        LnEndSkip,
        LnEndMiss,
        LnEndHit,
        Mine
    };
    auto events = QVector<std::pair<Type, std::variant<HitEvent, MineHit>>>{};
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
                events.append(
                  { Type::LnEndMiss,
                    HitEvent{
                      columnIndex, noteIndex, noteTime.count(), points } });
            } else {
                events.append(
                  { Miss,
                    HitEvent{
                      columnIndex, noteIndex, noteTime.count(), points } });
                if (lnEndSkip) {
                    events.append({ Type::LnEndSkip,
                                    HitEvent{ columnIndex,
                                              noteIndex + 1,
                                              noteTime.count(),
                                              *lnEndSkip } });
                }
            }
        }
        for (auto [points, noteIndex] : newMisses.second) {
            auto noteTime = std::visit([](auto& note) { return note.time; },
                                       visibleNotes[columnIndex][noteIndex]);
            events.append(
              { Type::LnEndHit,
                HitEvent{ columnIndex, noteIndex, noteTime.count(), points } });
        }
        if (pressedState[columnIndex]) {
            auto res = hitRules->mineHit(visibleNotes[columnIndex],
                                         currentVisibleNotes[columnIndex],
                                         offsetFromStart);
            for (auto [offset, noteIndex, penalty] : res) {
                events.append({ Type::Mine,
                                MineHit{ offsetFromStart.count(),
                                         offset.count(),
                                         penalty,
                                         columnIndex,
                                         noteIndex } });
            }
        }
    }
    std::sort(
      events.begin(), events.end(), [](const auto& left, const auto& right) {
          auto getOffset = [](const auto& event) {
              return event.getHitOffset();
          };
          return std::visit(getOffset, left.second) <
                 std::visit(getOffset, right.second);
      });
    for (const auto& event : events) {
        if (event.first == Type::Mine) {
            const auto& mineHit = std::get<MineHit>(event.second);
            score->addMineHit(mineHit);
            if (mineHitSound != nullptr) {
                mineHitSound->play();
            }
            continue;
        }
        const auto& hitEvent = std::get<HitEvent>(event.second);
        if (event.first == Type::Miss) {
            auto columnIndex = hitEvent.getColumn();
            auto noteIndex = hitEvent.getNoteIndex();
            assignLastKeysound(columnIndex,
                               visibleNotes[columnIndex][noteIndex]);

            score->addMiss(hitEvent);
        } else if (event.first == Type::LnEndSkip) {
            score->addLnEndSkip(hitEvent);
        } else if (event.first == Type::LnEndSkip) {
            score->addLnEndMiss(hitEvent);
        } else if (event.first == Type::LnEndHit) {
            score->addLnEndHit(hitEvent);
        }
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
