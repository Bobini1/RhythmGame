//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_test_macros.hpp>

#include "../findTestAssetsFolder.h"
#include "sounds/AudioPlayer.h"
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

TEST_CASE("AudioPlayer distinguishes playback intent from a loaded sound",
          "[sounds][AudioPlayer]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto engine = sounds::AudioEngine{};
    sounds::AudioPlayer::engine = &engine;
    {
        auto player = sounds::AudioPlayer{};

        player.setSource(QStringLiteral("missing-preview.ogg"));
        player.play();

        CHECK(player.isPlaying());
        CHECK_FALSE(player.property("loaded").toBool());

        const auto preview = findTestAssetsFolder() / "supportedSoundFormats" /
                             "audiocheck.net_sin_1000Hz_-3dBFS_0.2s_44.1k.ogg";
        player.setSource(QString::fromStdWString(preview.wstring()));

        CHECK(player.property("loaded").toBool());
    }
    sounds::AudioPlayer::engine = nullptr;
}
