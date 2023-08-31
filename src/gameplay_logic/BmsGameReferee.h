//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <list>
#include <span>
#include <boost/container/flat_map.hpp>
#include "charts/gameplay_models/BmsNotesData.h"
#include "gameplay_logic/BmsRules.h"
#include "input/BmsKeys.h"
#include "BmsScore.h"
namespace gameplay_logic {
class BmsGameReferee
{
    using BgmType = std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>;

    std::array<std::vector<BmsRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      visibleNotes;
    std::array<std::vector<BmsRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      invisibleNotes;
    std::array<std::span<BmsRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      currentVisibleNotes;
    std::array<std::span<BmsRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      currentInvisibleNotes;
    std::vector<BgmType> bgms;
    std::span<BgmType> currentBgms;
    std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      bpmChanges;
    std::span<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      currentBpmChanges;
    BmsRules rules;
    BmsScore* score;

  public:
    using Position = double;

  private:
    /**
     * @brief Get the position in the chart, expressed in beats
     * @warning This function mutates the internal state of the referee. It is
     * not const for a reason!
     * @param offsetFromStart The current time offset from the start of the
     * chart
     * @return The position in the chart, expressed in beats
     */
    auto getPosition(std::chrono::nanoseconds offsetFromStart) -> Position;

  public:
    explicit BmsGameReferee(
      const charts::gameplay_models::BmsNotesData& notesData,
      BmsScore* score,
      std::unordered_map<std::string, sounds::OpenALSound>& sounds,
      gameplay_logic::BmsRules rules);
    /**
     * @brief Update the internal state of the referee
     * @param offsetFromStart The current time offset from the start of the
     * chart
     * @return The position in the chart, expressed in beats
     */
    auto update(std::chrono::nanoseconds offsetFromStart) -> Position;

    auto passInput(std::chrono::nanoseconds offsetFromStart, input::BmsKey key)
      -> std::optional<int>;
    auto isOver() const -> bool;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
