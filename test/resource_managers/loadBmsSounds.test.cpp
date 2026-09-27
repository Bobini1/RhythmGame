//
// Created by bobini on 18.06.23.
//

#include <fmt/format.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "resource_managers/loadBmsSounds.h"
#include "../findTestAssetsFolder.h"
#include "charts/ReadBmsFile.h"
#include "sounds/AudioEngine.h"
#include "sounds/NormalSound.h"
#include "sounds/NormalSoundBuffer.h"

#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>

#include <atomic>

#include <support/PathToQString.h>
#include <support/UtfStringToPath.h>
#include <support/QStringToPath.h>

// Exercise the case-sensitive filesystem resolver on Windows builds too.
namespace charts {
auto
createLowerCaseFilesMap(std::filesystem::path directory)
  -> std::unordered_map<std::string, std::filesystem::path>;
auto
getActualPath(
  const std::unordered_map<std::string, std::filesystem::path>& files,
  const std::filesystem::path& path) -> std::optional<std::filesystem::path>;
}

TEST_CASE(
  "Case-sensitive sound lookup keeps directories and handles short paths",
  "[loadBmsSounds]")
{
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto root = support::qStringToPath(directory.path());
    std::filesystem::create_directories(root / "samples");
    std::filesystem::create_directories(root / "other");
    for (const auto* path : { "samples/kick.wav", "other/kick.wav" }) {
        QFile file(directory.filePath(path));
        REQUIRE(file.open(QIODevice::WriteOnly));
    }
    const auto files = charts::createLowerCaseFilesMap(root);
    CHECK(charts::getActualPath(files, "./SAMPLES\\kick.WAV") ==
          root / "samples/kick.wav");
    CHECK(charts::getActualPath(files, "other/KICK.flac") ==
          root / "other/kick.wav");
    CHECK(charts::getActualPath(files, "samples/kick") ==
          root / "samples/kick.wav");
    for (const auto* missing : { "", "a", "ab", "kick.wav" }) {
        CAPTURE(missing);
        CHECK_FALSE(charts::getActualPath(files, missing));
    }
}

namespace {
auto randomGenerator = [](charts::ParsedBmsChart::RandomRange range) {
    return range;
};
} // namespace

TEST_CASE("Sounds are loaded from a folder according to the bms file",
          "[loadBmsSounds]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto folder = findTestAssetsFolder() / "supportedSoundFormats";
    auto path =
      std::string("8BIT_audiocheck.net_sin_1000Hz_-3dBFS_0.2s_8.0k.wav");
    const auto bmsFile = fmt::format("#WAV01 {}\n#WAV02 {}", path, path);
    auto tags = charts::readBmsChart(bmsFile, randomGenerator).tags;
    std::unordered_map<uint64_t, std::filesystem::path> wavs;
    wavs.reserve(tags.wavs.size());
    for (auto& wav : tags.wavs) {
        wavs.emplace(wav.first, support::utfStringToPath(wav.second));
    }
    auto engine = sounds::AudioEngine{};
    auto sounds = charts::loadBmsSounds(&engine, wavs, folder);
    REQUIRE(sounds.size() == 2);
    auto sound1 = std::dynamic_pointer_cast<sounds::NormalSound>(sounds.at(1));
    REQUIRE(sound1 != nullptr);
    auto sound2 = std::dynamic_pointer_cast<sounds::NormalSound>(sounds.at(2));
    REQUIRE(sound2 != nullptr);
    REQUIRE(sound1->getBuffer() == sound2->getBuffer());
}

TEST_CASE("Sounds are loaded from encoded archive entries in memory",
          "[loadBmsSounds]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    const auto path = findTestAssetsFolder() / "supportedSoundFormats" /
                      "audiocheck.net_sin_1000Hz_-3dBFS_0.2s_44.1k.ogg";
    auto file = QFile{ support::pathToQString(path) };
    REQUIRE(file.open(QIODevice::ReadOnly));
    auto encoded = std::make_shared<const QByteArray>(file.readAll());
    const auto wavs = charts::EncodedSounds{ { 1, encoded }, { 2, encoded } };

    auto engine = sounds::AudioEngine{};
    auto loaded = charts::loadBmsSounds(&engine, wavs);

    REQUIRE(loaded.size() == 2);
    const auto first =
      std::dynamic_pointer_cast<sounds::NormalSound>(loaded.at(1));
    const auto second =
      std::dynamic_pointer_cast<sounds::NormalSound>(loaded.at(2));
    REQUIRE(first);
    REQUIRE(second);
    CHECK(first->getBuffer() == second->getBuffer());
}

