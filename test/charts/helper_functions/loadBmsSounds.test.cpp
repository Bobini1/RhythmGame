//
// Created by bobini on 18.06.23.
//

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "charts/helper_functions/loadBmsSounds.h"
#include "../../findTestAssetsFolder.h"
#include "charts/chart_readers/BmsChartReader.h"

#include <support/UtfStringToPath.h>

namespace {
auto randomGenerator =
  [](charts::parser_models::ParsedBmsChart::RandomRange range) {
      return range;
  };
} // namespace

TEST_CASE("Sounds are loaded from a folder according to the bms file",
          "[loadBmsSounds]")
{
    auto folder = findTestAssetsFolder() / "supportedSoundFormats";
    auto path =
      std::string("8BIT_audiocheck.net_sin_1000Hz_-3dBFS_0.2s_8.0k.wav");
    const auto bmsFile = fmt::format("#WAV01 {}\n#WAV02 {}", path, path);
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart(bmsFile, randomGenerator).tags;
    std::unordered_map<uint16_t, std::filesystem::path> wavs;
    wavs.reserve(tags.wavs.size());
    for (auto& wav : tags.wavs) {
        wavs.emplace(wav.first, support::utfStringToPath(wav.second));
    }
    auto sounds = charts::helper_functions::loadBmsSounds(wavs, folder);
    REQUIRE(sounds.size() == 2);
    REQUIRE(sounds.at(1).getBuffer() == sounds.at(2).getBuffer());
}

TEST_CASE("Even when the extension says wav, allow loading other extensions",
          "[loadBmsSounds]")
{
    auto folder = findTestAssetsFolder() / "bmsFallbackExtensions";
    auto pathIterator = std::filesystem::directory_iterator(folder);
    auto paths = std::vector<std::string>();
    std::transform(pathIterator,
                   std::filesystem::directory_iterator(),
                   std::back_inserter(paths),
                   [](const auto& entry) {
                       auto path = entry.path();
                       return path.replace_extension("wav").filename().string();
                   });
    const auto bmsFile =
      fmt::format("#WAV01 {}\n#WAV02 {}\n#WAV03 {}\n#WAV04 {}",
                  paths[0],
                  paths[1],
                  paths[2],
                  paths[3]);
    auto reader = charts::chart_readers::BmsChartReader();
    auto tags = reader.readBmsChart(bmsFile, randomGenerator).tags;
    std::unordered_map<uint16_t, std::filesystem::path> wavs;
    wavs.reserve(tags.wavs.size());
    for (auto& wav : tags.wavs) {
        wavs.emplace(wav.first, support::utfStringToPath(wav.second));
    }
    auto sounds = charts::helper_functions::loadBmsSounds(wavs, folder);
    REQUIRE(sounds.size() == 4);
}