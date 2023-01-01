//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "charts/chart_readers/BmsChartReader.h"
#include "lua/Lua.h"

TEST_CASE("Check if Title is parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Check if Artist is parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
}

TEST_CASE("Multiple tags at once", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres\n#TITLE END TIME"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Extra whitespace is ignored", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = " #ARTIST   cres   \n\n #TITLE     END TIME  \n"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Check if BPM is parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#BPM 120.0"s;
    constexpr auto expectedBpm = 120.0;
    constexpr auto allowedError = 0.00001;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);

    auto& res = *resReader;
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120"s;
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120.";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120.0F";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 12E1d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 1200E-1f";

    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM -120.0";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(-expectedBpm).epsilon(allowedError));
}

TEST_CASE("Random blocks get parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#BPM 120\n#SUBTITLE testSubTitle\n#GENRE bicior\n#SUBARTIST MC BOBSON\n#ENDIF"s;
    auto resReader = reader.readBmsChart(testString);
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
    REQUIRE(res.randomBlocks[0].second->begin()->second.bpm == 120);
    REQUIRE(res.randomBlocks[0].second->begin()->second.subTitle ==
            "testSubTitle"s);
    REQUIRE(res.randomBlocks[0].second->begin()->second.genre == "bicior"s);
    REQUIRE(res.randomBlocks[0].second->begin()->second.subArtist ==
            "MC BOBSON"s);
}

TEST_CASE("Nested random blocks", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#RANDOM 1\n#IF 1\n#ARTIST -45\n#ENDIF\n#ENDRANDOM\n#ENDIF"s;
    auto resReader = reader.readBmsChart(testString);
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

TEST_CASE("Test return values on failed parse", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = ""s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE_FALSE(resReader);

    testString = "#ARTST -44"s;
    auto resReaderTags = reader.readBmsChart(testString);
    REQUIRE(resReaderTags == std::nullopt);
}

TEST_CASE("Check if readBmsChart returns an actual chart", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    sol::state lua;
    constexpr auto expectedBpm = 180.0;
    constexpr auto allowedError = 0.00001;
    auto testString =
      " #ARTIST   cres   \n\n #TITLE     END TIME  \n   #BPM 180"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = resReader.value();

    REQUIRE(res.title);
    REQUIRE(res.title == "END TIME"s);
    REQUIRE(res.artist);
    REQUIRE(res.artist == "cres"s);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));
}

TEST_CASE("Check if unicode is parsed correctly", "[BmsChartReader]")
{
    // note for using unicode - string literals MUST be used, otherwise sol will
    // throw an exception
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    sol::state lua;
    constexpr auto expectedBpm = 166.0;
    constexpr auto allowedError = 0.00001;
    auto testString =
      "#ARTIST LUNEの右手と悠里おねぇちゃんの左脚 \n\n #TITLE どうか私を殺して下さい -もう、樹海しか見えない-\n   #BPM 166"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = resReader.value();
    REQUIRE(res.title);
    REQUIRE(res.title == "どうか私を殺して下さい -もう、樹海しか見えない-"s);
    REQUIRE(res.artist);
    REQUIRE(res.artist == "LUNEの右手と悠里おねぇちゃんの左脚"s);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));
}

TEST_CASE("Notes get read from a chart correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00111:00"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto resReader = reader.readBmsChart(chart);
    REQUIRE(resReader);
    auto& res = resReader.value();
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[1].size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[1][0] == "00"s);
}

TEST_CASE("Notes get read from a chart correctly with multiple notes",
          "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00111:00010203"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto resReader = reader.readBmsChart(chart);
    REQUIRE(resReader);
    auto& res = resReader.value();
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[1].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[1][0] == "00"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][1] == "01"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][2] == "02"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][3] == "03"s);
}

TEST_CASE("Notes get read from a chart correctly with multiple lanes",
          "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00111:00010203\n#00112:04050607"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto resReader = reader.readBmsChart(chart);
    REQUIRE(resReader);
    auto& res = resReader.value();
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[1].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[1][0] == "00"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][1] == "01"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][2] == "02"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][3] == "03"s);
    REQUIRE(res.measures[1].p1VisibleNotes[2].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[2][0] == "04"s);
    REQUIRE(res.measures[1].p1VisibleNotes[2][1] == "05"s);
    REQUIRE(res.measures[1].p1VisibleNotes[2][2] == "06"s);
    REQUIRE(res.measures[1].p1VisibleNotes[2][3] == "07"s);
}

TEST_CASE("Bgm notes get parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00101:00010203\n#00101:04050607"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto resReader = reader.readBmsChart(chart);
    REQUIRE(resReader);
    auto& res = resReader.value();
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].bgmNotes.size() == 2);
    REQUIRE(res.measures[1].bgmNotes[0].size() == 4);
    REQUIRE(res.measures[1].bgmNotes[0][0] == "00"s);
    REQUIRE(res.measures[1].bgmNotes[0][1] == "01"s);
    REQUIRE(res.measures[1].bgmNotes[0][2] == "02"s);
    REQUIRE(res.measures[1].bgmNotes[0][3] == "03"s);
    REQUIRE(res.measures[1].bgmNotes[1].size() == 4);
    REQUIRE(res.measures[1].bgmNotes[1][0] == "04"s);
    REQUIRE(res.measures[1].bgmNotes[1][1] == "05"s);
    REQUIRE(res.measures[1].bgmNotes[1][2] == "06"s);
    REQUIRE(res.measures[1].bgmNotes[1][3] == "07"s);
}

TEST_CASE("Invalid chart doesn't get parsed")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00101:00010203\n#00101:0405060"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto resReader = reader.readBmsChart(chart);
    REQUIRE(!resReader);
}