//
// Created by bobini on 15.04.23.
//

#include "OpenAlSoundBuffer.h"

#include <AL/alc.h>
#include <AL/alext.h>
#include <spdlog/spdlog.h>
#include <sndfile.hh>
#include <QGuiApplication>

auto
sounds::getALDevice() -> ALCdevice*
{
    static auto device = std::unique_ptr<ALCdevice, decltype(&alcCloseDevice)>(
      alcOpenDevice(/*devicename=*/nullptr), &alcCloseDevice);
    return device.get();
}
auto
sounds::getALContext() -> ALCcontext*
{
    static auto context =
      std::unique_ptr<ALCcontext, decltype(&alcDestroyContext)>(
        alcCreateContext(getALDevice(), /*attrlist=*/nullptr),
        &alcDestroyContext);
    return context.get();
}

namespace {

struct OpenALEventContext {
    LPALCREOPENDEVICESOFT reopenFunc {nullptr};
    ALCdevice *device {nullptr};
    // For thread marshaling back to Qt main (or owning) thread
    QCoreApplication *app {nullptr};
};

// The actual event callback
static void ALC_APIENTRY onAlEvent(ALCenum eventType, ALCenum deviceType,
    ALCdevice *device, ALCsizei length, const ALCchar *message, void *userParam) noexcept
{
    auto *ctx = static_cast<OpenALEventContext*>(userParam);
    if(!ctx || !ctx->reopenFunc) return;

    if(eventType == ALC_EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT) {
        // (Optional) Log message text if provided
        QString msg = QString::fromUtf8(message ? QByteArray(message, length) : QByteArray());
        spdlog::debug("[OpenAL] Default device changed event:");

        // Reopen must generally be done in your main / audio management thread,
        // so dispatch via Qt event loop to be safe.
        QMetaObject::invokeMethod(ctx->app, [ctx, device]() {
            // Reopen with nullptrs to use the new default device
            if(!ctx->reopenFunc(sounds::getALDevice(), nullptr, nullptr)) {
                spdlog::error("[OpenAL] alcReopenDeviceSOFT failed after default device change.");
            } else {
                spdlog::debug("[OpenAL] Device successfully reopened on new default output.");
            }
        }, Qt::QueuedConnection);
    }
}
} // namespace

void sounds::installOpenALEvents()
{
    ALCdevice *device = sounds::getALDevice();
    if(!device) {
        spdlog::warn("No OpenAL device available.");
        return;
    }

    auto alcReopenDeviceSOFT  = reinterpret_cast<LPALCREOPENDEVICESOFT>(
        alcGetProcAddress(device, "alcReopenDeviceSOFT"));
    auto alcEventCallbackSOFT = reinterpret_cast<LPALCEVENTCALLBACKSOFT>(
        alcGetProcAddress(device, "alcEventCallbackSOFT"));
    auto alcEventControlSOFT  = reinterpret_cast<LPALCEVENTCONTROLSOFT>(
        alcGetProcAddress(device, "alcEventControlSOFT"));

    if(!alcReopenDeviceSOFT || !alcEventCallbackSOFT || !alcEventControlSOFT) {
        spdlog::warn("Required OpenAL event extension functions not available.");
        return;
    }

    static auto ctx = std::make_unique<OpenALEventContext>();
    ctx->reopenFunc = alcReopenDeviceSOFT;
    ctx->device     = device;
    ctx->app        = QCoreApplication::instance();

    // Register callback
    alcEventCallbackSOFT(onAlEvent, ctx.get());

    // Enable just the default-device-changed event
    const ALCenum events[] = { ALC_EVENT_TYPE_DEFAULT_DEVICE_CHANGED_SOFT };
    if(alcEventControlSOFT(1, events, ALC_TRUE) == ALC_FALSE) {
        spdlog::error("Failed to enable default device change event.");
    } else {
        spdlog::debug("OpenAL default device change event enabled.");
    }

    // You may keep ctx alive for the app lifetime. If you later want to disable:
    // alcEventControlSOFT(1, events, ALC_FALSE);
    // alcEventCallbackSOFT(nullptr, nullptr);
}

sounds::OpenALSoundBuffer::OpenALSoundBuffer(
  const std::filesystem::path& filename)
{
    auto sndFile = SndfileHandle{ filename.c_str(), SFM_READ };
    // throw if the file is not readable
    if (auto error = sndFile.error(); error != 0) {
        spdlog::error("Could not open sound file {}: {}",
                      filename.string(),
                      sf_error_number(error));
        throw std::runtime_error("Could not open sound file");
    }
    auto sndFileSamples = std::vector<float>{};
    sndFileSamples.resize(sndFile.frames() * sndFile.channels());
    sndFile.read(sndFileSamples.data(), sndFileSamples.size());
    static auto once = std::once_flag{};
    std::call_once(once, alcMakeContextCurrent, getALContext());
    alGenBuffers(1, &sampleBuffer);
    alBufferData(sampleBuffer,
                 sndFile.channels() == 1 ? AL_FORMAT_MONO_FLOAT32
                                         : AL_FORMAT_STEREO_FLOAT32,
                 sndFileSamples.data(),
                 sndFileSamples.size() * sizeof(float),
                 sndFile.samplerate());
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
    alDeleteBuffers(1, &sampleBuffer);
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
      1'000'000'000ULL * static_cast<uint64_t>(size) * CHAR_BIT /
      (static_cast<uint64_t>(denominator))));
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
