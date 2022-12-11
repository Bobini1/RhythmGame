//
// Created by bobini on 08.12.22.
//

#ifndef RHYTHMGAME_ANIMATIONSEQUENCE_H
#define RHYTHMGAME_ANIMATIONSEQUENCE_H

#include "Animation.h"
#include <vector>
namespace drawing::animations {
class AnimationSequence : public Animation
{
  public:
    auto getDuration() const -> std::chrono::nanoseconds override;
    void updateImpl(std::chrono::nanoseconds delta) override;
    auto cloneImpl() const -> AnimationSequence* override;
    static auto make(std::vector<std::shared_ptr<Animation>> animations)
      -> std::shared_ptr<AnimationSequence>;

  protected:
    explicit AnimationSequence(
      std::vector<std::shared_ptr<Animation>> animations);

  private:
    std::vector<std::shared_ptr<Animation>> animations;
    auto setDurationImpl(std::chrono::nanoseconds newDuration) -> void override;
};
} // namespace drawing::animations
#endif // RHYTHMGAME_ANIMATIONSEQUENCE_H
