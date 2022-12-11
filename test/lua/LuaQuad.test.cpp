//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Quad.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local quad = Quad.new{width = 100, height = 100, minWidth = 50, minHeight = 50,
                          outlineColor = Color.new(0, 0, 0, 255),
                          outlineThickness = 1,
                          fillColor = Color.new(255, 0, 0, 255)}
    return quad
)";

TEST_CASE("Quad can be constructed from lua", "[drawing][actors][quad]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(100));
    REQUIRE(quad->getHeight() == Catch::Approx(100));
    REQUIRE(quad->getMinWidth() == Catch::Approx(50));
    REQUIRE(quad->getMinHeight() == Catch::Approx(50));
    REQUIRE(quad->getOutlineColor() == sf::Color(0, 0, 0, 255));
    REQUIRE(quad->getOutlineThickness() == Catch::Approx(1));
    REQUIRE(quad->getFillColor() == sf::Color(255, 0, 0, 255));
}