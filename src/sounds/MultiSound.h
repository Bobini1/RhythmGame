//
// Created by copilot on 24.02.26.
//

#ifndef RHYTHMGAME_MULTISOUND_H
#define RHYTHMGAME_MULTISOUND_H

#include "Sound.h"
#include <vector>
#include <memory>

namespace sounds {

/**
 * @brief A sound that plays multiple underlying sounds simultaneously.
 * @details Used for bmson note fusion: when notes from different sound
 * channels share the same (x, y) position, their slices are combined
 * into a single MultiSound. Playing this sound plays all of them at once.
 */
class MultiSound : public Sound
{
    std::vector<std::shared_ptr<Sound>> childSounds;

  public:
    explicit MultiSound(std::vector<std::shared_ptr<Sound>> sounds);

    void play() override;
    void stop() override;
    void setVolume(float volume) override;
    auto isPlaying() const -> bool override;
    auto getVolume() const -> float override;
};

} // namespace sounds

#endif // RHYTHMGAME_MULTISOUND_H
