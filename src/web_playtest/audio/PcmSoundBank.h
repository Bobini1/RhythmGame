#pragma once

#include "AudioCommand.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <vector>

namespace web_playtest {

struct PcmClip
{
    std::vector<float> interleavedStereo;
    std::uint32_t sampleRate{};
};

struct DecodedPcm
{
    using DownmixToStereo = std::vector<float> (*)(std::span<const float>,
                                                   std::uint8_t);

    std::vector<float> interleaved;
    std::uint32_t sampleRate{};
    std::uint8_t channelCount{};
    DownmixToStereo downmixToStereo{};
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
    bool isFrozen{};
    std::vector<PcmClip> clips;
    std::vector<ClipId> voices;
    std::unordered_map<std::string, ClipId> assetClips;
    std::unordered_map<std::uint64_t, VoiceId> chartVoices;
};

} // namespace web_playtest
