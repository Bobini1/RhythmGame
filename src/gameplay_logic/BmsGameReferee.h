//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <list>
#include <span>
#include <boost/container/flat_map.hpp>
#include "charts/gameplay_models/BmsChart.h"
#include "gameplay_logic/BmsRules.h"
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
    std::span<BgmType> currentBgms;
    BmsRules rules;
    BmsScore* score;

    boost::container::flat_map<Judgement, int> judgements;
    double totalPoints = 0;

  public:
    explicit BmsGameReferee(const charts::gameplay_models::BmsChart* chart,
                            BmsScore* score,
                            std::map<std::string, sounds::OpenALSound>* sounds,
                            gameplay_logic::BmsRules rules);
    void update(std::chrono::nanoseconds offsetFromStart);

    auto passInput(std::chrono::nanoseconds offsetFromStart, input::BmsKey key)
      -> std::optional<int>;
    auto isOver() const -> bool;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
