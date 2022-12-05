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
    [[nodiscard]] auto clone() const -> std::shared_ptr<Linear>;
    auto setFunction(std::function<void(float)> updated) -> void;
    auto getFunction() const -> std::function<void(float)>;
    auto getStart() const -> float;
    auto getEnd() const -> float;
    auto setStart(float start) -> void;
    auto setEnd(float end) -> void;
    static auto make(
      std::function<void(float)> updated,
      std::chrono::nanoseconds duration,
      float start,
      float end) -> std::shared_ptr<Linear>;

  protected:
    Linear(std::function<void(float)> updated,
           std::chrono::nanoseconds duration,
           float start,
           float end);

  private:
    [[nodiscard]] auto cloneImpl() const -> Linear* override;
    void updateImpl(std::chrono::nanoseconds delta) override;
    float start;
    float end;
    std::function<void(float)> updated;
};
} // namespace drawing::animations

#endif // RHYTHMGAME_LINEAR_H
