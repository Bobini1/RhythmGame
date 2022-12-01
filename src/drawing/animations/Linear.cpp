//
// Created by bobini on 11/24/22.
//

#include "Linear.h"

#include <utility>
void
drawing::animations::Linear::updateImpl(std::chrono::nanoseconds /*delta*/)
{
    auto progress = getProgress();
    auto value = start + (end - start) * progress;
}
drawing::animations::Linear::Linear(std::function<void(float)> updated,
                                    std::chrono::nanoseconds duration,
                                    float start,
                                    float end)
    : Animation(duration), start(start), end(end), updated(std::move(updated))
{
}
