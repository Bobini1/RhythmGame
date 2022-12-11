//
// Created by bobini on 11/24/22.
//

#include <memory>
#include "Animation.h"
#include "drawing/actors/Actor.h"
auto
drawing::animations::Animation::setIsLooping(bool newIsLooping) -> void
{
    isLooping = newIsLooping;
}
void
drawing::animations::Animation::setOnFinished(
  std::function<void()> newOnFinished)
{
    onFinished = std::move(newOnFinished);
}
auto
drawing::animations::Animation::getIsLooping() const -> bool
{
    return isLooping;
}
auto
drawing::animations::Animation::getIsFinished() const -> bool
{
    return getElapsed() >= getDuration();
}
auto
drawing::animations::Animation::getProgress() const -> float
{
    auto duration = getDuration();
    if (!duration.count()) {
        return 1.0f;
    }
    return static_cast<float>(elapsed.count()) /
           static_cast<float>(getDuration().count());
}
void
drawing::animations::Animation::update(std::chrono::nanoseconds delta)
{
    auto oldElapsed = elapsed;
    elapsed += delta;
    auto duration = getDuration();

    if (elapsed >= duration) {
        if (isLooping) {
            elapsed = duration - oldElapsed;
        } else {
            elapsed = duration;
        }
        updateImpl(delta);
        if (onFinished) {
            onFinished();
        }
    } else {
        updateImpl(delta);
    }
}
auto
drawing::animations::Animation::getOnFinished() const
  -> const std::function<void()>&
{
    return onFinished;
}
void
drawing::animations::Animation::reset()
{
    elapsed = std::chrono::nanoseconds();
}
auto
drawing::animations::Animation::setProgress(float progress) -> void
{
    if (progress >= 1.0F) {
        elapsed = getDuration();
    } else {
        progress = std::max(progress, 0.0F);
        elapsed =
          std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
            static_cast<float>(getDuration().count()) * progress));
    }
}
auto
drawing::animations::Animation::clone() const
  -> std::shared_ptr<drawing::animations::Animation>
{
    return std::unique_ptr<Animation>{ cloneImpl() };
}
auto
drawing::animations::Animation::getElapsed() const -> std::chrono::nanoseconds
{
    return elapsed;
}
auto
drawing::animations::Animation::setElapsed(std::chrono::nanoseconds newElapsed)
  -> void
{
    auto duration = getDuration();
    if (newElapsed >= duration) {
        elapsed = duration;
    } else {
        elapsed = std::max(newElapsed, std::chrono::nanoseconds{});
    }
}
auto
drawing::animations::Animation::setDuration(
  std::chrono::nanoseconds newDuration) -> void
{
    setDurationImpl(std::max(std::chrono::nanoseconds{}, newDuration));
    auto duration = getDuration();
    if (elapsed > duration) {
        elapsed = duration;
    }
}
