//
// Created by bobini on 21.06.23.
//

#include <ranges>
#include <algorithm>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  const charts::gameplay_models::BmsChart* chart,
  gameplay_logic::BmsRules rules)
  : rules(rules)
{
    for (int i = 0; i < charts::gameplay_models::BmsChart::columnNumber; i++) {
        std::transform(
          chart->visibleNotes[i].begin(),
          chart->visibleNotes[i].end(),
          std::back_inserter(visibleNotes[i]),
          [](auto& note) {
              return BmsRules::NoteType{ note.second.sound, note.first };
          });
        currentVisibleNotes[i] = visibleNotes[i];
        std::transform(
          chart->invisibleNotes[i].begin(),
          chart->invisibleNotes[i].end(),
          std::back_inserter(invisibleNotes[i]),
          [](auto& note) {
              return BmsRules::NoteType{ note.second.sound, note.first };
          });
        currentInvisibleNotes[i] = invisibleNotes[i];
    }
    bgms = chart->bgmNotes;
    score.maxHits = std::accumulate(
      chart->visibleNotes.begin(),
      chart->visibleNotes.end(),
      0,
      [](int acc, auto& column) { return acc + column.size(); });
    score.maxPoints = score.maxHits * BmsPoints::maxValue;
}
void
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds delta)
{
    if (startTime == gameplay_logic::TimePoint{}) {
        return;
    }
    timePassed += delta;
    for (auto& column : currentVisibleNotes) {
        auto newMisses =
          rules.getMisses(column, startTime + timePassed, startTime);
        column = column.subspan(newMisses.size());
        for (auto miss : newMisses) {
            score.addMiss(miss);
        }
    }
    for (auto& column : currentInvisibleNotes) {
        auto skipped =
          rules.skipInvisible(column, startTime + timePassed, startTime);
        column = column.subspan(skipped);
    }
    for (const auto& bgm : bgms) {
        if (bgm.first < timePassed) {
            bgm.second->play();
        } else {
            break;
        }
    }
}
auto
gameplay_logic::BmsGameReferee::passInput(gameplay_logic::TimePoint timePoint,
                                          input::BmsKey key)
  -> std::optional<int>
{
    if (startTime == gameplay_logic::TimePoint{}) {
        return std::nullopt;
    }
    auto columnIndex = static_cast<int>(key);
    if (columnIndex < 0 ||
        columnIndex >= charts::gameplay_models::BmsChart::columnNumber) {
        return std::nullopt;
    }
    auto& column = currentVisibleNotes[columnIndex];
    auto& invisibleColumn = currentInvisibleNotes[columnIndex];
    auto res = rules.visibleNoteHit(column, timePoint, startTime);
    if (!res) {
        rules.invisibleNoteHit(invisibleColumn, timePoint, startTime);
        score.addHit(timePoint);
        return std::nullopt;
    }
    auto [points, iter] = *res;
    score.addHit(timePoint, points);
    return iter - column.begin();
}
void
gameplay_logic::BmsGameReferee::start(gameplay_logic::TimePoint newStartTime)
{
    startTime = newStartTime;
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
gameplay_logic::BmsGameReferee::getScore() const -> const BmsScore&
{
    return score;
}