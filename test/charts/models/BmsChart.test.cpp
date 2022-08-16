#include <catch2/catch_test_macros.hpp>
#include "charts/models/BmsChart.h"
#include <sol/sol.hpp>

TEST_CASE("Check if incomplete tags are parsed correctly", "[BmsChart]")
{
    using namespace std::literals::string_literals;
    sol::state lua;
    charts::behaviour::SongDataWriter writer(lua);
    charts::models::BmsChart::Tags tags;
    tags.title = "END TIME"s;
    tags.artist = "cres"s;
    tags.genre = "ReQuiEm tRanCe"s;
    charts::models::BmsChart chart(std::move(tags));
    chart.writeFullData(writer);
    REQUIRE(lua["getTitle"].call<std::string>() == "END TIME"s);
    REQUIRE(lua["getArtist"].call<std::string>() == "cres"s);
    REQUIRE(lua["getGenre"].call<std::string>() == "ReQuiEm tRanCe"s);
    REQUIRE_FALSE(lua["getBpm"].valid());
    REQUIRE_FALSE(lua["getSubTitle"].valid());
    REQUIRE_FALSE(lua["getSubArtist"].valid());
}

TEST_CASE("Check if unicode is parsed correctly", "[BmsChart]")
{
    using namespace std::literals::string_literals;
    sol::state lua;
    charts::behaviour::SongDataWriter writer(lua);
    charts::models::BmsChart::Tags tags;
    tags.title = "😀"s;
    tags.artist = "🤣"s;
    tags.genre = "😇"s;
    tags.subTitle = "日本語"s;
    tags.subArtist = "愛してる"s;
    charts::models::BmsChart chart(std::move(tags));
    chart.writeFullData(writer);
    REQUIRE(lua["getTitle"].call<std::string>() == "😀"s);
    REQUIRE(lua["getArtist"].call<std::string>() == "🤣"s);
    REQUIRE(lua["getGenre"].call<std::string>() == "😇"s);
    REQUIRE_FALSE(lua["getBpm"].valid());
    REQUIRE(lua["getSubTitle"].call<std::string>() == "日本語"s);
    REQUIRE(lua["getSubArtist"].call<std::string>() == "愛してる"s);
}

TEST_CASE("Check no tags provided scenario", "[BmsChart]")
{
    using namespace std::literals::string_literals;
    sol::state lua;
    charts::behaviour::SongDataWriter writer(lua);
    charts::models::BmsChart::Tags tags;
    charts::models::BmsChart chart(std::move(tags));
    chart.writeFullData(writer);
    REQUIRE_FALSE(lua["getTitle"].valid());
    REQUIRE_FALSE(lua["getArtist"].valid());
    REQUIRE_FALSE(lua["getGenre"].valid());
    REQUIRE_FALSE(lua["getBpm"].valid());
    REQUIRE_FALSE(lua["getSubTitle"].valid());
    REQUIRE_FALSE(lua["getSubArtist"].valid());
}