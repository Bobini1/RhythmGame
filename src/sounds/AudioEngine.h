//
// Created by PC on 04/09/2025.
//

#ifndef RHYTHMGAME_AUDIOENGINE_H
#define RHYTHMGAME_AUDIOENGINE_H
#include <miniaudio.h>
namespace sounds {
class Sound;

/**
 * AudioEngine
 * Owns a PortAudio output stream and mixes registered Sound voices.
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    auto operator=(const AudioEngine&) -> AudioEngine& = delete;
    AudioEngine(AudioEngine&&) = delete;
    auto operator=(AudioEngine&&) -> AudioEngine& = delete;

    auto getResourceManager() -> ma_resource_manager* { return &resourceManager; }

    auto getEngine() -> ma_engine* { return &engine; }

    auto getSampleRate() const -> int { return sampleRate; }

    auto getChannels() const -> int { return channels; }

private:
    ma_context context;
    ma_resource_manager_data_source dataSource;
    ma_device device{};
    ma_resource_manager resourceManager{};
    ma_engine engine{};
    int sampleRate;
    int channels;
};
} // namespace sounds

#endif // RHYTHMGAME_AUDIOENGINE_H
