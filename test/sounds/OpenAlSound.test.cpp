//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_test_macros.hpp>

#include "../findTestAssetsFolder.h"
#include "sounds/AudioPlayer.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/SoundBuffer.h"
#include "support/PathToQString.h"

#include <QByteArray>
#include <QFile>

TEST_CASE("OpenAlSound supports formats", "[sounds][FFmpegOpenAlSound]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto engine = sounds::AudioEngine{};
    for (const auto soundFolder =
           findTestAssetsFolder() / "supportedSoundFormats";
         const auto& entry : std::filesystem::directory_iterator(soundFolder)) {
        auto filename = entry.path().string();
        auto sound =
          sounds::NormalSoundBuffer(&engine, std::filesystem::path{ filename });
    }
}

TEST_CASE("NormalSoundBuffer decodes an encoded sound from memory",
          "[sounds][NormalSoundBuffer]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", QByteArrayLiteral("Null"));
    auto engine = sounds::AudioEngine{};
    const auto path = findTestAssetsFolder() / "supportedSoundFormats" /
                      "audiocheck.net_sin_1000Hz_-3dBFS_0.2s_44.1k.ogg";
    auto file = QFile{ support::pathToQString(path) };
    REQUIRE(file.open(QIODevice::ReadOnly));
    const auto encoded = file.readAll();

    const auto fromFile = sounds::NormalSoundBuffer{ &engine, path };
    const auto fromMemory = sounds::NormalSoundBuffer{ &engine, encoded };

    CHECK(fromMemory.getFrames() == fromFile.getFrames());
    CHECK(fromMemory.getSamples().size() == fromFile.getSamples().size());
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
