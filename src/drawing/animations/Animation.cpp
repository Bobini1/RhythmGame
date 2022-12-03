//
// Created by bobini on 11/24/22.
//

#include <memory>
#include "Animation.h"
#include "drawing/actors/Actor.h"
auto
drawing::animations::Animation::setIsLooping() -> void
{
    isLooping = true;
}
void
drawing::animations::Animation::setOnFinished(
  std::function<void(std::shared_ptr<actors::Actor>)> newOnFinished)
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
    return isFinished;
}
auto
drawing::animations::Animation::getDuration() const -> std::chrono::nanoseconds
{
    return duration;
}
auto
drawing::animations::Animation::getProgress() const -> float
{
    return static_cast<float>(elapsed.count()) /
           static_cast<float>(duration.count());
}
drawing::animations::Animation::Animation(std::weak_ptr<actors::Actor> actor,
                                          std::chrono::nanoseconds duration)
  : actor(std::move(actor))
  , duration(duration)
{
}
void
drawing::animations::Animation::update(std::chrono::nanoseconds delta)
{
    if (actor.expired()) {
        isFinished = true;
        return;
    }

    elapsed += delta;

    if (elapsed >= duration) {
        elapsed = duration;
        updateImpl(delta);
        if (isLooping) {
            elapsed = std::chrono::nanoseconds{};
        } else {
            isFinished = true;
        }
        if (onFinished) {
            onFinished(actor.lock());
        }
    } else {
        updateImpl(delta);
    }
}
auto
drawing::animations::Animation::getOnFinished() const
  -> const std::function<void(std::shared_ptr<actors::Actor>)>&
{
    return onFinished;
}
void
drawing::animations::Animation::reset()
{
    elapsed = std::chrono::nanoseconds();
    isFinished = false;
}
auto
drawing::animations::Animation::setDuration(
  std::chrono::nanoseconds newDuration) -> void
{
    duration = newDuration;
}
auto
drawing::animations::Animation::setProgress(float progress) -> void
{
    if (progress >= 1.0F) {
        elapsed = duration;
    } else {
        progress = std::max(progress, 0.0F);
        elapsed =
          std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(
            static_cast<float>(duration.count()) * progress));
    }
    update(std::chrono::nanoseconds{});
}
auto
drawing::animations::Animation::clone() const
  -> std::unique_ptr<drawing::animations::Animation>
{
    return std::unique_ptr<Animation>{ cloneImpl() };
}
void
drawing::animations::Animation::setActor(std::weak_ptr<actors::Actor> newActor)
{
    actor = std::move(newActor);
}
auto
drawing::animations::Animation::getActor() const
  -> std::shared_ptr<actors::Actor>
{
    return actor.lock();
}
