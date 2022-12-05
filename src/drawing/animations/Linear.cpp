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
    auto value = start + (end - start) * progress;
    updated(getActor(), value);
}
drawing::animations::Linear::Linear(
  std::weak_ptr<actors::Actor> actor,
  std::function<void(std::shared_ptr<actors::Actor>, float)> updated,
  std::chrono::nanoseconds duration,
  float start,
  float end)
  : Animation(std::move(actor), duration)
  , start(start)
  , end(end)
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
    return new Linear(getActor(), updated, getDuration(), start, end);
}
auto
drawing::animations::Linear::setFunction(
  std::function<void(std::shared_ptr<actors::Actor>, float)> newUpdated) -> void
{
    this->updated = std::move(newUpdated);
}
auto
drawing::animations::Linear::getFunction() const
  -> std::function<void(std::shared_ptr<actors::Actor>, float)>
{
    return updated;
}
auto
drawing::animations::Linear::make(
  std::weak_ptr<actors::Actor> actor,
  std::function<void(std::shared_ptr<actors::Actor>, float)> updated,
  std::chrono::nanoseconds duration,
  float start,
  float end) -> std::shared_ptr<Linear>
{
    return std::shared_ptr<Linear>{ new Linear(
      std::move(actor), std::move(updated), duration, start, end) };
}
auto
drawing::animations::Linear::getStart() const -> float
{
    return start;
}
auto
drawing::animations::Linear::getEnd() const -> float
{
    return end;
}
auto
drawing::animations::Linear::setStart(float newStart) -> void
{
    start = newStart;
}
auto
drawing::animations::Linear::setEnd(float newEnd) -> void
{
    end = newEnd;
}
