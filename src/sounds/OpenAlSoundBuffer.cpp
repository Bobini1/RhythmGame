//
// Created by bobini on 15.04.23.
//

#include "OpenAlSoundBuffer.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include <AL/alc.h>
#include <AL/alext.h>
#include <stdexcept>
#include <spdlog/spdlog.h>
#include <span>

auto
getALFormat(AVSampleFormat AVFormat, int channels) -> ALenum
{
    switch (channels) {
        case 1:
            switch (AVFormat) {
                case AV_SAMPLE_FMT_U8:
                    return AL_FORMAT_MONO8;
                case AV_SAMPLE_FMT_S16:
                    return AL_FORMAT_MONO16;
                case AV_SAMPLE_FMT_FLT:
                case AV_SAMPLE_FMT_S32:
                    return AL_FORMAT_MONO_FLOAT32;
                case AV_SAMPLE_FMT_DBL:
                    return AL_FORMAT_MONO_DOUBLE_EXT;
                default:
                    break;
            }
        case 2:
            switch (AVFormat) {
                case AV_SAMPLE_FMT_U8:
                case AV_SAMPLE_FMT_U8P:
                    return AL_FORMAT_STEREO8;
                case AV_SAMPLE_FMT_S16:
                case AV_SAMPLE_FMT_S16P:
                    return AL_FORMAT_STEREO16;
                case AV_SAMPLE_FMT_FLT:
                case AV_SAMPLE_FMT_FLTP:
                case AV_SAMPLE_FMT_S32:
                case AV_SAMPLE_FMT_S32P:
                    return AL_FORMAT_STEREO_FLOAT32;
                case AV_SAMPLE_FMT_DBL:
                case AV_SAMPLE_FMT_DBLP:
                    return AL_FORMAT_STEREO_DOUBLE_EXT;
                default:
                    break;
            }
        default:
            break;
    }
    throw std::runtime_error("Unsupported format");
}

auto
setSampleFormat(AVCodecContext& context, const AVCodec& codec) -> AVSampleFormat
{
    if (codec.type != AVMEDIA_TYPE_AUDIO || codec.sample_fmts == nullptr) {
        throw std::runtime_error("Could not decode audio");
    }
    const AVSampleFormat* fmt{};

    context.request_sample_fmt = AV_SAMPLE_FMT_NONE;

    // we look for a non-planar format first
    // this is because OpenAL does not support planar formats
    for (fmt = codec.sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++) {
        // OpenAL does not support 32-bit integer formats
        if (!av_sample_fmt_is_planar(*fmt) && *fmt != AV_SAMPLE_FMT_S32) {
            context.request_sample_fmt = *fmt;
            break;
        }
    }

    if (context.request_sample_fmt == AV_SAMPLE_FMT_NONE) {

        // we failed to find a non-planar format
        // we can still work with planar,
        // but it will require extra work to convert it.
        for (fmt = codec.sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++) {
            if (av_sample_fmt_is_planar(*fmt) && *fmt != AV_SAMPLE_FMT_S32P) {
                context.request_sample_fmt = *fmt;
                break;
            }
        }
    }

    if (context.request_sample_fmt == AV_SAMPLE_FMT_NONE) {
        // last resort, using 32-bit integer
        // this will require manual casting of every single sample
        for (fmt = codec.sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++) {
            if (*fmt == AV_SAMPLE_FMT_S32 || *fmt == AV_SAMPLE_FMT_S32P) {
                context.request_sample_fmt = *fmt;
                break;
            }
        }
    }

    if (context.request_sample_fmt == AV_SAMPLE_FMT_NONE) {
        throw std::runtime_error("Could not decode audio");
    }
    context.sample_fmt = context.request_sample_fmt;
    return context.request_sample_fmt;
}

void
deletePacket(AVPacket* packet)
{
    av_packet_free(&packet);
}

