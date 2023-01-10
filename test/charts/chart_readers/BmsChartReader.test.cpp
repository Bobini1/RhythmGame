//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "charts/chart_readers/BmsChartReader.h"
#include "lua/Lua.h"

using namespace std::literals::string_literals;
TEST_CASE("Parse title", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Parse artist", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
}

TEST_CASE("Parse multiple tags at once", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres\n#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Extra whitespace is ignored", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = " #ARTIST   cres   \n\n #TITLE     END TIME  \n"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.artist);
    REQUIRE(res.artist.value() == "cres"s);
    REQUIRE(res.title);
    REQUIRE(res.title.value() == "END TIME"s);
}

TEST_CASE("Parse BPM", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#BPM 120.0"s;
    constexpr auto expectedBpm = 120.0;
    constexpr auto allowedError = 0.00001;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120"s;
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120.";
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120.0F";
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 120d";
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 12E1d";
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM 1200E-1f";

    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));

    testString = "#BPM -120.0";
    res = reader.readBmsChart(testString);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(-expectedBpm).epsilon(allowedError));
}

TEST_CASE("Random blocks get parsed correctly", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#BPM 120\n#SUBTITLE testSubTitle\n#GENRE bicior\n#SUBARTIST MC BOBSON\n#ENDIF"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.title == std::optional<std::string>{});
    REQUIRE(res.artist == std::optional<std::string>{});
    REQUIRE(res.bpm == std::optional<double>{});
    REQUIRE(res.randomBlocks.size() == 1);
    REQUIRE(res.randomBlocks[0].first == 5L);
    REQUIRE(res.randomBlocks[0].second.size() == 1);
    REQUIRE(res.randomBlocks[0].second[0].first == 5L);
    REQUIRE(res.randomBlocks[0].second[0].second.title == "44river"s);
    REQUIRE(res.randomBlocks[0].second[0].second.bpm == 120);
    REQUIRE(res.randomBlocks[0].second[0].second.subTitle == "testSubTitle"s);
    REQUIRE(res.randomBlocks[0].second[0].second.genre == "bicior"s);
    REQUIRE(res.randomBlocks[0].second[0].second.subArtist == "MC BOBSON"s);
}

TEST_CASE("Nested random blocks", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#RANDOM 1\n#IF 1\n#ARTIST -45\n#ENDIF\n#ENDRANDOM\n#ENDIF"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.title == std::optional<std::string>{});
    REQUIRE(res.artist == std::optional<std::string>{});
    REQUIRE(res.bpm == std::optional<double>{});
    REQUIRE(res.randomBlocks.size() == 1);
    REQUIRE(res.randomBlocks[0].first == 5L);
    REQUIRE(res.randomBlocks[0].second.size() == 1);
    REQUIRE(res.randomBlocks[0].second[0].first == 5L);
    REQUIRE(res.randomBlocks[0].second[0].second.title == "44river"s);
    REQUIRE(res.randomBlocks[0].second[0].second.randomBlocks.size() == 1);
    REQUIRE(res.randomBlocks[0].second[0].second.randomBlocks[0].first == 1L);
    REQUIRE(
      res.randomBlocks[0].second[0].second.randomBlocks[0].second.size() == 1);
    REQUIRE(
      res.randomBlocks[0].second[0].second.randomBlocks[0].second[0].first ==
      1);
    REQUIRE(res.randomBlocks[0]
              .second[0]
              .second.randomBlocks[0]
              .second[0]
              .second.artist == "-45"s);
}

TEST_CASE("Parse unicode", "[BmsChartReader]")
{
    auto reader = charts::chart_readers::BmsChartReader{};
    constexpr auto expectedBpm = 166.0;
    constexpr auto allowedError = 0.00001;
    auto testString =
      "#ARTIST LUNEの右手と悠里おねぇちゃんの左脚 \n\n #TITLE どうか私を殺して下さい -もう、樹海しか見えない-\n   #BPM 166"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.title);
    REQUIRE(res.title == "どうか私を殺して下さい -もう、樹海しか見えない-"s);
    REQUIRE(res.artist);
    REQUIRE(res.artist == "LUNEの右手と悠里おねぇちゃんの左脚"s);
    REQUIRE(res.bpm);
    REQUIRE(res.bpm.value() ==
            Catch::Approx(expectedBpm).epsilon(allowedError));
}

TEST_CASE("Parse notes", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    const auto chart = "#00111:00"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[0].size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[0][0] == "00"s);
}

