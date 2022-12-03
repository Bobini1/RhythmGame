//
// Created by bobini on 03.12.22.
//

#ifndef RHYTHMGAME_ANIMATIONPLAYERIMPL_H
#define RHYTHMGAME_ANIMATIONPLAYERIMPL_H

#include <set>
#include "drawing/animations/AnimationPlayer.h"

namespace drawing::animations {
class AnimationPlayerImpl
{
  public:
    void update(std::chrono::nanoseconds delta);
    void playAnimation(std::shared_ptr<Animation> animation);
    void stopAnimation(std::shared_ptr<Animation> animation);

  private:
    std::set<std::shared_ptr<Animation>> animations;
};
static_assert(AnimationPlayer<AnimationPlayerImpl>);
} // namespace drawing::animations

#endif // RHYTHMGAME_ANIMATIONPLAYERIMPL_H
