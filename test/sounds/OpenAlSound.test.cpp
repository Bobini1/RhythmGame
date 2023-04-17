//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "sounds/OpenAlSound.h"
#include "../findTestAssetsFolder.h"
#include "sounds/OpenAlSoundBuffer.h"
#include "resource_managers/SoundLoaderImpl.h"

TEST_CASE("OpenAlSound supports formats", "[sounds][FFmpegOpenAlSound]")
{
    auto root = findTestAssetsFolder();
    auto soundFolder = root / "supportedSoundFormats";
    auto loader = resource_managers::SoundLoaderImpl(soundFolder.string(), {});
    // load all files in folder
    for (const auto& entry : std::filesystem::directory_iterator(soundFolder)) {
        auto filename = entry.path().filename().string();
        auto sound = loader.load(filename);
        INFO("Failed to load sound format: " << filename);
        CHECK(sound.has_value());
        /*if (sound.has_value()) {
            CHECK(sound->getDuration().count() > 0);
            CHECK(sound->getFrequency() > 0);
            CHECK(sound->getChannels() > 0);
        }*/
    }
}