TEST_CASE("Parse notes from a chart with multiple notes", "[BmsChartReader]")
{
    const auto chart = "#00111:00010203"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[0].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[0][0] == "00"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][1] == "01"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][2] == "02"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][3] == "03"s);
}

TEST_CASE("Parse notes from a chart with multiple lanes", "[BmsChartReader]")
{
    const auto chart = "#00111:00010203\n#00112:04050607"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures.size() == 1);
    REQUIRE(res.measures[1].p1VisibleNotes[0].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[0][0] == "00"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][1] == "01"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][2] == "02"s);
    REQUIRE(res.measures[1].p1VisibleNotes[0][3] == "03"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1].size() == 4);
    REQUIRE(res.measures[1].p1VisibleNotes[1][0] == "04"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][1] == "05"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][2] == "06"s);
    REQUIRE(res.measures[1].p1VisibleNotes[1][3] == "07"s);
}

TEST_CASE("Bgm notes get parsed correctly", "[BmsChartReader]")
{
    const auto chart = "#00101:00010203\n#00101:04050607"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
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

TEST_CASE("Parse all basic note types get", "[BmsChartReader]")
{
    const auto chart =
      "#00101:00010203\n  #00101:04050607 \n#00511:0405\n#01021:00\n"
      "#99935:123456\n#88844:01\n#77753:10\n#66662:11 "s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures.size() == 7);
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
    REQUIRE(res.measures[5].p1VisibleNotes[0].size() == 2);
    REQUIRE(res.measures[5].p1VisibleNotes[0][0] == "04"s);
    REQUIRE(res.measures[5].p1VisibleNotes[0][1] == "05"s);
    REQUIRE(res.measures[10].p2VisibleNotes[0].size() == 1);
    REQUIRE(res.measures[10].p2VisibleNotes[0][0] == "00"s);
    REQUIRE(res.measures[999].p1InvisibleNotes[4].size() == 3);
    REQUIRE(res.measures[999].p1InvisibleNotes[4][0] == "12"s);
    REQUIRE(res.measures[999].p1InvisibleNotes[4][1] == "34"s);
    REQUIRE(res.measures[999].p1InvisibleNotes[4][2] == "56"s);
    REQUIRE(res.measures[888].p2InvisibleNotes[3].size() == 1);
    REQUIRE(res.measures[888].p2InvisibleNotes[3][0] == "01"s);
    REQUIRE(res.measures[777].p1LongNotes[2].size() == 1);
    REQUIRE(res.measures[777].p1LongNotes[2][0] == "10"s);
    REQUIRE(res.measures[666].p2LongNotes[1].size() == 1);
    REQUIRE(res.measures[666].p2LongNotes[1][0] == "11"s);
}

TEST_CASE("Error recovery on bad value", "[BmsChartReader]")
{
    const auto chart = "#BPM 120\n#BPM %sdalk"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.bpm == 120);
}

TEST_CASE("Parse meter", "[BmsChartReader]")
{
    const auto chart = "#00102: 1.5"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures[1].meter == Catch::Approx(1.5));
}

TEST_CASE("Parse old-style bpm changes", "[BmsChartReader]")
{
    const auto chart = "#00103:0011"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures[1].bpmChanges.size() == 2);
    REQUIRE(res.measures[1].bpmChanges[1] == "11");
}

TEST_CASE("Parse new-style bpm changes", "[BmsChartReader]")
{
    const auto chart = "#EXBPM20 120\n#BPMFF 12\n#00108:12"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.measures[1].exBpmChanges.size() == 1);
    REQUIRE(res.measures[1].exBpmChanges[0] == "12");
    REQUIRE(res.exBpms.size() == 2);
    REQUIRE(res.exBpms.find("20") != res.exBpms.end());
    REQUIRE(res.exBpms.find("20")->second == Catch::Approx(120));
    REQUIRE(res.exBpms.find("FF") != res.exBpms.end());
    REQUIRE(res.exBpms.find("FF")->second == Catch::Approx(12));
}

TEST_CASE("Parse WAVXX", "[BmsChartReader]")
{
    const auto chart = "#WAV01 01.wav"s;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto res = reader.readBmsChart(chart);
    REQUIRE(res.wavs.size() == 1);
    REQUIRE(res.wavs.find("01") != res.wavs.end());
    REQUIRE(res.wavs["01"] == "01.wav"s);
}