//
// Created by bobini on 03.12.22.
//

#ifndef RHYTHMGAME_ANIMATIONPLAYER_H
#define RHYTHMGAME_ANIMATIONPLAYER_H

#include "Animation.h"
#include <memory>
namespace drawing::animations {
template<typename T>
concept AnimationPlayer = requires(T animationPlayer,
                                   std::chrono::nanoseconds delta,
                                   std::shared_ptr<Animation> animation) {
                              {
                                  animationPlayer.update(delta)
                              };
                              {
                                  animationPlayer.playAnimation(animation)
                              };
                              {
                                  animationPlayer.stopAnimation(animation)
                              };
                              {
                                  animationPlayer.isPlaying(animation)
                              } -> std::same_as<bool>;
                          };
} // namespace drawing::animations

#endif // RHYTHMGAME_ANIMATIONPLAYER_H
