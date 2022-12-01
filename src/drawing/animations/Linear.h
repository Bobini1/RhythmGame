//
// Created by bobini on 11/24/22.
//

#ifndef RHYTHMGAME_LINEAR_H
#define RHYTHMGAME_LINEAR_H
#include "Animation.h"
namespace drawing::animations {
class Linear : public Animation
{
  public:
    Linear(std::function<void(float)> updated, std::chrono::nanoseconds duration, float start, float end);
  private:
    void updateImpl(std::chrono::nanoseconds delta) override;
    float start;
    float end;
    std::function<void(float)> updated;
};
} // namespace drawing::animations

#endif // RHYTHMGAME_LINEAR_H
