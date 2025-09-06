//
// Created by bobini on 15.04.23.
//

#include "SoundBuffer.h"

#include <spdlog/spdlog.h>
#include <sndfile.hh>
#include <QGuiApplication>

sounds::SoundBuffer::SoundBuffer(const std::filesystem::path& filename) {
    // Decode with libsndfile (float conversion directly).
    SndfileHandle sndFile{ filename.string() };
    if (sndFile.error() != 0) {
        spdlog::error("Could not open sound file {}: {}", filename.string(), sf_error_number(sndFile.error()));
        throw std::runtime_error("Could not open sound file");
    }

    if ((sndFile.format() & SF_FORMAT_TYPEMASK) == 0) {
        spdlog::error("Unsupported/unknown format for sound file {}", filename.string());
        throw std::runtime_error("Unsupported sound file format");
    }

    channels   = sndFile.channels();
    sampleRate = sndFile.samplerate();
    frames = sndFile.frames();

    if (channels <= 0 || sampleRate <= 0 || frames <= 0) {
        spdlog::error("Invalid metadata in audio file {} (channels={}, rate={}, frames={})",
                      filename.string(), channels, sampleRate, frames);
        throw std::runtime_error("Invalid audio metadata");
    }

    samples.resize(frames * static_cast<std::size_t>(channels));

    sf_count_t readCount = sndFile.readf(samples.data(), frames);
    if (readCount != frames) {
        spdlog::warn("Short read: expected {} frames, got {} in {}", frames, readCount, filename.string());
        samples.resize(static_cast<std::size_t>(readCount) * static_cast<std::size_t>(channels));
    }

    spdlog::debug("Loaded '{}' ({} frames, {} ch, {} Hz)", filename.string(), readCount, channels, sampleRate);
}
auto
sounds::SoundBuffer::getBuffer() const -> const std::vector<float>&
{
    return samples;
}


auto
sounds::SoundBuffer::getDuration() const -> std::chrono::nanoseconds
{
    if (sampleRate <= 0) return std::chrono::nanoseconds{0};
    const auto seconds = static_cast<long double>(frames) / static_cast<long double>(sampleRate);
    const auto ns = static_cast<std::chrono::nanoseconds::rep>(seconds * 1'000'000'000.0L);
    return std::chrono::nanoseconds{ns};
}

auto
sounds::SoundBuffer::getFrequency() const -> int
{
    return sampleRate;
}

auto
sounds::SoundBuffer::getChannels() const -> int
{
    return channels;
}
auto
sounds::SoundBuffer::getFrames() const -> std::size_t
{
    return frames;
}
