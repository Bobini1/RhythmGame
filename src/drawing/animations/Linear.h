//
// Created by bobini on 11/24/22.
//

#ifndef RHYTHMGAME_LINEAR_H
#define RHYTHMGAME_LINEAR_H
#include <memory>
#include "Animation.h"
namespace drawing::animations {
class Linear : public Animation
{
  public:
    Linear(std::weak_ptr<actors::Actor> actor,
           std::function<void(std::shared_ptr<actors::Actor>, float)> updated,
           std::chrono::nanoseconds duration,
           float start,
           float end);
    [[nodiscard]] auto clone() const -> std::unique_ptr<Linear>;
    auto setFunction(
      std::function<void(std::shared_ptr<actors::Actor>, float)> updated)
      -> void;
    auto getFunction() const
      -> std::function<void(std::shared_ptr<actors::Actor>, float)>;

  private:
    [[nodiscard]] auto cloneImpl() const -> Linear* override;
    void updateImpl(std::chrono::nanoseconds delta) override;
    float start;
    float end;
    std::function<void(std::shared_ptr<actors::Actor>, float)> updated;
};
} // namespace drawing::animations

#endif // RHYTHMGAME_LINEAR_H
