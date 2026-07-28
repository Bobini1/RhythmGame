#include "PcmSoundBank.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace web_playtest {
namespace {
auto
toStereo(const DecodedPcm& decoded) -> std::vector<float>
{
    if (decoded.sampleRate == 0 || decoded.interleaved.empty() ||
        decoded.channelCount == 0) {
        throw std::invalid_argument(
          "decoded PCM must have a sample rate and channels");
    }
    if (decoded.interleaved.size() % decoded.channelCount != 0 ||
        !std::ranges::all_of(decoded.interleaved,
                             [](float v) { return std::isfinite(v); })) {
        throw std::invalid_argument("malformed or non-finite decoded PCM");
    }
    if (decoded.channelCount == 2) {
        return decoded.interleaved;
    }
    if (decoded.channelCount > 2) {
        if (decoded.downmixToStereo == nullptr) {
            throw std::invalid_argument(
              "multichannel PCM requires an explicit decoder downmix");
        }
        auto stereo =
          decoded.downmixToStereo(decoded.interleaved, decoded.channelCount);
        const auto sourceFrames =
          decoded.interleaved.size() / decoded.channelCount;
        if (stereo.size() != sourceFrames * 2 ||
            !std::ranges::all_of(
              stereo, [](float value) { return std::isfinite(value); })) {
            throw std::invalid_argument(
              "decoder downmix returned malformed stereo");
        }
        return stereo;
    }
    auto stereo = std::vector<float>{};
    stereo.reserve(decoded.interleaved.size() * 2);
    for (const auto sample : decoded.interleaved) {
        stereo.push_back(sample);
        stereo.push_back(sample);
    }
    return stereo;
}

auto
resampleLinear(const std::vector<float>& stereo,
               std::uint32_t sourceRate,
               std::uint32_t outputRate) -> std::vector<float>
{
    const auto sourceFrames = stereo.size() / 2;
    if (sourceFrames == 0 || sourceRate == outputRate) {
        return stereo;
    }
    const auto outputFrames = static_cast<std::size_t>(
      static_cast<std::uint64_t>(sourceFrames) * outputRate / sourceRate);
    if (outputFrames == 0) {
        throw std::invalid_argument("resampling produced an empty clip");
    }
    auto output = std::vector<float>(outputFrames * 2);
    for (std::size_t frame = 0; frame < outputFrames; ++frame) {
        const auto sourcePosition =
          static_cast<long double>(frame) * sourceRate / outputRate;
        const auto first =
          std::min(static_cast<std::size_t>(sourcePosition), sourceFrames - 1);
        const auto second = std::min(first + 1, sourceFrames - 1);
        const auto fraction =
          static_cast<float>(sourcePosition - static_cast<long double>(first));
        for (std::size_t channel = 0; channel < 2; ++channel) {
            const auto a = stereo[first * 2 + channel];
            const auto b = stereo[second * 2 + channel];
            output[frame * 2 + channel] = a + (b - a) * fraction;
        }
    }
    return output;
}
} // namespace

PcmSoundBank::PcmSoundBank(std::uint32_t outputSampleRate)
  : outputRate(outputSampleRate)
{
    if (outputRate == 0) {
        throw std::invalid_argument("output sample rate must be nonzero");
    }
}

auto
PcmSoundBank::addClip(std::string_view canonicalAssetKey, DecodedPcm decoded)
  -> ClipId
{
    if (isFrozen || canonicalAssetKey.empty()) {
        throw std::logic_error("PCM bank is frozen or asset key is empty");
    }
    if (const auto existing = assetClips.find(std::string(canonicalAssetKey));
        existing != assetClips.end()) {
        return existing->second;
    }
    if (clips.size() >= std::numeric_limits<ClipId>::max()) {
        throw std::overflow_error("ClipId capacity exceeded");
    }
    auto stereo = toStereo(decoded);
    stereo = resampleLinear(stereo, decoded.sampleRate, outputRate);
    if (stereo.size() % 2 != 0 || !std::ranges::all_of(stereo, [](float value) {
            return std::isfinite(value);
        })) {
        throw std::invalid_argument("invalid resampled PCM");
    }
    const auto id = static_cast<ClipId>(clips.size());
    clips.push_back(
      { .interleavedStereo = std::move(stereo), .sampleRate = outputRate });
    assetClips.emplace(canonicalAssetKey, id);
    return id;
}

auto
PcmSoundBank::addVoice(std::uint64_t chartSoundId, ClipId clipId) -> VoiceId
{
    if (isFrozen || clipId >= clips.size()) {
        throw std::logic_error("invalid voice insertion");
    }
    if (chartVoices.contains(chartSoundId)) {
        throw std::invalid_argument("chart sound ID already owns a voice");
    }
    if (voices.size() >= std::numeric_limits<VoiceId>::max() ||
        voices.size() >= AudioTransport::voiceCapacity) {
        throw std::overflow_error("VoiceId capacity exceeded");
    }
    const auto id = static_cast<VoiceId>(voices.size());
    voices.push_back(clipId);
    chartVoices.emplace(chartSoundId, id);
    return id;
}

void
PcmSoundBank::freeze()
{
    clips.shrink_to_fit();
    voices.shrink_to_fit();
    isFrozen = true;
}

auto
PcmSoundBank::frozen() const noexcept -> bool
{
    return isFrozen;
}
auto
PcmSoundBank::outputSampleRate() const noexcept -> std::uint32_t
{
    return outputRate;
}
auto
PcmSoundBank::clip(ClipId id) const noexcept -> const PcmClip&
{
    return clips[id];
}
auto
PcmSoundBank::clipForVoice(VoiceId id) const noexcept -> ClipId
{
    return voices[id];
}
auto
PcmSoundBank::voiceForChartSound(std::uint64_t chartSoundId) const -> VoiceId
{
    return chartVoices.at(chartSoundId);
}
auto
PcmSoundBank::clipCount() const noexcept -> std::size_t
{
    return clips.size();
}
auto
PcmSoundBank::voiceCount() const noexcept -> std::size_t
{
    return voices.size();
}

} // namespace web_playtest
