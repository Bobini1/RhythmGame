//
// Created by bobini on 11.07.2022.
//

#include <catch2/catch_test_macros.hpp>
#include "charts/chart_readers/BmsChartReader.h"
#include <string>
#include <iostream>
#include <regex>
#include <sstream>

TEST_CASE("Check if Title is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.getTitle() == "END TIME"s);
}

TEST_CASE("Check if Artist is parsed correctly", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.getArtist() == "cres"s);
}

TEST_CASE("Multiple tags at once", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = "#ARTIST cres\n#TITLE END TIME"s;
    auto res = reader.readBmsChart(testString);
    REQUIRE(res.getArtist() == "cres"s);
    REQUIRE(res.getTitle() == "END TIME"s);
}

std::vector<std::string>
split(const std::string& s, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

TEST_CASE("Extra whitespace is ignored", "[single-file]")
{
    using namespace std::literals::string_literals;
    auto reader = charts::chart_readers::BmsChartReader{};
    auto testString = " #ARTIST   cres   \n\n #TITLE     END TIME  \n"s;
    // measure time
    auto start = std::chrono::high_resolution_clock::now();

    auto res = reader.readBmsChart(testString);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
        .count();
    std::cout << "Duration: " << duration << " microseconds" << std::endl;
    REQUIRE(res.getArtist() == "cres"s);
    REQUIRE(res.getTitle() == "END TIME"s);
}