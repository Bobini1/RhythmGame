//
// Created by bobini on 11/24/22.
//

#ifndef RHYTHMGAME_ANIMATION_H
#define RHYTHMGAME_ANIMATION_H

#include <chrono>
#include <functional>
#include <support/EnableSharedFromBase.h>
namespace drawing::actors {
class Actor;
} // namespace drawing::actors
namespace drawing::animations {
class Animation : public support::EnableSharedFromBase<Animation>
{
  public:
    void update(std::chrono::nanoseconds delta);
    void reset();
    void setOnFinished(std::function<void()> onFinished);
    [[nodiscard]] auto getOnFinished() const -> const std::function<void()>&;
    [[nodiscard]] auto getIsLooping() const -> bool;
    auto setIsLooping(bool newIsLooping) -> void;
    [[nodiscard]] auto getIsFinished() const -> bool;
    [[nodiscard]] virtual auto getDuration() const
      -> std::chrono::nanoseconds = 0;
    auto setDuration(std::chrono::nanoseconds newDuration) -> void;
    [[nodiscard]] auto getProgress() const -> float;
    auto setProgress(float progress) -> void;
    [[nodiscard]] auto getElapsed() const -> std::chrono::nanoseconds;
    auto setElapsed(std::chrono::nanoseconds newElapsed) -> void;
    virtual ~Animation() = default;
    [[nodiscard]] auto clone() const -> std::shared_ptr<Animation>;

  protected:
    Animation() = default;

  private:
    virtual void updateImpl(std::chrono::nanoseconds delta) = 0;
    virtual auto setDurationImpl(std::chrono::nanoseconds newDuration)
      -> void = 0;
    [[nodiscard]] virtual auto cloneImpl() const -> Animation* = 0;
    std::chrono::nanoseconds elapsed{};
    std::function<void()> onFinished{};
    bool isLooping = false;
};
} // namespace drawing::animations
#endif // RHYTHMGAME_ANIMATION_H
