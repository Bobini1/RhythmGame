//
// Created by bobini on 14.01.23.
//

#include <stdexcept>
#include <memory>
#include "FFmpegOpenAlSound.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
}
#include <AL/alc.h>
#include <AL/alext.h>

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
                    return AL_FORMAT_MONO_FLOAT32;
                case AV_SAMPLE_FMT_DBL:
                    return AL_FORMAT_MONO_DOUBLE_EXT;
                default:
                    throw std::runtime_error("Unsupported format");
            }
        case 2:
            switch (AVFormat) {
                case AV_SAMPLE_FMT_U8:
                    return AL_FORMAT_STEREO8;
                case AV_SAMPLE_FMT_S16:
                    return AL_FORMAT_STEREO16;
                case AV_SAMPLE_FMT_FLT:
                    return AL_FORMAT_STEREO_FLOAT32;
                case AV_SAMPLE_FMT_DBL:
                    return AL_FORMAT_STEREO_DOUBLE_EXT;
                default:
                    throw std::runtime_error("Unsupported format");
            }
        default:
            throw std::runtime_error("Unsupported format");
    }
}

auto
setSampleFormat(AVCodecContext& context, const AVCodec& codec) -> AVSampleFormat
{
    if (codec.type != AVMEDIA_TYPE_AUDIO || codec.sample_fmts == nullptr) {
        throw std::runtime_error("Could not decode audio");
    }
    const AVSampleFormat* fmt{};

    context.request_sample_fmt = AV_SAMPLE_FMT_NONE;

    for (fmt = codec.sample_fmts; *fmt != AV_SAMPLE_FMT_NONE; fmt++) {
        if (!av_sample_fmt_is_planar(*fmt) && *fmt != AV_SAMPLE_FMT_S32) {
            context.request_sample_fmt = *fmt;
            break;
        }
    }

    if (context.request_sample_fmt == AV_SAMPLE_FMT_NONE) {
        throw std::runtime_error("Could not decode audio");
    }
    context.sample_fmt = context.request_sample_fmt;
    return context.request_sample_fmt;
}

void
deleteBuffer(ALuint* buffer)
{
    alDeleteBuffers(1, buffer);
    delete buffer;
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
                const auto frameSampleCount =
                  static_cast<size_t>(frame->nb_samples);
                sampleCount += frameSampleCount * channels;
                samples.resize(sampleCount * bytesPerSample);
                auto frameData = std::span(
                  frame->data[0], frameSampleCount * channels * bytesPerSample);
                std::copy(frameData.begin(),
                          frameData.end(),
                          samples.end() - frameData.size());
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
sounds::FFmpegOpenALSound::FFmpegOpenALSound(const char* filename)
{
    auto* formatContext = avformat_alloc_context();
    avformat_open_input(&formatContext, filename, nullptr, nullptr);

    const auto streams =
      std::span(formatContext->streams, formatContext->nb_streams);

    avformat_find_stream_info(formatContext, nullptr);
    const auto audioStream =
      std::find_if(streams.begin(), streams.end(), [](const AVStream* stream) {
          return stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
      });
    if (audioStream == streams.end()) {
        throw std::runtime_error("Could not find audio stream");
    }
    const auto* codecParameters = (*audioStream)->codecpar;
    const auto* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (codec == nullptr) {
        throw std::runtime_error("Unsupported codec");
    }
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        throw std::runtime_error("Couldn't copy codec context");
    }
    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        throw std::runtime_error("Could not open codec");
    }
    auto format = setSampleFormat(*codecContext, *codec);
    spdlog::info("Format: {}", formatContext->iformat->long_name);
    spdlog::info("Sample format: {}", av_get_sample_fmt_name(format));
    spdlog::info("Sample rate: {}", codecContext->sample_rate);
    spdlog::info("Channels: {}", codecContext->channels);

    auto samples = decodeFile(*formatContext, **audioStream, *codecContext);
    alcMakeContextCurrent(getALContext());
    // load samples into openal
    ALuint buffer{};
    alGenBuffers(1, &buffer);
    sampleBuffer = std::shared_ptr<ALuint>(new ALuint(buffer), deleteBuffer);

    alBufferData(buffer,
                 getALFormat(format, codecContext->channels),
                 samples.data(),
                 static_cast<ALsizei>(samples.size()),
                 codecContext->sample_rate);

    avcodec_close(codecContext);
    avformat_close_input(&formatContext);

    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer));
}
sounds::FFmpegOpenALSound::~FFmpegOpenALSound()
{
    alDeleteSources(1, &source);
}
auto
sounds::FFmpegOpenALSound::isPlaying() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}

