//
// Created by PC on 04/09/2025.
//

#include "AudioEngine.h"
#include "Sound.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>

namespace sounds {
namespace {
// PortAudio global init ref counting
std::atomic_int paRefCount{ 0 };
std::mutex paInitMutex;

void
retainPortAudio()
{
    std::lock_guard lock(paInitMutex);
    if (paRefCount.fetch_add(1) == 0) {
        if (auto err = Pa_Initialize(); err != paNoError) {
            paRefCount.store(0);
            throw std::runtime_error(std::string("Pa_Initialize failed: ") +
                                     Pa_GetErrorText(err));
        }
    }
}

void
releasePortAudio()
{
    std::lock_guard lock(paInitMutex);
    int prev = paRefCount.fetch_sub(1);
    if (prev == 1) {
        if (auto err = Pa_Terminate(); err != paNoError) {
            spdlog::warn("Pa_Terminate error: {}", Pa_GetErrorText(err));
        }
    } else if (prev <= 0) {
        paRefCount.store(0);
    }
}
} // namespace

AudioEngine::AudioEngine(int sampleRate, PaDeviceIndex deviceIndex)
  : sampleRate(sampleRate)
{
    retainPortAudio();

    PaStreamParameters outParams{};
    outParams.device =
      (deviceIndex == paNoDevice) ? Pa_GetDefaultOutputDevice() : deviceIndex;
    if (outParams.device == paNoDevice) {
        releasePortAudio();
        throw std::runtime_error("No output device available.");
    }
    outParams.channelCount = outputChannels;
    outParams.sampleFormat = paFloat32;
    constexpr auto bufferSize = paFramesPerBufferUnspecified;
    auto* info = Pa_GetDeviceInfo(outParams.device);
    outParams.suggestedLatency = info ? info->defaultLowOutputLatency : 0.1;
    outParams.hostApiSpecificStreamInfo = nullptr;

    if (auto err = Pa_OpenStream(&stream,
                                 nullptr,
                                 &outParams,
                                 sampleRate,
                                 bufferSize,
                                 paNoFlag,
                                 &AudioEngine::paCallback,
                                 this);
        err != paNoError) {
        releasePortAudio();
        throw std::runtime_error(std::string("Pa_OpenStream failed: ") +
                                 Pa_GetErrorText(err));
    }

    if (auto err = Pa_StartStream(stream); err != paNoError) {
        Pa_CloseStream(stream);
        stream = nullptr;
        releasePortAudio();
        throw std::runtime_error(std::string("Pa_StartStream failed: ") +
                                 Pa_GetErrorText(err));
    }

    running.store(true);
    spdlog::info(
      "AudioEngine started ({} Hz, {} ch)", sampleRate, outputChannels);
}

AudioEngine::~AudioEngine()
{
    running.store(false);
    {
        std::lock_guard lock(voicesMutex);
        voices.clear();
    }
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    releasePortAudio();
}

auto
AudioEngine::getSampleRate() const -> int
{
    return sampleRate;
}

auto
AudioEngine::getOutputChannels() const -> int
{
    return outputChannels;
}

auto
AudioEngine::isRunning() const -> bool
{
    return running.load();
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
AudioEngine::paCallback(const void*,
                        void* output,
                        unsigned long frameCount,
                        const PaStreamCallbackTimeInfo*,
                        PaStreamCallbackFlags,
                        void* userData) -> int
{
    auto* eng = static_cast<AudioEngine*>(userData);
    auto* out = static_cast<float*>(output);
    eng->mix(out, frameCount);
    return paContinue;
}

void
AudioEngine::mix(float* out, unsigned long frames)
{
    std::fill_n(out, frames * outputChannels, 0.0f);

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
        v->mixInto(out, frames, outputChannels, sampleRate);
    }

    // Clamp
    auto total = frames * static_cast<unsigned long>(outputChannels);
    for (unsigned long i = 0; i < total; ++i) {
        if (out[i] > 1.0f)
            out[i] = 1.0f;
        else if (out[i] < -1.0f)
            out[i] = -1.0f;
    }
}
}