//
// Created by bobini on 22.07.2022.
//

#ifndef RHYTHMGAME_PARSEDBMSCHART_H
#define RHYTHMGAME_PARSEDBMSCHART_H

#include <map>
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
    using IfTag = int64_t;

    struct Measure
    {
        static constexpr auto columnNumber = 8;
        std::array<std::vector<std::string>, columnNumber> p1VisibleNotes;
        std::array<std::vector<std::string>, columnNumber> p2VisibleNotes;
        std::array<std::vector<std::string>, columnNumber> p1InvisibleNotes;
        std::array<std::vector<std::string>, columnNumber> p2InvisibleNotes;
        std::array<std::vector<std::string>, columnNumber> p1LongNotes;
        std::array<std::vector<std::string>, columnNumber> p2LongNotes;
        std::vector<std::vector<std::string>> bgmNotes;
        std::vector<std::string> bpmChanges;   // old-school, FF = BPM is 255
        std::vector<std::string> exBpmChanges; // new, FF = #BPMFF
        double meter = 1;
    };

    /**
     * @brief Tags that a BMS chart can have.
     */
    struct Tags
    {
        std::optional<std::string> title;
        std::optional<std::string> artist;
        std::optional<double> bpm;
        std::optional<std::string> subTitle;
        std::optional<std::string> subArtist;
        std::optional<std::string> genre;
        std::map<std::string, double> exBpms;
        std::map<std::string, std::string> wavs;
        std::map<uint64_t, Measure> measures;

        std::vector<std::pair<RandomRange, std::vector<std::pair<IfTag, Tags>>>>
          randomBlocks; /*< Random blocks can hold any tags, including ones
                           that were already defined. */
    };

    Tags tags;
};
} // namespace charts::parser_models
#endif // RHYTHMGAME_PARSEDBMSCHART_H