sounds::FFmpegOpenALSound::FFmpegOpenALSound(
  const sounds::FFmpegOpenALSound& other)
  : sampleBuffer(other.sampleBuffer)
{
    alGenSources(1, &source);
    alSourcei(source, AL_BUFFER, static_cast<ALint>(*sampleBuffer));
}
sounds::FFmpegOpenALSound::FFmpegOpenALSound(
  sounds::FFmpegOpenALSound&& other) noexcept
  : source(other.source)
  , sampleBuffer(std::move(other.sampleBuffer))
{
    other.source = 0;
}
auto
sounds::FFmpegOpenALSound::operator=(const sounds::FFmpegOpenALSound& other)
  -> sounds::FFmpegOpenALSound&
{
    if (this != &other) {
        alDeleteSources(1, &source);
        alGenSources(1, &source);
        alSourcei(source, AL_BUFFER, static_cast<ALint>(*sampleBuffer));
    }
    return *this;
}
auto
sounds::FFmpegOpenALSound::operator=(sounds::FFmpegOpenALSound&& other) noexcept
  -> sounds::FFmpegOpenALSound&
{
    if (this != &other) {
        alDeleteSources(1, &source);
        source = other.source;
        sampleBuffer = std::move(other.sampleBuffer);
        other.source = 0;
    }
    return *this;
}
#pragma clang diagnostic push
#pragma ide diagnostic ignored "readability-make-member-function-const"
void
sounds::FFmpegOpenALSound::pause()
{
    alSourcePause(source);
}
void
sounds::FFmpegOpenALSound::stop()
{
    alSourceStop(source);
}
void
sounds::FFmpegOpenALSound::play()
{
    alSourcePlay(source);
}
void
sounds::FFmpegOpenALSound::setVolume(float volume)
{
    alSourcef(source, AL_GAIN, volume);
}
auto
sounds::FFmpegOpenALSound::isPaused() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_PAUSED;
}
auto
sounds::FFmpegOpenALSound::isStopped() const -> bool
{
    ALint state{};
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}
auto
sounds::FFmpegOpenALSound::getVolume() const -> float
{
    float volume{};
    alGetSourcef(source, AL_GAIN, &volume);
    return volume;
}
auto
sounds::FFmpegOpenALSound::getDuration() const -> std::chrono::nanoseconds
{
    ALint size{};
    alGetBufferi(*sampleBuffer, AL_SIZE, &size);
    ALint frequency{};
    alGetBufferi(*sampleBuffer, AL_FREQUENCY, &frequency);
    ALint channels{};
    alGetBufferi(*sampleBuffer, AL_CHANNELS, &channels);
    ALint bits{};
    alGetBufferi(*sampleBuffer, AL_BITS, &bits);
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
      size * 8 / (frequency * channels * bits)));
}
void
sounds::FFmpegOpenALSound::setIsLooping(bool looping)
{
    alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}
void
sounds::FFmpegOpenALSound::setRate(float rate)
{
    alSourcef(source, AL_PITCH, rate);
}
void
sounds::FFmpegOpenALSound::setTimePoint(std::chrono::nanoseconds offset)
{
    const auto frequency = getFrequency();
    const auto samples = offset.count() * frequency / 1'000'000'000;
    alSourcei(source, AL_SAMPLE_OFFSET, static_cast<ALint>(samples));
    // todo: check if this requires the number of channels
}
auto
sounds::FFmpegOpenALSound::getFrequency() const -> int
{
    ALint frequency{};
    alGetBufferi(*sampleBuffer, AL_FREQUENCY, &frequency);
    return frequency;
}
auto
sounds::FFmpegOpenALSound::getRate() const -> float
{
    float rate{};
    alGetSourcef(source, AL_PITCH, &rate);
    return rate;
}
auto
sounds::FFmpegOpenALSound::getIsLooping() const -> bool
{
    ALint looping{};
    alGetSourcei(source, AL_LOOPING, &looping);
    return looping == AL_TRUE;
}
auto
sounds::FFmpegOpenALSound::getChannels() const -> int
{
    ALint channels{};
    alGetBufferi(*sampleBuffer, AL_CHANNELS, &channels);
    return channels;
}
auto
sounds::FFmpegOpenALSound::getTimePoint() const -> std::chrono::nanoseconds
{
    ALint samples{};
    alGetSourcei(source, AL_SAMPLE_OFFSET, &samples);
    return std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
      samples * 1'000'000'000 / getFrequency()));
}
#pragma clang diagnostic pop
