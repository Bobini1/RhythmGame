//
// Created by bobini on 08.12.22.
//

#include <numeric>
#include "AnimationSequence.h"
auto
drawing::animations::AnimationSequence::getDuration() const
  -> std::chrono::nanoseconds
{
    return std::accumulate(
      animations.begin(),
      animations.end(),
      std::chrono::nanoseconds{},
      [](std::chrono::nanoseconds time, const auto& animation) {
          return time + animation->getDuration();
      });
}
void
drawing::animations::AnimationSequence::setDurationImpl(
  std::chrono::nanoseconds newDuration)
{
    auto oldDuration = getDuration();
    auto ratio = newDuration / oldDuration;
    std::for_each(
      animations.begin(), animations.end(), [ratio](const auto& animation) {
          animation->setDuration(animation->getDuration() * ratio);
      });
}
void
drawing::animations::AnimationSequence::updateImpl(
  std::chrono::nanoseconds delta)
{
    auto timePoint = std::chrono::nanoseconds{};
    auto elapsed = getElapsed();
    auto currentFound = false;
    for (const auto& animation : animations) {
        auto animationDuration = animation->getDuration();
        auto absoluteEnd = animationDuration + timePoint;
        if (!currentFound) {
            if (absoluteEnd >= elapsed) {
                animation->setElapsed(elapsed - timePoint);
                animation->update({});
                currentFound = true;
            } else {
                animation->setElapsed(animationDuration);
                if (elapsed - delta <= absoluteEnd) {
                    animation->update({});
                }
            }
        } else {
            animation->setElapsed({});
        }
        timePoint += animationDuration;
    }
}
auto
drawing::animations::AnimationSequence::cloneImpl() const
  -> drawing::animations::AnimationSequence*
{
    return new drawing::animations::AnimationSequence(
      *this); // NOLINT(cppcoreguidelines-owning-memory)
}
drawing::animations::AnimationSequence::AnimationSequence(
  std::vector<std::shared_ptr<Animation>> animations)
  : animations(std::move(animations))
{
}
auto
drawing::animations::AnimationSequence::make(
  std::vector<std::shared_ptr<Animation>> animations)
  -> std::shared_ptr<AnimationSequence>
{
    return std::shared_ptr<AnimationSequence>{ new AnimationSequence{
      std::move(animations) } };
}
