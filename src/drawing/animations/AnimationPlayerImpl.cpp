//
// Created by bobini on 03.12.22.
//

#include "AnimationPlayerImpl.h"
#include <algorithm>
void
drawing::animations::AnimationPlayerImpl::update(std::chrono::nanoseconds delta)
{
    for (auto& animation : animations) {
        animation->update(delta);
    }
    erase_if(animations,
             [](const auto& animation) { return animation->getIsFinished(); });
}
void
drawing::animations::AnimationPlayerImpl::playAnimation(
  std::shared_ptr<Animation> animation)
{
    animations.insert(std::move(animation));
}
void
drawing::animations::AnimationPlayerImpl::stopAnimation(
  const std::shared_ptr<Animation>& animation)
{
    animations.erase(animation);
}
auto
drawing::animations::AnimationPlayerImpl::isPlaying(
  const std::shared_ptr<Animation>& animation) const -> bool
{
    return animations.contains(animation);
}