TEST_CASE("Cancelled sound loads do not start decoding", "[loadBmsSounds]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    const auto folder = findTestAssetsFolder() / "supportedSoundFormats";
    const auto wavs = std::unordered_map<uint64_t, std::filesystem::path>{
        { 1, "8BIT_audiocheck.net_sin_1000Hz_-3dBFS_0.2s_8.0k.wav" },
    };
    const auto encoded = charts::EncodedSounds{
        { 1, std::make_shared<const QByteArray>("not audio") },
    };
    auto cancellation = std::atomic_bool{ true };
    auto engine = sounds::AudioEngine{};

    CHECK(charts::loadBmsSounds(&engine, wavs, folder, &cancellation).empty());
    CHECK(charts::loadBmsSounds(&engine, encoded, &cancellation).empty());
}

TEST_CASE("Even when the extension says wav, allow loading other extensions",
          "[loadBmsSounds]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
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
    auto tags = charts::readBmsChart(bmsFile, randomGenerator).tags;
    std::unordered_map<uint64_t, std::filesystem::path> wavs;
    wavs.reserve(tags.wavs.size());
    for (auto& wav : tags.wavs) {
        wavs.emplace(wav.first, support::utfStringToPath(wav.second));
    }
    auto engine = sounds::AudioEngine{};
    auto sounds = charts::loadBmsSounds(&engine, wavs, folder);
    REQUIRE(sounds.size() == 4);
}

TEST_CASE("Sound paths preserve directories and safely try other extensions",
          "[loadBmsSounds]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    QTemporaryDir temporary;
    REQUIRE(temporary.isValid());
    const auto root = support::qStringToPath(temporary.path());
    std::filesystem::create_directories(root / "samples");
    std::filesystem::create_directories(root / "other");
    const auto source = findTestAssetsFolder() / "supportedSoundFormats" /
                        "8BIT_audiocheck.net_sin_1000Hz_-3dBFS_0.2s_8.0k.wav";
    std::filesystem::copy_file(source, root / "samples/kick.wav");
    std::filesystem::copy_file(source, root / "other/kick.wav");
    const auto paths = std::unordered_map<uint64_t, std::filesystem::path>{
        { 1, "./SAMPLES\\kick.WAV" },
        { 2, "other/KICK.flac" },
        { 3, "" },
        { 4, "a" },
        { 5, "ab" },
        { 6, "kick.wav" }
    };
    sounds::AudioEngine engine;
    SECTION("BMS")
    {
        const auto loaded = charts::loadBmsSounds(&engine, paths, root);
        REQUIRE(loaded.size() == 2);
        const auto first =
          std::dynamic_pointer_cast<sounds::NormalSound>(loaded.at(1));
        const auto second =
          std::dynamic_pointer_cast<sounds::NormalSound>(loaded.at(2));
        REQUIRE(first);
        REQUIRE(second);
        CHECK(first->getBuffer() != second->getBuffer());
    }
    SECTION("BMSON")
    {
        const auto slices = std::vector<charts::BmsNotesData::BmsonSliceInfo>{
            { 10, 1, 0.0, -1.0 }, { 20, 2, 0.0, -1.0 }, { 30, 3, 0.0, -1.0 },
            { 40, 4, 0.0, -1.0 }, { 50, 5, 0.0, -1.0 }, { 60, 6, 0.0, -1.0 }
        };
        const auto loaded =
          charts::loadBmsonSounds(&engine, paths, slices, {}, root);
        REQUIRE(loaded.size() == 2);
        CHECK(loaded.contains(10));
        CHECK(loaded.contains(20));
    }
}
