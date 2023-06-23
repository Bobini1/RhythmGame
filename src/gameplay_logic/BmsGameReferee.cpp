//
// Created by bobini on 21.06.23.
//

#include <ranges>
#include "BmsGameReferee.h"
gameplay_logic::BmsGameReferee::BmsGameReferee(
  const charts::gameplay_models::BmsChart* chart,
  gameplay_logic::BmsRules rules)
  : rules(rules)
{
    for (int i = 0; i < charts::gameplay_models::BmsChart::columnNumber; i++) {
        visibleNotes[i] = chart->visibleNotes[i];
        invisibleNotes[i] = chart->invisibleNotes[i];
    }
    bgms = chart->bgmNotes;
}
void
gameplay_logic::BmsGameReferee::update(std::chrono::nanoseconds delta)
{
    timePassed += delta;
    auto allMisses = std::vector<gameplay_logic::TimePoint>{};
    for (auto& column : visibleNotes) {
        auto misses = rules.getMisses(
          column | std::ranges::views::transform([this](auto& timeNote) {
              return startTime + timeNote.first;
          }),
          startTime + timePassed);
        column = column.subspan(misses.size());
        allMisses.insert(allMisses.end(), misses.begin(), misses.end());
    }
    for (auto& column : invisibleNotes) {
        auto skipped = rules.countInvisibleSkipped(
          column | std::ranges::views::transform([this](auto& timeNote) {
              return startTime + timeNote.first;
          }),
          startTime + timePassed);
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
