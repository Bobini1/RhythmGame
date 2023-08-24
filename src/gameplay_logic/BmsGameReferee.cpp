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
{
    for (int i = 0; i < charts::gameplay_models::BmsNotesData::columnNumber;
         i++) {
        std::transform(
          notesData.visibleNotes[i].begin(),
          notesData.visibleNotes[i].end(),
          std::back_inserter(visibleNotes[i]),
          [&sounds](auto& note) {
              auto soundId = note.second.sound;
              if (auto sound = sounds.find(soundId); sound != sounds.end()) {
                  return BmsRules::NoteType{ &sound->second, note.first };
              }
              return BmsRules::NoteType{ nullptr, note.first };
          });
        currentVisibleNotes[i] = visibleNotes[i];
        std::transform(
          notesData.invisibleNotes[i].begin(),
          notesData.invisibleNotes[i].end(),
          std::back_inserter(invisibleNotes[i]),
          [&sounds](auto& note) {
              auto soundId = note.second.sound;
              if (auto sound = sounds.find(soundId); sound != sounds.end()) {
                  return BmsRules::NoteType{ &sound->second, note.first };
              }
              return BmsRules::NoteType{ nullptr, note.first };
          });
        currentInvisibleNotes[i] = invisibleNotes[i];
    }
    std::transform(notesData.bgmNotes.begin(),
                   notesData.bgmNotes.end(),
                   std::back_inserter(bgms),
                   [&sounds](auto& note) {
                       auto soundId = note.second;
                       if (auto sound = sounds.find(soundId);
                           sound != sounds.end()) {
                           return BgmType{ note.first, &sound->second };
                       }
                       return BgmType{ note.first, nullptr };
                   });
    currentBgms = bgms;
}
void
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds offsetFromStart)
{

    for (auto columnIndex = 0; columnIndex < currentVisibleNotes.size();
         columnIndex++) {
        auto& column = currentVisibleNotes[columnIndex];
        auto newMisses = rules.getMisses(column, offsetFromStart);
        column = column.subspan(newMisses.size());
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
        rules.invisibleNoteHit(invisibleColumn, offsetFromStart);
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