void
deleteFrame(AVFrame* frame)
{
    av_frame_free(&frame);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "cppcoreguidelines-pro-type-reinterpret-cast"
#pragma ide diagnostic ignored "cppcoreguidelines-pro-bounds-pointer-arithmetic"
void
copyInterleaved(const size_t bytesPerSample,
                const size_t channels,
                std::vector<unsigned char>& samples,
                const AVFrame& frame,
                const size_t frameSampleCount)
{
    auto frameData =
      std::span(frame.data[0], frameSampleCount * channels * bytesPerSample);
    std::copy(
      frameData.begin(), frameData.end(), samples.end() - frameData.size());
}
void
copyInterleavedInt32(const size_t sampleCount,
                     const size_t bytesPerSample,
                     const size_t channels,
                     std::vector<unsigned char>& samples,
                     const AVFrame& frame,
                     const size_t frameSampleCount)
{
    auto frameData = std::span(reinterpret_cast<int32_t*>(frame.data[0]),
                               frameSampleCount * channels);
    auto samplesData = std::span(
      reinterpret_cast<float*>(samples.data() + samples.size() -
                               frameSampleCount * channels * bytesPerSample),
      sampleCount);
    std::transform(frameData.begin(),
                   frameData.end(),
                   samplesData.begin(),
                   [](int32_t sample) {
                       return static_cast<float>(sample) / 2147483648.0F;
                   });
}
#pragma clang diagnostic pop

// Decoding interleaved formats is easy. Just copy stuff over to the buffer.
void
decodeInterleaved(size_t& sampleCount,
                  const size_t bytesPerSample,
                  const size_t channels,
                  std::vector<unsigned char>& samples,
                  AVFrame& frame)
{
    const auto frameSampleCount = static_cast<size_t>(frame.nb_samples);
    const auto isInt32 = frame.format == AV_SAMPLE_FMT_S32;
    sampleCount += frameSampleCount * channels;
    samples.resize(sampleCount * bytesPerSample);
    if (isInt32) {
        copyInterleavedInt32(sampleCount,
                             bytesPerSample,
                             channels,
                             samples,
                             frame,
                             frameSampleCount);
    } else {
        copyInterleaved(
          bytesPerSample, channels, samples, frame, frameSampleCount);
    }
}

// Decoding planar formats is a bit more complicated.
// We need to manually interleave samples in the output buffer.
// This is because OpenAL does not support planar formats.
void
decodePlanar(size_t& sampleCount,
             const size_t bytesPerSample,
             const size_t channels,
             std::vector<unsigned char>& samples,
             AVFrame& frame)
{
    const auto frameSampleCount = static_cast<size_t>(frame.nb_samples);
    const auto isInt32 = frame.format == AV_SAMPLE_FMT_S32P;
    sampleCount += frameSampleCount * channels;
    samples.resize(sampleCount * bytesPerSample);
    auto currentSamples = std::span(samples).subspan(
      samples.size() - frameSampleCount * channels * bytesPerSample);
    auto currentOffset = 0U;
    for (size_t i = 0; i < channels; i++) {
        auto frameData =
          std::span(frame.data[i], frameSampleCount * bytesPerSample);
        for (size_t j = 0; j < frameSampleCount; j++) {
            if (isInt32) {
                auto sample = reinterpret_cast<int32_t*>(frameData.data())[j];
                reinterpret_cast<float*>(
                  currentSamples.data())[currentOffset + j * channels] =
                  static_cast<float>(sample) / 2147483648.0F;
            } else {
                std::copy(frameData.begin() + j * bytesPerSample,
                          frameData.begin() + (j + 1) * bytesPerSample,
                          currentSamples.begin() + currentOffset +
                            j * channels * bytesPerSample);
            }
        }
        currentOffset += bytesPerSample;
    }
}

auto
decodeFile(AVFormatContext& formatContext,
           const AVStream& audioStream,
           AVCodecContext& codecContext) -> std::vector<unsigned char>
{
    size_t sampleCount = 0;
    const auto bytesPerSample =
      static_cast<size_t>(av_get_bytes_per_sample(codecContext.sample_fmt));
    const auto channels = static_cast<size_t>(codecContext.channels);

    std::vector<unsigned char> samples;
    std::unique_ptr<AVPacket, decltype(&deletePacket)> packet = {
        av_packet_alloc(), &deletePacket
    };
    std::unique_ptr<AVFrame, decltype(&deleteFrame)> frame = { av_frame_alloc(),
                                                               &deleteFrame };
    while (av_read_frame(&formatContext, packet.get()) >= 0) {
        if (packet->stream_index == audioStream.index) {
            avcodec_send_packet(&codecContext, packet.get());
            while (avcodec_receive_frame(&codecContext, frame.get()) == 0) {
                if (av_sample_fmt_is_planar(codecContext.sample_fmt)) {
                    decodePlanar(
                      sampleCount, bytesPerSample, channels, samples, *frame);
                } else {
                    decodeInterleaved(
                      sampleCount, bytesPerSample, channels, samples, *frame);
                }
            }
        }
        av_packet_unref(packet.get());
    }
    return samples;
}
auto
getALDevice() -> ALCdevice*
{
    static auto device = std::unique_ptr<ALCdevice, decltype(&alcCloseDevice)>(
      alcOpenDevice(nullptr), &alcCloseDevice);
    return device.get();
}
auto
getALContext() -> ALCcontext*
{
    static auto context =
      std::unique_ptr<ALCcontext, decltype(&alcDestroyContext)>(
        alcCreateContext(getALDevice(), nullptr), &alcDestroyContext);
    return context.get();
}

void
deleteFormatContext(AVFormatContext* formatContext)
{
    avformat_close_input(&formatContext);
}

auto
createFormatContext(const char* filename)
  -> std::unique_ptr<AVFormatContext, decltype(&deleteFormatContext)>
{
    auto* formatContext = (AVFormatContext*)nullptr;
    if (avformat_open_input(&formatContext, filename, nullptr, nullptr)) {
        throw std::runtime_error("Could not open file " +
                                 std::string(filename));
    }
    return { formatContext, &deleteFormatContext };
}

void
deleteCodecContext(AVCodecContext* codecContext)
{
    avcodec_free_context(&codecContext);
};

auto
createCodecContext(const AVCodec* codec)
  -> std::unique_ptr<AVCodecContext, decltype(&deleteCodecContext)>
{
    auto* codecContext = avcodec_alloc_context3(codec);
    if (codecContext == nullptr) {
        throw std::runtime_error("Could not allocate codec context");
    }
    return { codecContext, &deleteCodecContext };
}

sounds::OpenALSoundBuffer::OpenALSoundBuffer(const char* filename)
{
    auto formatContext = createFormatContext(filename);
    if (avformat_find_stream_info(formatContext.get(), nullptr) < 0) {
        throw std::runtime_error("Could not find stream info");
    }
    const auto audioStreamIndex = av_find_best_stream(
      formatContext.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audioStreamIndex < 0) {
        throw std::runtime_error("Could not find audio stream");
    }
    const auto* audioStream = formatContext->streams[audioStreamIndex];
    const auto* codecParameters = audioStream->codecpar;
    const auto* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        throw std::runtime_error("Unsupported codec");
    }
    auto codecContext = createCodecContext(codec);

    if (avcodec_parameters_to_context(codecContext.get(), codecParameters) <
        0) {
        throw std::runtime_error("Couldn't copy codec context");
    }
    if (avcodec_open2(codecContext.get(), codec, nullptr) < 0) {
        throw std::runtime_error("Could not open codec");
    }
    auto format = setSampleFormat(*codecContext, *codec);
    spdlog::info("Format: {}", formatContext->iformat->long_name);
    spdlog::info("Sample format: {}", av_get_sample_fmt_name(format));
    spdlog::info("Sample rate: {}", codecContext->sample_rate);
    spdlog::info("Channels: {}", codecContext->channels);

    auto samples = decodeFile(*formatContext, *audioStream, *codecContext);
    alcMakeContextCurrent(getALContext());
    // load samples into openal
    alGenBuffers(1, &sampleBuffer);
    alBufferData(sampleBuffer,
                 getALFormat(format, codecContext->channels),
                 samples.data(),
                 static_cast<ALsizei>(samples.size()),
                 codecContext->sample_rate);
}
sounds::OpenALSoundBuffer::~OpenALSoundBuffer()
{
    alDeleteBuffers(1, &sampleBuffer);
}
auto
sounds::OpenALSoundBuffer::getBuffer() const -> ALuint
{
    return sampleBuffer;
}
sounds::OpenALSoundBuffer::OpenALSoundBuffer(
  sounds::OpenALSoundBuffer&& other) noexcept
  : sampleBuffer(other.sampleBuffer)
{
    other.sampleBuffer = 0;
}
auto
sounds::OpenALSoundBuffer::operator=(sounds::OpenALSoundBuffer&& other) noexcept
  -> sounds::OpenALSoundBuffer&
{
    if (this == &other) {
        return *this;
    }
    sampleBuffer = other.sampleBuffer;
    other.sampleBuffer = 0;
    return *this;
}

