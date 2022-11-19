//
// Created by bobini on 11.11.22.
//

#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Size of actor can't be changed below min size", "[drawing][actors][actor]")
{
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ -100.F, -100.F });
    REQUIRE(quad->getHeight() == Catch::Approx(quad->getMinHeight()));
    REQUIRE(quad->getWidth() == Catch::Approx(quad->getMinWidth()));
}