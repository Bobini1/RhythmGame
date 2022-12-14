//
// Created by bobini on 14/12/2022.
//


#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <drawing/animations/AnimationPlayerImpl.h>

TEST_CASE("AnimationPlayerImpl updates animations", "[drawing][animations][animationplayerimpl]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    auto player = drawing::animations::AnimationPlayerImpl{};
    player.playAnimation(animation);
    REQUIRE(player.isPlaying(animation));
    player.update({});
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    player.update(time / 2);
    REQUIRE(player.isPlaying(animation));
    REQUIRE(quad->getWidth() == Catch::Approx(50));
    player.update(time / 2);
    REQUIRE(!player.isPlaying(animation));
    REQUIRE(quad->getWidth() == Catch::Approx(100));
}

TEST_CASE("Animations can be stopped in AnimationPlayerImpl", "[drawing][animations][animationplayerimpl]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    auto player = drawing::animations::AnimationPlayerImpl{};
    player.playAnimation(animation);
    REQUIRE(player.isPlaying(animation));
    player.update({});
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    player.stopAnimation(animation);
    REQUIRE(!player.isPlaying(animation));
    player.update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
}