auto
sounds::OpenALSoundBuffer::getDuration() const -> std::chrono::nanoseconds
{
    ALint size{};
    alGetBufferi(sampleBuffer, AL_SIZE, &size);
    ALint frequency{};
    alGetBufferi(sampleBuffer, AL_FREQUENCY, &frequency);
    ALint channels{};
    alGetBufferi(sampleBuffer, AL_CHANNELS, &channels);
    ALint bits{};
    alGetBufferi(sampleBuffer, AL_BITS, &bits);
    auto denominator = frequency * channels * bits;
    if (denominator == 0) {
        spdlog::error("Could not calculate the duration of a sound. "
                      "Does your device have a sound card?");
        return std::chrono::nanoseconds::zero();
    }
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
      1'000'000'000ULL * static_cast<unsigned long long int>(size) * CHAR_BIT /
      (static_cast<unsigned long long int>(denominator))));
}
auto
sounds::OpenALSoundBuffer::getFrequency() const -> int
{
    ALint frequency{};
    alGetBufferi(sampleBuffer, AL_FREQUENCY, &frequency);
    return frequency;
}
auto
sounds::OpenALSoundBuffer::getChannels() const -> int
{
    ALint channels{};
    alGetBufferi(sampleBuffer, AL_CHANNELS, &channels);
    return channels;
}
