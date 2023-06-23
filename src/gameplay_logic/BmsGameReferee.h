//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <SFML/Window/Keyboard.hpp>
#include "charts/gameplay_models/BmsChart.h"
#include "BmsRules.h"
namespace gameplay_logic {
class BmsGameReferee
{
    using NoteType = decltype(charts::gameplay_models::BmsChart::visibleNotes)::
      value_type::value_type;
    using BgmType =
      decltype(charts::gameplay_models::BmsChart::bgmNotes)::value_type;

    std::array<std::span<const NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      visibleNotes;
    std::array<std::span<const NoteType>,
               charts::gameplay_models::BmsChart::columnNumber>
      invisibleNotes;
    std::span<const BgmType> bgms;
    BmsRules rules;
    gameplay_logic::TimePoint startTime;
    std::chrono::nanoseconds timePassed;

  public:
    explicit BmsGameReferee(const charts::gameplay_models::BmsChart* chart,
                            BmsRules rules);
    void start(gameplay_logic::TimePoint time);
    /**
     * Update the position of the chart
     * @param delta time since last update
     * @warning Call **BEFORE** passing inputs
     */
    void update(std::chrono::nanoseconds delta);
    void passInput(sf::Keyboard::Key key);
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
