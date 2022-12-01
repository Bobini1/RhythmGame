//
// Created by bobini on 11/24/22.
//

#ifndef RHYTHMGAME_ANIMATION_H
#define RHYTHMGAME_ANIMATION_H

#include <chrono>
#include <functional>
namespace drawing::animations {
class Animation
{
  public:
    explicit Animation(std::chrono::nanoseconds duration);
    void update(std::chrono::nanoseconds delta);
    void reset();
    void setOnFinished(std::function<void()> onFinished);
    [[nodiscard]] auto getOnFinished() const -> const std::function<void()>&;
    [[nodiscard]] auto getIsPlaying() const -> bool;
    auto setIsPlaying() -> void;
    [[nodiscard]] auto getIsLooping() const -> bool;
    auto setIsLooping() -> void;
    [[nodiscard]] auto getIsFinished() const -> bool;
    [[nodiscard]] auto getDuration() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto getElapsed() const -> std::chrono::nanoseconds;
    [[nodiscard]] auto getProgress() const -> float;
    virtual ~Animation() = default;
  private:
    virtual void updateImpl(std::chrono::nanoseconds delta) = 0;
    bool isPlaying = false;
    bool isLooping = false;
    bool isFinished = false;
    std::chrono::nanoseconds duration{};
    std::chrono::nanoseconds elapsed{};
    std::function<void()> onFinished{};
};
} // namespace drawing::animations
#endif // RHYTHMGAME_ANIMATION_H
