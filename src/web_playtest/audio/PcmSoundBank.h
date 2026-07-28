#pragma once

#include "AudioCommand.h"

#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace web_playtest {

inline constexpr auto minimumOutputSampleRate = std::uint32_t{ 8'000 };
inline constexpr auto maximumOutputSampleRate = std::uint32_t{ 192'000 };

namespace detail {
[[nodiscard]] auto
resampledFrameCount(std::size_t sourceFrames,
                    std::uint32_t sourceRate,
                    std::uint32_t outputRate,
                    std::size_t maximumOutputFrames =
                      (std::numeric_limits<std::size_t>::max)()) -> std::size_t;
}

struct PcmClip
{
    std::vector<float> interleavedStereo;
    std::uint32_t sampleRate = {};
};

struct DecodedPcm
{
    using DownmixToStereo = std::vector<float> (*)(std::span<const float>,
                                                   std::uint8_t);

    std::vector<float> interleaved;
    std::uint32_t sampleRate = {};
    std::uint8_t channelCount = {};
    DownmixToStereo downmixToStereo = {};
};

class PcmSoundBank
{
  public:
    explicit PcmSoundBank(std::uint32_t outputSampleRate);
    auto addClip(std::string_view canonicalAssetKey, DecodedPcm decoded)
      -> ClipId;
    auto addVoice(std::uint64_t chartSoundId, ClipId clipId) -> VoiceId;
    void freeze();

    [[nodiscard]] auto frozen() const noexcept -> bool;
    [[nodiscard]] auto outputSampleRate() const noexcept -> std::uint32_t;
    [[nodiscard]] auto clip(ClipId id) const noexcept -> const PcmClip&;
    [[nodiscard]] auto clipForVoice(VoiceId id) const noexcept -> ClipId;
    [[nodiscard]] auto voiceForChartSound(std::uint64_t chartSoundId) const
      -> VoiceId;
    [[nodiscard]] auto clipCount() const noexcept -> std::size_t;
    [[nodiscard]] auto voiceCount() const noexcept -> std::size_t;

  private:
    std::uint32_t outputRate;
    bool isFrozen = {};
    std::vector<PcmClip> clips;
    std::vector<ClipId> voices;
    std::unordered_map<std::string, ClipId> assetClips;
    std::unordered_map<std::uint64_t, VoiceId> chartVoices;
};

} // namespace web_playtest
