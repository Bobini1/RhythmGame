//
// Created by bobini on 05.02.23.
//

#include <catch2/catch_test_macros.hpp>

#include "../findTestAssetsFolder.h"
#include "sounds/AudioPlayer.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/NormalSound.h"
#include "sounds/SoundBuffer.h"
#include "support/PathToQString.h"

#include <QByteArray>
#include <QFile>
#include <QScopeGuard>
#include <latch>
#include <thread>

TEST_CASE("Changing the audio device preserves the mixer and sound cursor",
          "[sounds][AudioEngine]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    REQUIRE(engine.getBackend() == "Null");
    REQUIRE_FALSE(engine.getDeviceNames().isEmpty());
    auto* mixer = engine.getEngine();
    auto* resources = engine.getResourceManager();
    std::vector<float> samples(8192, 0.0f);
    auto config = ma_audio_buffer_config_init(
      ma_format_f32, 2, 4096, samples.data(), nullptr);
    ma_audio_buffer buffer{};
    REQUIRE(ma_audio_buffer_init(&config, &buffer) == MA_SUCCESS);
    const auto freeBuffer =
      qScopeGuard([&] { ma_audio_buffer_uninit(&buffer); });
    ma_sound sound{};
    REQUIRE(ma_sound_init_from_data_source(
              mixer, &buffer, 0, nullptr, &sound) == MA_SUCCESS);
    const auto freeSound = qScopeGuard([&] { ma_sound_uninit(&sound); });
    REQUIRE(ma_sound_seek_to_pcm_frame(&sound, 1234) == MA_SUCCESS);
    ma_sound_set_volume(&sound, 0.25f);
    for (const auto& device : { engine.getDeviceNames().first(), QString{} }) {
        engine.setDevice(device);
        CHECK(engine.getDevice() == device);
        CHECK(engine.getEngine() == mixer);
        CHECK(engine.getResourceManager() == resources);
        ma_uint64 cursor{};
        REQUIRE(ma_sound_get_cursor_in_pcm_frames(&sound, &cursor) ==
                MA_SUCCESS);
        CHECK(cursor == 1234);
        CHECK(ma_sound_get_volume(&sound) == 0.25f);
    }
}

TEST_CASE("Worker-owned chart sounds survive audio device changes",
          "[sounds][AudioEngine]")
{
    qputenv("RHYTHMGAME_AUDIO_BACKEND", "Null");
    sounds::AudioEngine engine;
    REQUIRE_FALSE(engine.getDeviceNames().isEmpty());
    const auto buffer = std::make_shared<sounds::NormalSoundBuffer>(
      &engine,
      findTestAssetsFolder() / "supportedSoundFormats" /
        "audiocheck.net_sin_1000Hz_-3dBFS_0.2s_44.1k.ogg");
    std::latch ready(1);
    std::latch changed(1);
    bool playedAfterChange = false;
    std::jthread worker([&] {
        sounds::NormalSound sound(&engine, buffer);
        ready.count_down();
        changed.wait();
        sound.play();
        playedAfterChange = sound.isPlaying();
        sound.stop();
    });
    ready.wait();
    engine.setDevice(engine.getDeviceNames().first());
    engine.setDevice("");
    changed.count_down();
    worker.join();
    CHECK(playedAfterChange);
}

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
