//
// Created by bobini on 11/24/22.
//

#include "Linear.h"

#include <utility>
#include <iostream>
void
drawing::animations::Linear::updateImpl(std::chrono::nanoseconds /*delta*/)
{
    auto progress = getProgress();
    auto value = from + (to - from) * progress;
    if (updated) {
        updated(value);
    }
}
drawing::animations::Linear::Linear(std::function<void(float)> updated,
                                    std::chrono::nanoseconds duration,
                                    float start,
                                    float end)
  : AbstractBasicAnimation(duration)
  , from(start)
  , to(end)
  , updated(std::move(updated))
{
}
auto
drawing::animations::Linear::clone() const
  -> std::shared_ptr<drawing::animations::Linear>
{
    return std::shared_ptr<Linear>{ cloneImpl() };
}
auto
drawing::animations::Linear::cloneImpl() const -> drawing::animations::Linear*
{
    return new Linear(*this); // NOLINT(cppcoreguidelines-owning-memory)
}
auto
drawing::animations::Linear::setFunction(std::function<void(float)> newUpdated)
  -> void
{
    this->updated = std::move(newUpdated);
}
auto
drawing::animations::Linear::getFunction() const -> std::function<void(float)>
{
    return updated;
}
auto
drawing::animations::Linear::make(std::function<void(float)> updated,
                                  std::chrono::nanoseconds duration,
                                  float start,
                                  float end) -> std::shared_ptr<Linear>
{
    return std::shared_ptr<Linear>{ new Linear(
      std::move(updated), duration, start, end) };
}
auto
drawing::animations::Linear::getFrom() const -> float
{
    return from;
}
auto
drawing::animations::Linear::getTo() const -> float
{
    return to;
}
auto
drawing::animations::Linear::setFrom(float newFrom) -> void
{
    from = newFrom;
}
auto
drawing::animations::Linear::setTo(float newTo) -> void
{
    to = newTo;
}
