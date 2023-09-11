//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <list>
#include <span>
#include <boost/container/flat_map.hpp>
#include "charts/gameplay_models/BmsNotesData.h"
#include "input/BmsKeys.h"
#include "BmsScore.h"
#include "BmsNotes.h"
#include "gameplay_logic/rules/BmsHitRules.h"
namespace gameplay_logic {
class BmsGameReferee
{
    using BgmType = std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>;

    std::array<std::vector<rules::BmsHitRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      visibleNotes;
    std::array<std::vector<rules::BmsHitRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      invisibleNotes;
    std::array<std::span<rules::BmsHitRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      currentVisibleNotes;
    std::array<std::span<rules::BmsHitRules::NoteType>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      currentInvisibleNotes;
    std::vector<BgmType> bgms;
    std::span<BgmType> currentBgms;
    std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      bpmChanges;
    std::span<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      currentBpmChanges;
    std::unordered_map<std::string, sounds::OpenALSound> sounds;
    charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart;
    std::unique_ptr<rules::BmsHitRules> hitRules;
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
    void playLastKeysound(int index, std::chrono::nanoseconds offsetFromStart);

  public:
    explicit BmsGameReferee(
      const charts::gameplay_models::BmsNotesData& notesData,
      BmsScore* score,
      std::unordered_map<std::string, sounds::OpenALSound> sounds,
      std::unique_ptr<rules::BmsHitRules> hitRules,
      charts::gameplay_models::BmsNotesData::Time timeBeforeChartStart);
    /**
     * @brief Update the internal state of the referee
     * @param offsetFromStart The current time offset from the start of the
     * chart
     * @return The position in the chart, expressed in beats
     */
    auto update(std::chrono::nanoseconds offsetFromStart) -> Position;

    auto passInput(std::chrono::nanoseconds offsetFromStart, input::BmsKey key)
      -> void;
    auto isOver() const -> bool;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
