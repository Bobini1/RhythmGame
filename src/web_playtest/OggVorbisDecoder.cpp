#include "OggVorbisDecoder.h"

#include "support/PathToQString.h"

#include <QFile>

#include <climits>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#define STB_VORBIS_NO_STDIO
#define STB_VORBIS_NO_INTEGER_CONVERSION
#define STB_VORBIS_NO_PUSHDATA_API
#include <stb_vorbis.c>

namespace web_playtest {
namespace {

class VorbisHandle final
{
  public:
    explicit VorbisHandle(stb_vorbis* value) noexcept
      : handle(value)
    {
    }
    ~VorbisHandle()
    {
        if (handle != nullptr) {
            stb_vorbis_close(handle);
        }
    }
    VorbisHandle(const VorbisHandle&) = delete;
    auto operator=(const VorbisHandle&) -> VorbisHandle& = delete;

    [[nodiscard]] auto get() const noexcept -> stb_vorbis* { return handle; }

  private:
    stb_vorbis* handle;
};

} // namespace

auto
decodeOggVorbis(const std::filesystem::path& path) -> DecodedPcm
{
    QFile file{ support::pathToQString(path) };
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Could not open an OGG keysound");
    }
    const auto bytes = file.readAll();
    if (file.error() != QFileDevice::NoError || bytes.isEmpty() ||
        bytes.size() > INT_MAX) {
        throw std::runtime_error("Could not read an OGG keysound");
    }

    auto openError = int{};
    auto decoder = VorbisHandle{ stb_vorbis_open_memory(
      reinterpret_cast<const unsigned char*>(bytes.constData()),
      static_cast<int>(bytes.size()),
      &openError,
      nullptr) };
    if (decoder.get() == nullptr) {
        throw std::runtime_error("stb_vorbis rejected an OGG keysound");
    }

    const auto info = stb_vorbis_get_info(decoder.get());
    const auto sourceFrames = static_cast<std::size_t>(
      stb_vorbis_stream_length_in_samples(decoder.get()));
    if (info.sample_rate < minimumOutputSampleRate ||
        info.sample_rate > maximumOutputSampleRate || info.channels <= 0 ||
        info.channels > 2 || sourceFrames == 0) {
        throw std::runtime_error("OGG keysound metadata is invalid");
    }
    const auto outputChannels = static_cast<std::uint8_t>(info.channels);
    if (sourceFrames >
          (std::numeric_limits<std::size_t>::max)() / outputChannels ||
        sourceFrames * outputChannels > static_cast<std::size_t>(INT_MAX)) {
        throw std::overflow_error("OGG keysound is too large to decode");
    }

    auto samples = std::vector<float>(sourceFrames * outputChannels);
    auto decodedFrames = std::size_t{};
    while (decodedFrames < sourceFrames) {
        const auto remainingSamples =
          (sourceFrames - decodedFrames) * outputChannels;
        const auto decoded = stb_vorbis_get_samples_float_interleaved(
          decoder.get(),
          outputChannels,
          samples.data() + decodedFrames * outputChannels,
          static_cast<int>(remainingSamples));
        if (decoded <= 0) {
            break;
        }
        decodedFrames += static_cast<std::size_t>(decoded);
    }
    if (decodedFrames != sourceFrames ||
        stb_vorbis_get_error(decoder.get()) != VORBIS__no_error) {
        throw std::runtime_error("OGG keysound decoding was incomplete");
    }

    return {
        .interleaved = std::move(samples),
        .sampleRate = info.sample_rate,
        .channelCount = outputChannels,
    };
}

} // namespace web_playtest
