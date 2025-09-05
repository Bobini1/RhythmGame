//
// Created by bobini on 14.01.23.
//

#include <stdexcept>
#include <memory>
#include "Sound.h"

#include "AudioEngine.h"
#include "sounds/SoundBuffer.h"
namespace sounds {
Sound::Sound(AudioEngine* engine,
             std::shared_ptr<const SoundBuffer> buffer)
  : engine(engine)
  , buffer(std::move(buffer))
{
    if (this->engine == nullptr) {
        throw std::invalid_argument("Sound constructed with null AudioEngine pointer");
    }
    if (!this->buffer) {
        throw std::invalid_argument("Sound constructed with null SoundBuffer");
    }
}

Sound::~Sound()
{
    unregister();
}

void Sound::ensureRegistered()
{
    if (!registered.load()) {
        engine->registerVoice(shared_from_this());
        registered.store(true);
    }
}

void Sound::unregister()
{
    if (registered.exchange(false)) {
        engine->unregisterVoice(this);
    }
}

void Sound::play()
{
    positionFrames.store(0.0);

    if (!playing) {
        playing = true;
        ensureRegistered();
    }
}

void Sound::stop()
{
    positionFrames.store(0.0);
    if (playing) {
        playing = false;
        unregister();
    }
}

auto Sound::isPlaying() const -> bool
{
    return playing.load();
}

void Sound::setVolume(float v)
{
    volume.store(std::clamp(v, 0.0f, 10.0f));
}

auto Sound::getVolume() const -> float
{
    return volume.load();
}

void Sound::setTimePoint(std::chrono::nanoseconds offset)
{
    auto sr = buffer->getFrequency();
    if (sr <= 0) {
        return;
    }
    auto frame = static_cast<long double>(offset.count()) *
                 static_cast<long double>(sr) / 1'000'000'000.0L;
    auto clamped = std::clamp(static_cast<double>(frame),
                              0.0,
                              static_cast<double>(buffer->getFrames()));
    positionFrames.store(clamped);
}

auto Sound::getTimePoint() const -> std::chrono::nanoseconds
{
    auto sr = buffer->getFrequency();
    if (sr <= 0) {
        return std::chrono::nanoseconds::zero();
    }
    auto frame = positionFrames.load();
    if (frame < 0.0) {
        frame = 0.0;
    }
    auto seconds = static_cast<long double>(frame) /
                   static_cast<long double>(sr);
    auto ns = static_cast<std::chrono::nanoseconds::rep>(seconds * 1'000'000'000.0L);
    return std::chrono::nanoseconds(ns);
}

auto Sound::getFrequency() const -> int
{
    return buffer->getFrequency();
}

auto Sound::getChannels() const -> int
{
    return buffer->getChannels();
}

auto Sound::getDuration() const -> std::chrono::nanoseconds
{
    return buffer->getDuration();
}

auto Sound::getBuffer() const
  -> const std::shared_ptr<const SoundBuffer>&
{
    return buffer;
}

void Sound::mixInto(float* out,
                    unsigned long framesOut,
                    int outChannels,
                    int outSampleRate)
{
    {
        if (!playing.load(std::memory_order_relaxed)) {
            return;
        }
    }

    // Access buffer data
    const auto& samples = buffer->getBuffer();
    const std::size_t srcFrames   = buffer->getFrames();
    const int         srcChannels = buffer->getChannels();
    const int         srcRate     = buffer->getFrequency();

    if (samples.empty() || srcFrames == 0) return;
    if (outChannels <= 0 || outSampleRate <= 0 || srcRate <= 0) return;

    float vol = volume.load(std::memory_order_relaxed);
    if (vol == 0.0f) return;

    // Playback step (no separate per-voice rate shown here)
    double step = static_cast<double>(srcRate) / static_cast<double>(outSampleRate);
    if (!(step > 0.0) || std::isnan(step) || std::isinf(step)) {
        return;
    }

    double pos = positionFrames.load(std::memory_order_relaxed);

    for (unsigned long f = 0; f < framesOut; ++f) {
        if (pos >= static_cast<double>(srcFrames)) {
            positionFrames.store(0.0, std::memory_order_relaxed);
            playing.store(false, std::memory_order_relaxed);
            unregister();
            break;
        }

        const std::size_t ipos = static_cast<std::size_t>(pos);
        const double frac = pos - static_cast<double>(ipos);
        const std::size_t ipos2 = (ipos + 1 < srcFrames) ? (ipos + 1) : ipos;

        // Mix channel(s)
        for (int oc = 0; oc < outChannels; ++oc) {
            float sampleValue = 0.0f;

            if (srcChannels == outChannels) {
                const float* a = &samples[ipos * srcChannels];
                const float* b = &samples[ipos2 * srcChannels];
                const float av = a[oc];
                const float bv = b[oc];
                sampleValue = av + (bv - av) * static_cast<float>(frac);
            } else if (srcChannels == 1 && outChannels == 2) {
                const float av = samples[ipos];
                const float bv = samples[ipos2];
                const float mono = av + (bv - av) * static_cast<float>(frac);
                sampleValue = mono;
            } else if (srcChannels == 2 && outChannels == 1) {
                const float* a = &samples[ipos * 2];
                const float* b = &samples[ipos2 * 2];
                const float l = a[0] + (b[0] - a[0]) * static_cast<float>(frac);
                const float r = a[1] + (b[1] - a[1]) * static_cast<float>(frac);
                sampleValue = 0.5f * (l + r);
            } else {
                // Arbitrary mismatch: just take channel 0 with interpolation
                const float* a = &samples[ipos * srcChannels];
                const float* b = &samples[ipos2 * srcChannels];
                const float av = a[0];
                const float bv = b[0];
                sampleValue = av + (bv - av) * static_cast<float>(frac);
            }

            out[f * outChannels + oc] += sampleValue * vol;
        }

        pos += step;
    }

    positionFrames.store(pos, std::memory_order_relaxed);
}
} // namespace sounds