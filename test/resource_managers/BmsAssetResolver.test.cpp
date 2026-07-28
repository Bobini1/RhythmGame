#include "resource_managers/BmsAssetResolver.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>

using charts::BmsAssetResolver;

namespace {
auto
entries(std::initializer_list<std::string> names)
  -> std::vector<BmsAssetResolver::Entry>
{
    auto result = std::vector<BmsAssetResolver::Entry>{};
    for (const auto& name : names) {
        result.push_back({ .relativeUtf8 = name,
                           .actualPath = std::filesystem::u8path(name) });
    }
    return result;
}
} // namespace

TEST_CASE("BMS resolver applies exact/folded extension precedence",
          "[BmsAssetResolver]")
{
    const auto resolver = BmsAssetResolver{ entries(
      { "sample.ogg", "VOICE.WAV", "voice.ogg", "nested/Key.FLAC" }) };
    REQUIRE(resolver.valid());
    REQUIRE(resolver.resolve("sample.wav")->relativeUtf8 == "sample.ogg");
    REQUIRE(resolver.resolve("voice.wav")->relativeUtf8 == "VOICE.WAV");
    REQUIRE(resolver.resolve("nested\\key.wav")->relativeUtf8 ==
            "nested/Key.FLAC");
}

TEST_CASE("BMS resolver normalizes only paths inside its lexical root",
          "[BmsAssetResolver]")
{
    const auto resolver =
      BmsAssetResolver{ entries({ "dir/sample.ogg", "outside.ogg" }) };
    REQUIRE(resolver.resolve("dir/./child/../sample.wav"));
    REQUIRE_FALSE(resolver.resolve(""));
    REQUIRE_FALSE(resolver.resolve("/dir/sample.wav"));
    REQUIRE_FALSE(resolver.resolve("C:\\dir\\sample.wav"));
    REQUIRE_FALSE(resolver.resolve("\\\\server\\share\\sample.wav"));
    REQUIRE_FALSE(resolver.resolve("../outside.wav"));
    REQUIRE_FALSE(resolver.resolve(std::string("sample\0.wav", 11)));
}

TEST_CASE("BMS resolver uses NFC Unicode case folding", "[BmsAssetResolver]")
{
    const auto resolver =
      BmsAssetResolver{ entries({ "音/CAFÉ.ogg", "音/別.ogg" }) };
    REQUIRE(resolver.resolve("音/cafe\u0301.wav")->relativeUtf8 ==
            "音/CAFÉ.ogg");
}

TEST_CASE("BMS resolver resolves decoded non-ASCII chart names",
          "[BmsAssetResolver][CP932-decoded-path]")
{
    // ReadBmsFile owns CP932 decoding. The resolver receives that decoded
    // UTF-8 spelling and must retain it through extension fallback.
    const auto resolver = BmsAssetResolver{ entries({ "音/龍の鍵.ogg" }) };
    REQUIRE(resolver.resolve("音\\龍の鍵.wav")->relativeUtf8 ==
            "音/龍の鍵.ogg");
}

TEST_CASE("BMS resolver diagnoses folded collisions deterministically",
          "[BmsAssetResolver]")
{
    const auto resolver =
      BmsAssetResolver{ entries({ "Z/CAFÉ.ogg", "z/cafe\u0301.ogg" }) };
    REQUIRE_FALSE(resolver.valid());
    REQUIRE(resolver.diagnostic() ==
            "Unicode case-fold collision: Z/CAFÉ.ogg, z/café.ogg");
    REQUIRE_FALSE(resolver.resolve("z/café.wav"));
}

TEST_CASE("BMS resolver never sees ambient files omitted from its index",
          "[BmsAssetResolver]")
{
    const auto resolver = BmsAssetResolver{ entries({ "indexed.ogg" }) };
    REQUIRE_FALSE(resolver.resolve("CMakeLists.txt"));
    REQUIRE_FALSE(resolver.resolve("not-indexed.wav"));
    const auto missing = BmsAssetResolver::fromDirectory(
      std::filesystem::temp_directory_path() /
      "rhythmgame-resolver-definitely-missing-root");
    REQUIRE_FALSE(missing.valid());
    REQUIRE(missing.diagnostic() == "BMS asset directory scan failed");
}

TEST_CASE("BMS resolver indexes real fallback assets once",
          "[BmsAssetResolver][integration]")
{
    const auto root = std::filesystem::path(RHYTHMGAME_SOURCE_DIR) /
                      "testOnlyAssets/bmsFallbackExtensions";
    const auto resolver = BmsAssetResolver::fromDirectory(root);
    REQUIRE(resolver.valid());
    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        auto declared = entry.path().filename();
        declared.replace_extension(".wav");
        const auto declaredUtf8 = declared.generic_u8string();
        const auto resolved = resolver.resolve(
          std::string(reinterpret_cast<const char*>(declaredUtf8.data()),
                      declaredUtf8.size()));
        REQUIRE(resolved);
        REQUIRE(resolved->actualPath == entry.path());
    }
}

TEST_CASE("BMS resolver retains actual mixed-case indexed path",
          "[BmsAssetResolver][integration]")
{
    auto root = std::filesystem::temp_directory_path() /
                "rhythmgame-bms-resolver-mixed-case";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Nested");
    {
        auto file = std::ofstream(root / "Nested" / "MiXeD.ogg");
        file << "fixture";
    }
    const auto resolver = BmsAssetResolver::fromDirectory(root);
    REQUIRE(resolver.resolve("nested\\mixed.wav")->actualPath ==
            root / "Nested" / "MiXeD.ogg");
    std::filesystem::remove_all(root);
}

TEST_CASE("Real Dstorv WAV declarations resolve from its immutable index",
          "[BmsAssetResolver][Dstorv]")
{
    const auto* rootEnvironment = std::getenv("RHYTHMGAME_DSTORV_ROOT");
    if (rootEnvironment == nullptr) {
        SKIP("RHYTHMGAME_DSTORV_ROOT is not available");
    }
    const auto root = std::filesystem::path(rootEnvironment);
    const auto resolver = BmsAssetResolver::fromDirectory(root);
    REQUIRE(resolver.valid());
    auto chart = std::ifstream(root / "Dstorv_act1_evo.bme", std::ios::binary);
    REQUIRE(chart.good());
    auto declarations = std::size_t{};
    auto fallbackToOgg = std::size_t{};
    auto line = std::string{};
    while (std::getline(chart, line)) {
        if (line.size() < 8 || !line.starts_with("#WAV")) {
            continue;
        }
        const auto separator = line.find_first_of(" \t", 6);
        if (separator == std::string::npos) {
            continue;
        }
        const auto valueStart = line.find_first_not_of(" \t", separator);
        if (valueStart == std::string::npos) {
            continue;
        }
        auto declared = line.substr(valueStart);
        if (!declared.empty() && declared.back() == '\r') {
            declared.pop_back();
        }
        const auto resolved = resolver.resolve(declared);
        REQUIRE(resolved);
        ++declarations;
        const auto declaredExtension =
          std::filesystem::path(declared).extension().string();
        if (declaredExtension == ".wav" &&
            resolved->actualPath.extension() == ".ogg") {
            ++fallbackToOgg;
        }
    }
    REQUIRE(declarations == 630);
    REQUIRE(fallbackToOgg == declarations);
}
