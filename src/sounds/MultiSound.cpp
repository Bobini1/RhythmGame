//
// Created by copilot on 24.02.26.
//

#include "MultiSound.h"

namespace sounds {

MultiSound::MultiSound(std::vector<std::shared_ptr<Sound>> sounds)
  : childSounds(std::move(sounds))
{
}

void
MultiSound::play()
{
    for (auto& sound : childSounds) {
        sound->play();
    }
}

void
MultiSound::stop()
{
    for (auto& sound : childSounds) {
        sound->stop();
    }
}

void
MultiSound::setVolume(float volume)
{
    for (auto& sound : childSounds) {
        sound->setVolume(volume);
    }
}

auto
MultiSound::isPlaying() const -> bool
{
    for (const auto& sound : childSounds) {
        if (sound->isPlaying()) {
            return true;
        }
    }
    return false;
}

auto
MultiSound::getVolume() const -> float
{
    if (childSounds.empty()) {
        return 1.0F;
    }
    return childSounds.front()->getVolume();
}

} // namespace sounds
