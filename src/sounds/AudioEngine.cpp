//
// Created by PC on 04/09/2025.
//

#include "AudioEngine.h"
#include "Sound.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>

namespace sounds {

void errorCallback(RtAudioErrorType type, const std::string& errorText)
{
    if (type == RTAUDIO_WARNING) {
        spdlog::warn("[RtAudio] {}", errorText);
    } else if (type > RTAUDIO_WARNING) {
        spdlog::error("[RtAudio] {}", errorText);
    } else {
        spdlog::info("[RtAudio] {}", errorText);
    }
}

void
AudioEngine::restartStream()
{
    auto outputParams = RtAudio::StreamParameters{};
    outputParams.deviceId = currentDefault;
    outputParams.nChannels = 2; // stereo
    auto info = audio.getDeviceInfo(outputParams.deviceId);
    spdlog::info("Using audio device {}: {}, {} channels, {}Hz - {}",
                 outputParams.deviceId,
                 info.name,
                 info.outputChannels,
                 info.preferredSampleRate,
                 info.isDefaultOutput ? "default" : "");

    auto frames = 0u;
    auto options = RtAudio::StreamOptions{};
    options.flags = RTAUDIO_MINIMIZE_LATENCY;
    auto err = audio.openStream(&outputParams,
                                nullptr,
                                RTAUDIO_FLOAT32,
                                info.preferredSampleRate,
                                &frames,
                                &AudioEngine::callback,
                                this,
                                &options);
    if (err != RTAUDIO_NO_ERROR) {
        throw std::runtime_error("Failed to open audio stream: " +
                                 audio.getErrorText());
    }
    err = audio.startStream();
    if (err != RTAUDIO_NO_ERROR) {
        throw std::runtime_error("Failed to start audio stream: " +
                                 audio.getErrorText());
    }
}
AudioEngine::AudioEngine(RtAudio::Api api)
    : audio(api)
{
    audio.setErrorCallback(&errorCallback);
    for (auto id : audio.getDeviceIds()) {
        try {
            auto info = audio.getDeviceInfo(id);
            spdlog::info("Found audio device {}: {}, {} channels, {}Hz - {}",
                         id,
                         info.name,
                         info.outputChannels,
                         info.preferredSampleRate,
                         info.isDefaultOutput ? "default" : "");
        } catch (const std::exception& e) {
            spdlog::error("Error getting info for device {}: {}", id, e.what());
        }
    }
    currentDefault = audio.getDefaultOutputDevice();
    restartStream();
    defaultDeviceMonitorThread = std::jthread{ [this](std::stop_token st) {
        while (!st.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            auto newDefault = audio.getDefaultOutputDevice();
            if (newDefault != currentDefault) {
                spdlog::info("Audio default output device changed: {} -> {}",
                             currentDefault,
                             newDefault);
                currentDefault = newDefault;
                try {
                    audio.stopStream();
                    audio.closeStream();
                } catch (const std::exception& e) {
                    spdlog::error("Error closing audio stream: {}", e.what());
                }
                try {
                    restartStream();
                } catch (const std::exception& e) {
                    spdlog::error("Error restarting audio stream: {}", e.what());
                }
            }
        }
    } };
}

AudioEngine::~AudioEngine()
{

}


void
AudioEngine::registerVoice(const std::shared_ptr<Sound>& voice)
{
    if (!voice) {
        return;
    }
    std::lock_guard lock(voicesMutex);
    // avoid duplicate raw pointers
    auto* raw = voice.get();
    const auto it =
      std::ranges::find_if(voices, [raw](std::weak_ptr<Sound>& s) {
          return !s.expired() && s.lock().get() == raw;
      });
    if (it == voices.end()) {
        voices.push_back(voice);
    }
}

void
AudioEngine::unregisterVoice(Sound* voice)
{
    if (!voice) {
        return;
    }
    std::lock_guard lock(voicesMutex);
    std::erase_if(voices, [voice](const std::weak_ptr<Sound>& s) {
        if (auto sp = s.lock()) {
            return sp.get() == voice;
        }
        return true; // also purge expired
    });
}

auto
AudioEngine::callback(void* outputBuffer, void* inputBuffer, unsigned int nFrames, double streamTime, RtAudioStreamStatus status, void* userData) -> int
{
    auto data = static_cast<AudioEngine*>(userData);
    data->mix(static_cast<float*>(outputBuffer), nFrames);
    if (status != 0) {
        if (status & RTAUDIO_INPUT_OVERFLOW) {
            spdlog::warn("Audio input overflow");
        }
        if (status & RTAUDIO_OUTPUT_UNDERFLOW) {
            spdlog::warn("Audio output underflow");
        }
    }
    return 0;
}

void
AudioEngine::mix(float* out, unsigned long frames)
{
    std::fill_n(out, frames * 2, 0.0f);

    // Snapshot valid voices
    std::vector<std::shared_ptr<Sound>> active;
    {
        std::lock_guard lock(voicesMutex);
        active.reserve(voices.size());
        for (auto it = voices.begin(); it != voices.end();) {
            if (auto sp = it->lock()) {
                active.push_back(std::move(sp));
                ++it;
            } else {
                it = voices.erase(it);
            }
        }
    }

    if (active.empty()) {
        return;
    }

    // Mix
    for (auto& v : active) {
        v->mixInto(out, frames, 2, audio.getStreamSampleRate());
    }

    // Clamp
    auto total = frames * static_cast<unsigned long>(2);
    for (unsigned long i = 0; i < total; ++i) {
        if (out[i] > 1.0f)
            out[i] = 1.0f;
        else if (out[i] < -1.0f)
            out[i] = -1.0f;
    }
}
}