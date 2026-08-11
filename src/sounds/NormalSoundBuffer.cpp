//
// Created by PC on 24/02/2026.
//

#include "NormalSoundBuffer.h"

#include "support/PathToUtfString.h"

#include <spdlog/spdlog.h>
#include <sndfile.hh>

#include <algorithm>
#include <cstring>
#include <string_view>

namespace {

struct MemorySoundFile
{
    QByteArrayView contents;
    sf_count_t position = 0;
};

auto
memoryLength(void* userData) -> sf_count_t
{
    const auto& file = *static_cast<MemorySoundFile*>(userData);
    return static_cast<sf_count_t>(file.contents.size());
}

auto
memorySeek(const sf_count_t offset, const int whence, void* userData)
  -> sf_count_t
{
    auto& file = *static_cast<MemorySoundFile*>(userData);
    const auto length = memoryLength(userData);
    auto target = sf_count_t{};
    switch (whence) {
        case SEEK_SET:
            target = offset;
            break;
        case SEEK_CUR:
            target = file.position + offset;
            break;
        case SEEK_END:
            target = length + offset;
            break;
        default:
            return -1;
    }
    if (target < 0 || target > length) {
        return -1;
    }
    file.position = target;
    return file.position;
}

auto
memoryRead(void* destination, const sf_count_t count, void* userData)
  -> sf_count_t
{
    auto& file = *static_cast<MemorySoundFile*>(userData);
    const auto remaining = memoryLength(userData) - file.position;
    const auto bytes = std::min(count, remaining);
    if (bytes <= 0) {
        return 0;
    }
    std::memcpy(destination,
                file.contents.data() + file.position,
                static_cast<size_t>(bytes));
    file.position += bytes;
    return bytes;
}

auto
memoryWrite(const void*, sf_count_t, void*) -> sf_count_t
{
    return 0;
}

auto
memoryTell(void* userData) -> sf_count_t
{
    return static_cast<MemorySoundFile*>(userData)->position;
}

auto
memoryIo() -> SF_VIRTUAL_IO
{
    return { memoryLength, memorySeek, memoryRead, memoryWrite, memoryTell };
}

void
decode(sounds::AudioEngine* engine,
       SndfileHandle& sndFile,
       std::vector<float>& samples,
       const std::string_view description)
{
    if (sndFile.error() != 0) {
        spdlog::error("Could not open sound {}: {}",
                      description,
                      sf_error_number(sndFile.error()));
        throw std::runtime_error("Could not open sound file");
    }

    if ((sndFile.format() & SF_FORMAT_TYPEMASK) == 0) {
        spdlog::error("Unsupported/unknown format for sound {}", description);
        throw std::runtime_error("Unsupported sound file format");
    }
    const auto sampleRate = sndFile.samplerate();
    const auto frames = sndFile.frames();
    const auto channels = sndFile.channels();
    if (frames <= 0 || channels <= 0) {
        throw std::runtime_error("Sound file contains no audio frames");
    }
    auto samplesOriginal = std::vector<float>(static_cast<size_t>(frames) *
                                              static_cast<size_t>(channels));
    const auto framesRead = sndFile.readf(samplesOriginal.data(), frames);
    if (framesRead == 0) {
        throw std::runtime_error("Could not read all frames from sound file");
    }
    samplesOriginal.resize(static_cast<size_t>(framesRead) *
                           static_cast<size_t>(channels));
    auto convertedSamples = std::vector<float>{};
    const auto engineChannels = static_cast<ma_uint32>(engine->getChannels());
    if (static_cast<ma_uint32>(channels) == engineChannels) {
        convertedSamples = std::move(samplesOriginal);
    } else {
        convertedSamples.resize(static_cast<size_t>(framesRead) *
                                engineChannels);
        auto channelConverterConfig =
          ma_channel_converter_config_init(ma_format_f32,
                                           static_cast<ma_uint32>(channels),
                                           nullptr,
                                           engineChannels,
                                           nullptr,
                                           ma_channel_mix_mode_default);
        auto channelConverter = ma_channel_converter{};
        ma_channel_converter_init(
          &channelConverterConfig, nullptr, &channelConverter);
        ma_channel_converter_process_pcm_frames(
          &channelConverter,
          convertedSamples.data(),
          samplesOriginal.data(),
          static_cast<ma_uint64>(framesRead));
        ma_channel_converter_uninit(&channelConverter, nullptr);
    }
    const auto engineSampleRate =
      static_cast<ma_uint32>(engine->getSampleRate());
    const auto sourceSampleRate = static_cast<ma_uint32>(sampleRate);
    if (sourceSampleRate == engineSampleRate) {
        samples = std::move(convertedSamples);
    } else {
        const auto config =
          ma_resampler_config_init(ma_format_f32,
                                   engineChannels,
                                   sourceSampleRate,
                                   engineSampleRate,
                                   ma_resample_algorithm_linear);

        auto resampler = ma_resampler{};
        const auto result = ma_resampler_init(&config, nullptr, &resampler);
        if (result != MA_SUCCESS) {
            throw std::runtime_error("Could not initialize resampler");
        }
        auto framesIn = static_cast<ma_uint64>(framesRead);
        auto framesOut = ma_uint64{};
        if (ma_resampler_get_expected_output_frame_count(
              &resampler, framesIn, &framesOut) != MA_SUCCESS) {
            ma_resampler_uninit(&resampler, nullptr);
            throw std::runtime_error(
              "Could not determine resampled sound length");
        }
        samples.resize(static_cast<size_t>(framesOut) * engineChannels);
        if (ma_resampler_process_pcm_frames(&resampler,
                                            convertedSamples.data(),
                                            &framesIn,
                                            samples.data(),
                                            &framesOut) != MA_SUCCESS) {
            ma_resampler_uninit(&resampler, nullptr);
            throw std::runtime_error("Could not resample sound");
        }
        samples.resize(static_cast<size_t>(framesOut) * engineChannels);

        ma_resampler_uninit(&resampler, nullptr);
    }
}

} // namespace

sounds::NormalSoundBuffer::NormalSoundBuffer(
  AudioEngine* engine,
  const std::filesystem::path& filename)
{
    // Decode with libsndfile (float conversion directly).
    const auto filenameText = support::pathToUtfString(filename);
#if defined(_WIN32)
    const auto nativeFilename = filename.wstring();
    SndfileHandle sndFile{ nativeFilename.c_str() };
#else
    SndfileHandle sndFile{ filenameText.c_str() };
#endif
    decode(engine, sndFile, samples, filenameText);
}

sounds::NormalSoundBuffer::NormalSoundBuffer(AudioEngine* engine,
                                             const QByteArrayView encoded)
{
    auto file = MemorySoundFile{ encoded };
    auto io = memoryIo();
    auto sndFile = SndfileHandle{ io, &file };
    decode(engine, sndFile, samples, "from memory");
}
auto
sounds::NormalSoundBuffer::getFrames() const -> ma_uint64
{
    return samples.size() / 2;
}
auto
sounds::NormalSoundBuffer::getSamples() const -> std::span<const float>
{
    return samples;
}
