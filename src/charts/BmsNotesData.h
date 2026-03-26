//
// Created by bobini on 15.06.23.
//

#ifndef RHYTHMGAME_BMSNOTESDATA_H
#define RHYTHMGAME_BMSNOTESDATA_H

#include <vector>
#include <chrono>
#include "Snap.h"
#include "ParsedBmsChart.h"

#include <QJsonObject>
#include <span>
/**
 * @brief Classes and functions related to loading and calculating offsets in
 * BMS charts.
 */
namespace charts {
struct BmsNotesData
{
    using Position = double;
    struct Time
    {
        std::chrono::nanoseconds timestamp;
        Position beatPosition; // without scrolls
        Position position;     // with scroll applied

        Time(std::chrono::nanoseconds timestamp,
             Position beatPosition,
             Position position)
          : timestamp(timestamp)
          , beatPosition(beatPosition)
          , position(position)
        {
        }

        Time() = default;

        auto operator+(const Time& other) const -> Time
        {
            return { timestamp + other.timestamp,
                     beatPosition + other.beatPosition,
                     position + other.position };
        }
        auto operator-=(const Time& other) -> Time&
        {
            timestamp -= other.timestamp;
            beatPosition -= other.beatPosition;
            position -= other.position;
            return *this;
        }

        auto operator<=>(const Time& other) const = default;
    };

    enum class NoteType
    {
        LongNoteBegin,
        Normal,
        Landmine,
        Invisible,
        LongNoteEnd
    };

    struct Note
    {
        Time time;
        Snap snap;
        NoteType noteType;
        uint16_t sound;
    };

    enum class LnType
    {
        RDM = 1,
        MGQ = 2
    };

    struct BpmChangeValues
    {
        double bpm;
        double scroll;
        Time timestamp;
    };

    static constexpr auto defaultBeatsPerMeasure = 4;
    static constexpr auto columnMapping =
      std::array{ 0, 1, 2, 3, 4, 7, 8, 5 }; // ignore "foot pedal"
    static constexpr auto columnNumber = columnMapping.size() * 2;
    std::array<std::vector<Note>, columnNumber> notes;
    std::vector<std::pair<Time, uint64_t>> bgmNotes;
    std::vector<std::pair<Time, uint64_t>> bgaBase;
    std::vector<std::pair<Time, uint64_t>> bgaPoor;
    std::vector<std::pair<Time, uint64_t>> bgaLayer;
    std::vector<std::pair<Time, uint64_t>> bgaLayer2;
    std::vector<BpmChangeValues> bpmChanges;
    std::vector<Time> barLines;
    static constexpr auto defaultBpm = 120.0;
    static constexpr auto defaultLnType = LnType::RDM;
    static constexpr auto defaultBase = 36;
    static constexpr auto defaultScroll = 1.0;

    /// Describes one audio slice for bmson sound loading.
    struct BmsonSliceInfo
    {
        uint64_t soundId;      ///< Unique sound ID assigned to this slice.
        uint64_t channelIndex; ///< Index into the bmson sound_channels array.
        double startSeconds;   ///< Slice start time in the audio file.
        double endSeconds;     ///< Slice end time (-1 = end of file).
    };

    /// Slice descriptors for bmson. Empty for BMS charts.
    std::vector<BmsonSliceInfo> bmsonSlices;

    /// Fusion map for bmson: maps a fused sound ID to the list of slice
    /// sound IDs that should play simultaneously. Only populated when
    /// multiple channels have notes at the same (column, pulse).
    std::unordered_map<uint64_t, std::vector<uint64_t>> bmsonFusions;

    static auto fromParsedChart(const ParsedBmsChart& chart) -> BmsNotesData;
    static auto fromBmson(const QJsonObject& bmson) -> BmsNotesData;

  private:
    void generateMeasures(
      double baseBpm,
      const std::unordered_map<uint16_t, double>& bpms,
      const std::unordered_map<uint16_t, double>& stops,
      const std::unordered_map<uint16_t, double>& scrolls,
      const std::unordered_map<uint16_t, double>& speeds,
      const std::map<int64_t, ParsedBmsChart::Measure>& measures,
      LnType lnType,
      std::optional<uint16_t> lnObj);
    void fillEmptyMeasures(int64_t lastMeasure,
                           int64_t measureIndex,
                           Time& measureStart,
                           double lastBpm,
                           double lastScroll);
    void adjustRdmLnEnds(
      const std::array<std::optional<size_t>,
                       ParsedBmsChart::Measure::columnNumber>&
        lastInsertedRdmNoteP1,
      const std::array<std::optional<size_t>,
                       ParsedBmsChart::Measure::columnNumber>&
        lastInsertedRdmNoteP2,
      std::span<std::vector<Note>> notes);
    void adjustMgqLnEnds(
      double lastBpm,
      double lastScroll,
      Time measureStart,
      std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP1,
      std::array<bool, ParsedBmsChart::Measure::columnNumber>& insideLnP2,
      std::span<std::vector<Note>> target);
};
} // namespace charts
#endif // RHYTHMGAME_BMSNOTESDATA_H
