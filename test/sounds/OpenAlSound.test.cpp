//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QQmlTypeNotAvailable>
#include "../findTestAssetsFolder.h"
#include "sounds/OpenAlSoundBuffer.h"

TEST_CASE("OpenAlSound supports formats", "[sounds][FFmpegOpenAlSound]")
{
    auto root = findTestAssetsFolder();
    auto soundFolder = root / "supportedSoundFormats";
    // load all files in folder
    for (const auto& entry : std::filesystem::directory_iterator(soundFolder)) {
        auto filename = entry.path().string();
        auto sound = sounds::OpenALSoundBuffer(filename.c_str());
    }
}