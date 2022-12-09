//
// Created by bobini on 08.12.22.
//

#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Linear can have its start and end set",
          "[drawing][animations][linear]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    constexpr auto time = std::chrono::nanoseconds{ 1000 };
    auto animation = drawing::animations::Linear::make(
      [&quad](float value) { quad->setWidth(value); }, time, 0, 100);
    animation->setStart(50);
    animation->update({});
    REQUIRE(quad->getWidth() == Catch::Approx(50));
    animation->setEnd(150);
    animation->update(time);
    REQUIRE(quad->getWidth() == Catch::Approx(150));
}
