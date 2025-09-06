//
// Created by PC on 04/09/2025.
//

#ifndef RHYTHMGAME_AUDIOENGINE_H
#define RHYTHMGAME_AUDIOENGINE_H
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <vector>
namespace sounds {
class Sound;

/**
 * AudioEngine
 * Owns a PortAudio output stream and mixes registered Sound voices.
 */
class AudioEngine {
public:
    struct Config {

    };

    explicit AudioEngine(int sampleRate = 48000, PaDeviceIndex deviceIndex = paNoDevice); // paNoDevice => use default);
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    auto operator=(const AudioEngine&) -> AudioEngine& = delete;
    AudioEngine(AudioEngine&&) = delete;
    auto operator=(AudioEngine&&) -> AudioEngine& = delete;

    auto getSampleRate() const -> int;
    auto getOutputChannels() const -> int;
    auto isRunning() const -> bool;

    // Voice management (called by Sound).
    void registerVoice(const std::shared_ptr<Sound>& voice);
    void unregisterVoice(Sound* voiceRaw);

private:
    static int paCallback(const void* input,
                          void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    void mix(float* out, unsigned long frames);

    std::vector<std::weak_ptr<Sound>> voices;
    std::mutex voicesMutex;

    PaStream* stream = nullptr;
    int sampleRate;
    static constexpr auto outputChannels = 2;
    std::atomic<bool> running{false};
};
} // namespace sounds

#endif // RHYTHMGAME_AUDIOENGINE_H
