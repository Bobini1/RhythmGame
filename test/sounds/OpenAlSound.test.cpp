//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_test_macros.hpp>

#include "../findTestAssetsFolder.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/SoundBuffer.h"

#include <QByteArray>

TEST_CASE("OpenAlSound supports formats", "[sounds][FFmpegOpenAlSound]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto engine = sounds::AudioEngine{};
    for (const auto soundFolder =
           findTestAssetsFolder() / "supportedSoundFormats";
         const auto& entry : std::filesystem::directory_iterator(soundFolder)) {
        auto filename = entry.path().string();
        auto sound = sounds::NormalSoundBuffer(&engine, filename.c_str());
    }
}
