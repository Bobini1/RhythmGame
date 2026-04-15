//
// Created by copilot on 24.02.26.
//

#ifndef RHYTHMGAME_SLICEDSOUNDBUFFER_H
#define RHYTHMGAME_SLICEDSOUNDBUFFER_H

#include "SoundBuffer.h"
#include <memory>

namespace sounds {

/**
 * @brief A view into a subsection of an existing SoundBuffer.
 * @details Used for bmson slicing. The parent buffer must outlive
 * the slice. Frame indices are in stereo frames (2 floats each).
 */
class SlicedSoundBuffer : public SoundBuffer
{
    std::shared_ptr<const SoundBuffer> parent;
    ma_uint64 startFrame;
    ma_uint64 frameCount;

  public:
    /**
     * @brief Creates a slice of an existing buffer.
     * @param parent The original buffer to slice from.
     * @param startFrame The first frame (inclusive) of the slice.
     * @param frameCount The number of frames in the slice.
     */
    SlicedSoundBuffer(std::shared_ptr<const SoundBuffer> parent,
                      ma_uint64 startFrame,
                      ma_uint64 frameCount);

    auto getFrames() const -> ma_uint64 override;
    auto getSamples() const -> std::span<const float> override;
};

} // namespace sounds

#endif // RHYTHMGAME_SLICEDSOUNDBUFFER_H
