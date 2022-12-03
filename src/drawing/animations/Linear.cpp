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
  -> std::unique_ptr<drawing::animations::Linear>
{
    return std::unique_ptr<Linear>{ cloneImpl() };
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
