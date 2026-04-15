//
// Created by copilot on 24.02.26.
//

#include "SlicedSoundBuffer.h"

#include <algorithm>

namespace sounds {

SlicedSoundBuffer::SlicedSoundBuffer(std::shared_ptr<const SoundBuffer> parent,
                                     ma_uint64 startFrame,
                                     ma_uint64 frameCount)
  : parent(std::move(parent))
  , startFrame(startFrame)
  , frameCount(frameCount)
{
}

auto
SlicedSoundBuffer::getFrames() const -> ma_uint64
{
    return frameCount;
}

auto
SlicedSoundBuffer::getSamples() const -> std::span<const float>
{
    constexpr auto channels = 2;
    auto parentSamples = parent->getSamples();
    auto startSample = startFrame * channels;
    auto sampleCount = frameCount * channels;
    // Clamp to parent bounds
    if (startSample >= parentSamples.size()) {
        return {};
    }
    sampleCount = std::min(sampleCount, parentSamples.size() - startSample);
    return parentSamples.subspan(startSample, sampleCount);
}

} // namespace sounds
