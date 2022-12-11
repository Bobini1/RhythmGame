//
// Created by bobini on 05.12.22.
//

#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Animation uses a function to set a value",
          "[drawing][animations][animation]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    SECTION("No change to properties")
    {
        SECTION("Progress below 0")
        {
            animation->setProgress(-1.5f);
        }
        SECTION("Elapsed below 0")
        {
            animation->setElapsed(std::chrono::nanoseconds{ -1 });
        }
        animation->update({});
        REQUIRE(quad->getWidth() == Catch::Approx(0));
        animation->update(time / 2);
        REQUIRE(quad->getWidth() == Catch::Approx(50));
        animation->update(time / 2);
        REQUIRE(quad->getWidth() == Catch::Approx(100));
        REQUIRE(animation->getIsFinished());
    }
    SECTION("Changed duration")
    {
        animation->setDuration(animation->getDuration() / 2);
        animation->update({});
        REQUIRE(quad->getWidth() == Catch::Approx(0));
        animation->update(time / 4);
        REQUIRE(quad->getWidth() == Catch::Approx(50));
        animation->update(time / 4);
        REQUIRE(quad->getWidth() == Catch::Approx(100));
        REQUIRE(animation->getIsFinished());
    }
    SECTION("Changed progress or elapsed")
    {
        SECTION("Changed progress")
        {
            animation->setProgress(0.5F);
        }
        SECTION("Changed elapsed")
        {
            animation->setElapsed(animation->getDuration() / 2);
        }
        animation->update({});
        REQUIRE(quad->getWidth() == Catch::Approx(50));
        animation->update(time / 2);
        REQUIRE(quad->getWidth() == Catch::Approx(100));
        animation->update(time / 2);
        REQUIRE(quad->getWidth() == Catch::Approx(100));
        REQUIRE(animation->getIsFinished());
    }
    SECTION("Values above allowed")
    {
        SECTION("Progress above 1")
        {
            animation->setProgress(1.5F);
        }
        SECTION("Elapsed above duration")
        {
            animation->setElapsed(animation->getDuration() +
                                  std::chrono::nanoseconds{ 1 });
        }
        animation->update({});
        REQUIRE(animation->getProgress() == Catch::Approx(1.0F));
        REQUIRE(animation->getElapsed() == animation->getDuration());
        REQUIRE(quad->getWidth() == Catch::Approx(100));
        REQUIRE(animation->getIsFinished());
    }
    SECTION("Duration below elapsed")
    {
        animation->setElapsed(animation->getDuration());
        animation->setDuration(animation->getElapsed() / 2);
        animation->update({});
        REQUIRE(animation->getProgress() == Catch::Approx(1.0F));
        REQUIRE(animation->getElapsed() == animation->getDuration());
        REQUIRE(animation->getIsFinished());
    }
    SECTION("Duration below 0")
    {
        animation->setDuration(std::chrono::nanoseconds{ -1 });
        animation->update({});
        REQUIRE(animation->getProgress() == Catch::Approx(1.0F));
        REQUIRE(animation->getElapsed() == std::chrono::nanoseconds{});
        REQUIRE(animation->getDuration() == std::chrono::nanoseconds{});
        REQUIRE(animation->getIsFinished());
    }
}

TEST_CASE("Animation fires a callback when it is finished",
          "[drawing][animations][animation]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    bool callbackCalled = false;
    animation->setOnFinished([&callbackCalled]() { callbackCalled = true; });
    animation->update(time);
    REQUIRE(callbackCalled);
    REQUIRE(animation->getIsFinished());
}