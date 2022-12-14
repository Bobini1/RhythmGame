//
// Created by bobini on 08.12.22.
//

#include "AbstractBasicAnimation.h"
auto
drawing::animations::AbstractBasicAnimation::getDuration() const
  -> std::chrono::nanoseconds
{
    return duration;
}
auto
drawing::animations::AbstractBasicAnimation::setDurationImpl(
  std::chrono::nanoseconds newDuration) -> void
{
    duration = newDuration;
}
drawing::animations::AbstractBasicAnimation::AbstractBasicAnimation(
  std::chrono::nanoseconds duration)
  : duration(duration)
{
}
auto
drawing::animations::AbstractBasicAnimation::clone() const
  -> std::shared_ptr<AbstractBasicAnimation>
{
    return std::shared_ptr<AbstractBasicAnimation>{ cloneImpl() };
}
