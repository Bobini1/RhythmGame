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
    auto setIsLooping() -> void;
    [[nodiscard]] auto getIsFinished() const -> bool;
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    auto setDuration(std::chrono::nanoseconds newDuration) -> void;
    [[nodiscard]] auto getProgress() const -> float;
    auto setProgress(float progress) -> void;
    virtual ~Animation() = default;
    [[nodiscard]] auto clone() const -> std::shared_ptr<Animation>;

  protected:
    explicit Animation(std::chrono::nanoseconds duration);

  private:
    virtual void updateImpl(std::chrono::nanoseconds delta) = 0;
    [[nodiscard]] virtual auto cloneImpl() const -> Animation* = 0;
    std::chrono::nanoseconds duration{};
    std::chrono::nanoseconds elapsed{};
    std::function<void()> onFinished{};
    bool isLooping = false;
    bool isFinished = false;
};
} // namespace drawing::animations
#endif // RHYTHMGAME_ANIMATION_H
