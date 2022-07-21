//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include "charts/chart_readers/BmsChartReader.h"
#include <string>
#include <iostream>
#include <regex>

TEST_CASE("Check if Title is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.getTitle() == "END TIME"s);
}

TEST_CASE("Check if Artist is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.getArtist() == "cres"s);
}

TEST_CASE("Multiple tags at once", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres\n#TITLE END TIME"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.getArtist() == "cres"s);
    REQUIRE(res.getTitle() == "END TIME"s);
}

TEST_CASE("Extra whitespace is ignored", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = " #ARTIST   cres   \n\n #TITLE     END TIME  \n"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.getArtist() == "cres"s);
    REQUIRE(res.getTitle() == "END TIME"s);
}

TEST_CASE("Check if BPM is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#BPM 120.0"s;
    constexpr auto expectedBpm = 120.0;
    constexpr auto allowedError = 0.00001;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    auto res = std::move(resReader.value());

    auto difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120"s;
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120.";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120.0F";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 12E1d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 1200E-1f";

    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM -120.0";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader.has_value());
    res = resReader.value();
    difference = res.getBpm() + expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);
}

TEST_CASE("Random blocks get parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#RANDOM 5\n#IF 5\n#TITLE 44river\n#ENDIF"s;
    auto resReader = reader.readBmsChartTags(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.title == std::optional<std::string>{});
    REQUIRE(res.artist == std::optional<std::string>{});
    REQUIRE(res.bpm == std::optional<double>{});
    REQUIRE(res.randomBlocks.size() == 1);
    REQUIRE(res.randomBlocks[0].first == std::uniform_int_distribution(0L, 5L));
    REQUIRE(res.randomBlocks[0].second->size() == 1);
    REQUIRE(res.randomBlocks[0].second->contains(5));
    REQUIRE(res.randomBlocks[0].second->begin()->second.title == "44river"s);
}

TEST_CASE("Nested random blocks", "[single-file]")
{

    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#RANDOM 1\n#IF 1\n#ARTIST -45\n#ENDIF\n#ENDRANDOM\n#ENDIF"s;
    auto resReader = reader.readBmsChartTags(testString);
    REQUIRE(resReader.has_value());
    auto& res = resReader.value();
    REQUIRE(res.title == std::optional<std::string>{});
    REQUIRE(res.artist == std::optional<std::string>{});
    REQUIRE(res.bpm == std::optional<double>{});
    REQUIRE(res.randomBlocks.size() == 1);
    REQUIRE(res.randomBlocks[0].first == std::uniform_int_distribution(0L, 5L));
    REQUIRE(res.randomBlocks[0].second->size() == 1);
    REQUIRE(res.randomBlocks[0].second->contains(5));
    REQUIRE(res.randomBlocks[0].second->begin()->second.title == "44river"s);
    REQUIRE(res.randomBlocks[0].second->begin()->second.randomBlocks.size() ==
            1);
    REQUIRE(res.randomBlocks[0].second->begin()->second.randomBlocks[0].first ==
            std::uniform_int_distribution(0L, 1L));
    REQUIRE(res.randomBlocks[0]
              .second->begin()
              ->second.randomBlocks[0]
              .second->size() == 1);
    REQUIRE(res.randomBlocks[0]
              .second->begin()
              ->second.randomBlocks[0]
              .second->contains(1) == 1);
    REQUIRE(res.randomBlocks[0]
              .second->begin()
              ->second.randomBlocks[0]
              .second->begin()
              ->second.artist == "-45"s);
}