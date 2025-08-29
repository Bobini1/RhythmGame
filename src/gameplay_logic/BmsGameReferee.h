//
// Created by bobini on 21.06.23.
//

#ifndef RHYTHMGAME_BMSGAMEREFEREE_H
#define RHYTHMGAME_BMSGAMEREFEREE_H

#include <span>
#include "charts/BmsNotesData.h"
#include "input/BmsKeys.h"
#include "BmsLiveScore.h"
#include "gameplay_logic/rules/StandardBmsHitRules.h"
#include "sounds/OpenAlSound.h"

/**
 * @brief Classes and functions mostly related to gameplay.
 */
namespace gameplay_logic {

/**
 * @brief The update manager of notes and sounds during gameplay.
 * @details The BmsGameReferee class is responsible for getting hits
 * and misses of notes, playing note and BGM sounds,
 * and providing the current position of notes.
 */
class BmsGameReferee
{
    using BgmType = std::pair<std::chrono::nanoseconds, sounds::OpenALSound*>;

    std::array<std::vector<rules::StandardBmsHitRules::Note>,
               charts::BmsNotesData::columnNumber>
      notes;
    std::array<std::vector<rules::StandardBmsHitRules::Mine>,
               charts::BmsNotesData::columnNumber>
      mines;
    std::vector<BgmType> bgms;
    std::span<BgmType> currentBgms;
    std::vector<std::pair<charts::BmsNotesData::Time, double>> bpmChanges;
    std::unordered_map<uint16_t, sounds::OpenALSound> sounds;
    rules::StandardBmsHitRules hitRules;
    BmsLiveScore* score;
    sounds::OpenALSound* mineHitSound;
    std::array<bool, charts::BmsNotesData::columnNumber> pressedState{};

  public:
    using Position = double;

    BmsGameReferee(const BmsGameReferee&) = delete;
    BmsGameReferee& operator=(const BmsGameReferee&) = delete;
    BmsGameReferee(BmsGameReferee&&) = default;
    BmsGameReferee& operator=(BmsGameReferee&&) = default;

  private:
    void addNote(decltype(notes)::value_type& column,
                 decltype(mines)::value_type& minesColumn,
                 const charts::BmsNotesData::Note& note,
                 int index);

  public:
    explicit BmsGameReferee(
      std::array<std::vector<charts::BmsNotesData::Note>,
                 charts::BmsNotesData::columnNumber> notes,
      const std::vector<std::pair<charts::BmsNotesData::Time, uint16_t>>&
        bgmNotes,
      std::vector<std::pair<charts::BmsNotesData::Time, double>> bpmChanges,
      sounds::OpenALSound* mineHitSound,
      BmsLiveScore* score,
      std::unordered_map<uint16_t, sounds::OpenALSound> sounds,
      rules::StandardBmsHitRules hitRules);
    /**
     * @brief Update the internal state of the referee
     * @param offsetFromStart The current time offset from the start of the
     * chart
     * @param lastUpdate If true, don't play any sounds in this and future
     * updates
     * @warning If this function detects a miss and you then use passPressed()
     * or passReleased() with an older timestamp, the miss will not be
     * rectified.
     */
    void update(std::chrono::nanoseconds offsetFromStart,
                bool lastUpdate = false);

    /**
     * @brief Get the position in the chart, expressed in beats
     * @param offsetFromStart The time offset from the start of the
     * chart
     * @return The position in the chart, expressed in beats
     */
    auto getPosition(std::chrono::nanoseconds offsetFromStart) const
      -> Position;

    /**
     * @brief Handle a key press event
     * @param offsetFromStart The time offset from the start of the
     * chart when the press happened
     * @param key The key that was pressed
     * @see update()
     */
    auto passPressed(std::chrono::nanoseconds offsetFromStart,
                     input::BmsKey key) -> void;
    /**
     * @brief Handle a key press event
     * @param offsetFromStart The time offset from the start of the
     * chart when the release happened
     * @param key The key that was released
     * @see update()
     */
    auto passReleased(std::chrono::nanoseconds offsetFromStart,
                      input::BmsKey key) -> void;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAMEREFEREE_H
