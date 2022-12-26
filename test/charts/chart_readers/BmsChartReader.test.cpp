//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
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
    sol::state lua;
    res.writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getTitle"].call<std::string>() == "END TIME"s);
}

TEST_CASE("Check if Artist is parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    sol::state lua;
    res.writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getArtist"].call<std::string>() == "cres"s);
}

TEST_CASE("Multiple tags at once", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres\n#TITLE END TIME"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    sol::state lua;
    res.writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getArtist"].call<std::string>() == "cres"s);
    REQUIRE(lua["getTitle"].call<std::string>() == "END TIME"s);
}

TEST_CASE("Extra whitespace is ignored", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = " #ARTIST   cres   \n\n #TITLE     END TIME  \n"s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    auto& res = *resReader;
    sol::state lua;
    res.writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getArtist"].call<std::string>() == "cres"s);
    REQUIRE(lua["getTitle"].call<std::string>() == "END TIME"s);
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
    sol::state lua;
    auto writer = charts::behaviour::SongDataWriterToLua{ lua };
    resReader->writeFullData(writer);

    auto difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120"s;
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(writer);
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120.";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120.0F";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 120d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 12E1d";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM 1200E-1f";

    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);

    testString = "#BPM -120.0";
    resReader = reader.readBmsChart(testString);
    REQUIRE(resReader);
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    difference = lua["getBpm"].call<double>() + expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);
}

TEST_CASE("Random blocks get parsed correctly", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString =
      "#RANDOM 5\n#IF 5\n#TITLE 44river\n#BPM 120\n#SUBTITLE testSubTitle\n#GENRE bicior\n#SUBARTIST MC BOBSON\n#ENDIF"s;
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

TEST_CASE("Test return values on failed parse", "[BmsChartReader]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = ""s;
    auto resReader = reader.readBmsChart(testString);
    REQUIRE_FALSE(resReader);

    testString = "#ARTST -44"s;
    auto resReaderTags = reader.readBmsChartTags(testString);
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
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getArtist"].call<std::string>() == "cres"s);
    REQUIRE(lua["getTitle"].call<std::string>() == "END TIME"s);
    auto difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);
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
    resReader->writeFullData(charts::behaviour::SongDataWriterToLua{ lua });
    REQUIRE(lua["getArtist"].call<std::string>() ==
            "LUNEの右手と悠里おねぇちゃんの左脚"s);
    REQUIRE(lua["getTitle"].call<std::string>() ==
            "どうか私を殺して下さい -もう、樹海しか見えない-"s);
    auto difference = lua["getBpm"].call<double>() - expectedBpm;
    REQUIRE(difference > -allowedError);
    REQUIRE(difference < allowedError);
}