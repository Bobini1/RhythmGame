//
// Created by bobini on 11/24/22.
//

#include "Animation.h"
auto
drawing::animations::Animation::setIsLooping() -> void
{
    isLooping = true;
}
void
drawing::animations::Animation::setOnFinished(std::function<void()> newOnFinished)
{
    this->onFinished = std::move(newOnFinished);
}
auto
drawing::animations::Animation::getIsPlaying() const -> bool
{
    return isPlaying;
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
drawing::animations::Animation::getElapsed() const -> std::chrono::nanoseconds
{
    return elapsed;
}
auto
drawing::animations::Animation::getProgress() const -> float
{
    return static_cast<float>(elapsed.count()) /
           static_cast<float>(duration.count());
}
drawing::animations::Animation::Animation(std::chrono::nanoseconds duration)
: duration(duration)
{
}
void
drawing::animations::Animation::update(std::chrono::nanoseconds delta)
{
    if (isPlaying) {
        elapsed += delta;
        if (elapsed >= duration) {
            elapsed = duration;
            updateImpl(delta);
            if (isLooping) {
                elapsed = std::chrono::nanoseconds{};
            } else {
                isPlaying = false;
                isFinished = true;
                if (onFinished) {
                    onFinished();
                }
            }
        }
        else {
            updateImpl(delta);
        }
    }
}
auto
drawing::animations::Animation::getOnFinished() const -> const std::function<void()>&
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
drawing::animations::Animation::setIsPlaying() -> void
{
    isPlaying = true;
}
