//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_PARSEDBMSCHART_H
#define RHYTHMGAME_PARSEDBMSCHART_H

#include <map>
#include <unordered_map>
#include <random>
#include <optional>
#include <memory>
#include <array>
#include <utility>

namespace charts::parser_models {
/**
 * @brief Be-Music Source chart.
 */

struct ParsedBmsChart
{
    using RandomRange = int64_t;

    struct Measure
    {
        static constexpr auto columnNumber = 9;
        static constexpr auto defaultMeter = 1.0;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p1VisibleNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p2VisibleNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p1InvisibleNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p2InvisibleNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p1LongNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p2LongNotes;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p1Landmines;
        std::array<std::vector<std::vector<uint16_t>>, columnNumber>
          p2Landmines;
        std::vector<std::vector<uint16_t>> bgaBase;
        std::vector<std::vector<uint16_t>> bgaPoor;
        std::vector<std::vector<uint16_t>> bgaLayer;
        std::vector<std::vector<uint16_t>> bgaLayer2;
        std::vector<std::vector<uint16_t>> bgmNotes;
        std::vector<uint16_t> bpmChanges;   // old-school, FF = BPM is 255
        std::vector<uint16_t> exBpmChanges; // new, FF = #BPMFF
        std::vector<uint16_t> stops;
        std::optional<double> meter;
    };

    /**
     * @brief Tags that a BMS chart can have.
     */
    struct Tags
    {
        std::optional<std::string> title;
        std::optional<std::string> artist;
        std::optional<std::string> subTitle;
        std::optional<std::string> subArtist;
        std::optional<std::string> genre;
        std::optional<std::string> stageFile;
        std::optional<std::string> banner;
        std::optional<std::string> backBmp;
        std::optional<double> bpm;
        std::optional<double> total;
        std::optional<int> rank;
        std::optional<int> playLevel;
        std::optional<int> difficulty;
        std::optional<uint16_t> lnObj;
        std::optional<int> lnType;
        std::unordered_map<uint16_t, double> exBpms;
        std::unordered_map<uint16_t, double> stops;
        std::unordered_map<uint16_t, std::string> wavs;
        std::unordered_map<uint16_t, std::string> bmps;
        std::map<int64_t, Measure> measures;
        bool isRandom = false;
    };

    static auto mergeTags(Tags& first, Tags second) -> void;

    Tags tags;
};
} // namespace charts::parser_models
#endif // RHYTHMGAME_PARSEDBMSCHART_H
