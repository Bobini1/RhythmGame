//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <span>
#include "charts/gameplay_models/BmsNotesData.h"
#include "input/BmsKeys.h"
#include "BmsScore.h"
#include "gameplay_logic/rules/BmsHitRules.h"
#include "sounds/OpenAlSound.h"
namespace gameplay_logic {
class BmsGameReferee
{
    using BgmType = std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>;

    std::array<std::vector<rules::BmsHitRules::Note>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      notes;
    std::array<std::vector<rules::BmsHitRules::Mine>,
               charts::gameplay_models::BmsNotesData::columnNumber>
      mines;
    std::vector<BgmType> bgms;
    std::span<BgmType> currentBgms;
    std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      bpmChanges;
    std::span<std::pair<charts::gameplay_models::BmsNotesData::Time, double>>
      currentBpmChanges;
    std::unordered_map<uint16_t, sounds::OpenALSound> sounds;
    std::unique_ptr<rules::BmsHitRules> hitRules;
    BmsScore* score;
    sounds::OpenALSound* mineHitSound;
    std::array<bool, charts::gameplay_models::BmsNotesData::columnNumber>
      pressedState{};

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
      std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>,
                 charts::gameplay_models::BmsNotesData::columnNumber> notes,
      const std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time,
                                  uint16_t>>& bgmNotes,
      std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time,
                            double>> bpmChanges,
      sounds::OpenALSound* mineHitSound,
      BmsScore* score,
      std::unordered_map<uint16_t, sounds::OpenALSound> sounds,
      std::unique_ptr<rules::BmsHitRules> hitRules);
    /**
     * @brief Update the internal state of the referee
     * @param offsetFromStart The current time offset from the start of the
     * chart
     * @return The position in the chart, expressed in beats
     */
    auto update(std::chrono::nanoseconds offsetFromStart,
                bool lastUpdate = false) -> Position;

    auto passPressed(std::chrono::nanoseconds offsetFromStart,
                     input::BmsKey key) -> void;
    auto passReleased(std::chrono::nanoseconds offsetFromStart,
                      input::BmsKey key) -> void;
    void addNote(decltype(notes)::value_type& column,
                 decltype(mines)::value_type& minesColumn,
                 const charts::gameplay_models::BmsNotesData::Note& note,
                 int index);
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
