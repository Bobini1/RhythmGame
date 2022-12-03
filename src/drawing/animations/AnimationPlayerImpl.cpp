//
// Created by bobini on 03.12.22.
//

#include "AnimationPlayerImpl.h"
#include <algorithm>
void
drawing::animations::AnimationPlayerImpl::update(std::chrono::nanoseconds delta)
{
    erase_if(animations, [delta](const auto& animation) {
        animation->update(delta);
        return animation->getIsFinished();
    });
}
void
drawing::animations::AnimationPlayerImpl::playAnimation(
  std::shared_ptr<Animation> animation)
{
    animations.insert(std::move(animation));
}
void
drawing::animations::AnimationPlayerImpl::stopAnimation(
  std::shared_ptr<Animation> animation)
{
    animations.erase(std::move(animation));
}
