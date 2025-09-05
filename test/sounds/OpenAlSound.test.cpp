//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_test_macros.hpp>

#include "../findTestAssetsFolder.h"
#include "sounds/SoundBuffer.h"

TEST_CASE("OpenAlSound supports formats", "[sounds][FFmpegOpenAlSound]")
{
    for (const auto soundFolder =
           findTestAssetsFolder() / "supportedSoundFormats";
         const auto& entry : std::filesystem::directory_iterator(soundFolder)) {
        auto filename = entry.path().string();
        auto sound = sounds::OpenALSoundBuffer(filename.c_str());
    }
}