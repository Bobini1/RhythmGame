//
// Created by bobini on 15.04.23.
//

#include "SoundBuffer.h"

#include <spdlog/spdlog.h>
#include <sndfile.hh>
#include <QGuiApplication>
#include <miniaudio.h>

sounds::SoundBuffer::SoundBuffer(AudioEngine* engine,
                                 const std::filesystem::path& filename)
{
    // Decode with libsndfile (float conversion directly).
    SndfileHandle sndFile{ filename.string() };
    if (sndFile.error() != 0) {
        spdlog::error("Could not open sound file {}: {}",
                      filename.string(),
                      sf_error_number(sndFile.error()));
        throw std::runtime_error("Could not open sound file");
    }

    if ((sndFile.format() & SF_FORMAT_TYPEMASK) == 0) {
        spdlog::error("Unsupported/unknown format for sound file {}",
                      filename.string());
        throw std::runtime_error("Unsupported sound file format");
    }
    auto sampleRate = sndFile.samplerate();
    auto frames = sndFile.frames();
    auto channels = sndFile.channels();
    auto samplesOriginal = std::vector<float>(frames * channels);
    auto framesRead = sndFile.readf(samplesOriginal.data(), frames);
    if (framesRead != frames) {
        throw std::runtime_error("Could not read all frames from sound file");
    }
    auto convertedSamples = std::vector<float>{};
    if (channels == 2) {
        convertedSamples = std::move(samplesOriginal);
    } else {
        convertedSamples.resize(frames * 2);
        ma_channel_converter_config channelConverterConfig =
          ma_channel_converter_config_init(ma_format_f32,
                                           sndFile.channels(),
                                           nullptr,
                                           2,
                                           nullptr,
                                           ma_channel_mix_mode_default);
        ma_channel_converter channelConverter;
        ma_channel_converter_init(
          &channelConverterConfig, nullptr, &channelConverter);
        ma_channel_converter_process_pcm_frames(&channelConverter,
                                                convertedSamples.data(),
                                                samplesOriginal.data(),
                                                frames);
        ma_channel_converter_uninit(&channelConverter, nullptr);
    }
    if (sampleRate == 44100) {
        samples = std::move(convertedSamples);
    } else {
        ma_resampler_config config = ma_resampler_config_init(
          ma_format_f32, 2, sampleRate, 44100, ma_resample_algorithm_linear);

        ma_resampler resampler;
        const auto result = ma_resampler_init(&config, NULL, &resampler);
        if (result != MA_SUCCESS) {
            throw std::runtime_error("Could not initialize resampler");
        }
        auto framesIn = static_cast<ma_uint64>(frames);
        auto framesOut = ma_uint64(frames * 44100 / sampleRate);
        samples.resize(framesOut * 2);
        ma_resampler_process_pcm_frames(&resampler,
                                        convertedSamples.data(),
                                        &framesIn,
                                        samples.data(),
                                        &framesOut);

        ma_resampler_uninit(&resampler, nullptr);
    }
}
auto
sounds::SoundBuffer::getFrames() const -> ma_uint64
{
    return samples.size() / 2;
}
auto
sounds::SoundBuffer::getSamples() const -> const std::vector<float>&
{
    return samples;
}
