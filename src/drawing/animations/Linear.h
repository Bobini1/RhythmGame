//
// Created by bobini on 11/24/22.
//

#ifndef RHYTHMGAME_LINEAR_H
#define RHYTHMGAME_LINEAR_H
#include <memory>
#include "AbstractBasicAnimation.h"
namespace drawing::animations {
class Linear : public AbstractBasicAnimation
{
  public:
#pragma clang diagnostic push
#pragma ide diagnostic ignored "HidingNonVirtualFunction"
    [[nodiscard]] auto clone() const -> std::shared_ptr<Linear>;
#pragma clang diagnostic pop
    auto setFunction(std::function<void(float)> updated) -> void;
    auto getFunction() const -> std::function<void(float)>;
    auto getFrom() const -> float;
    auto getTo() const -> float;
    auto setFrom(float from) -> void;
    auto setTo(float to) -> void;
    static auto make(std::function<void(float)> updated,
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
    float from;
    float to;
    std::function<void(float)> updated;
};
} // namespace drawing::animations

#endif // RHYTHMGAME_LINEAR_H
