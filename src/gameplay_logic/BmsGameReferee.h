//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <SFML/Window/Keyboard.hpp>
#include <list>
#include <span>
#include <boost/container/flat_map.hpp>
#include "charts/gameplay_models/BmsChart.h"
#include "BmsRules.h"
#include "input/BmsKeys.h"
#include "BmsScore.h"
namespace gameplay_logic {
class BmsGameReferee
{
    using BgmType = std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>;

    std::array<std::vector<BmsRules::NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      visibleNotes;
    std::array<std::vector<BmsRules::NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      invisibleNotes;
    std::array<std::span<BmsRules::NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      currentVisibleNotes;
    std::array<std::span<BmsRules::NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      currentInvisibleNotes;
    std::vector<BgmType> bgms;
    BmsRules rules;
    gameplay_logic::TimePoint startTime{};
    std::chrono::nanoseconds timePassed{};
    BmsScore score;

    boost::container::flat_map<Judgement, int> judgements;
    double totalPoints = 0;

  public:
    explicit BmsGameReferee(const charts::gameplay_models::BmsChart* chart,
                            gameplay_logic::BmsRules rules);
    /**
     * Update the position of the chart
     * @param delta time since last update
     * @warning Call **AFTER** passing inputs.
     */
    void update(std::chrono::nanoseconds delta);

    auto passInput(gameplay_logic::TimePoint timePoint, input::BmsKey key)
      -> std::optional<int>;
    void start(gameplay_logic::TimePoint newStartTime);
    auto isOver() const -> bool;
    auto getScore() const -> const BmsScore&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
