//
// Created by bobini on 05.12.22.
//

#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Linear animation uses a function to set a value",
          "[drawing][animations][linear]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    animation->update({});
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    animation->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(50));
    animation->update(time / 2);
    REQUIRE(quad->getWidth() == Catch::Approx(100));
    REQUIRE(animation->getIsFinished());
}