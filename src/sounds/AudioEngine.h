//
// Created by PC on 04/09/2025.
//

#ifndef RHYTHMGAME_AUDIOENGINE_H
#define RHYTHMGAME_AUDIOENGINE_H
#include <memory>
#include <mutex>
#include <rtaudio/RtAudio.h>
#include <vector>
namespace sounds {
class Sound;

class AudioEngine {
public:
    explicit AudioEngine(RtAudio::Api api);
    void restartStream();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    auto operator=(const AudioEngine&) -> AudioEngine& = delete;
    AudioEngine(AudioEngine&&) = delete;
    auto operator=(AudioEngine&&) -> AudioEngine& = delete;

    // Voice management (called by Sound).
    void registerVoice(const std::shared_ptr<Sound>& voice);
    void unregisterVoice(Sound* voiceRaw);

private:
    static int callback(void* outputBuffer, void* inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void* userData);

    void mix(float* out, unsigned long frames);

    std::vector<std::weak_ptr<Sound>> voices;
    std::mutex voicesMutex;

    RtAudio audio;
    std::jthread defaultDeviceMonitorThread;
    unsigned int currentDefault = 0;
};
} // namespace sounds

#endif // RHYTHMGAME_AUDIOENGINE_H
