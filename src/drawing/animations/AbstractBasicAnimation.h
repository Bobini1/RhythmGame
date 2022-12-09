//
// Created by bobini on 08.12.22.
//

#ifndef RHYTHMGAME_ABSTRACTBASICANIMATION_H
#define RHYTHMGAME_ABSTRACTBASICANIMATION_H

#include "Animation.h"
namespace drawing::animations {
class AbstractBasicAnimation : public Animation
{
  public:
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds override;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
    [[nodiscard]] auto clone() const -> std::shared_ptr<AbstractBasicAnimation>;
#pragma clang diagnostic pop

  protected:
    explicit AbstractBasicAnimation(std::chrono::nanoseconds duration);
    [[nodiscard]] auto cloneImpl() const
      -> AbstractBasicAnimation* override = 0;

  private:
    std::chrono::nanoseconds duration;
    auto setDurationImpl(std::chrono::nanoseconds newDuration) -> void override;
};
} // namespace drawing::animations
#endif // RHYTHMGAME_ABSTRACTBASICANIMATION_H
