//
// Created by bobini on 08.12.22.
//

#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include "drawing/animations/AnimationSequence.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("AnimationSequence runs the right animation",
          "[drawing][animations][animationsequence]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    auto animation2 = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 200, 300);
    auto sequence =
      drawing::animations::AnimationSequence::make({ animation, animation2 });
    sequence->update({});
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    sequence->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(50));
    sequence->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(100));
    sequence->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(250));
    sequence->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(300));
}

TEST_CASE("AnimationSequence always updates all animations at least once",
          "[drawing][animations][animationsequence]")
{
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto counter = 0;
    auto animation = drawing::animations::Linear::make(
      [&counter](float /* value */) { counter++; },
      std::chrono::nanoseconds{},
      0,
      100);
    auto animation2 = drawing::animations::Linear::make(
      [&counter](float /* value */) { counter++; }, time, 0, 100);
    auto sequence = drawing::animations::AnimationSequence::make(
      { animation, animation->clone(), animation->clone(), animation2 });
    sequence->update(time);
    REQUIRE(counter == 4);
    REQUIRE(sequence->getIsFinished());
}
