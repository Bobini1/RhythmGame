//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Padding.h"
#include "catch2/catch_test_macros.hpp"
#include "setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local padding = Padding.new{child = Quad.new{},
                                left = 11, right = 12, top = 13, bottom = 14}
    return padding
)";

TEST_CASE("Padding can be constructed from lua", "[drawing][actors][padding]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto padding = std::dynamic_pointer_cast<drawing::actors::Padding>(root);
    REQUIRE(padding != nullptr);
    REQUIRE(padding->getChild() != nullptr);
    REQUIRE(padding->getLeft() == Catch::Approx(11));
    REQUIRE(padding->getRight() == Catch::Approx(12));
    REQUIRE(padding->getTop() == Catch::Approx(13));
    REQUIRE(padding->getBottom() == Catch::Approx(14));
}

static constexpr auto scriptWithMixedConstructor = R"(
    local padding = Padding.new(Quad.new{},
                                {left = 11, right = 12, top = 13, bottom = 14})
    return padding
)";

TEST_CASE("Padding can be constructed from lua with mixed constructor",
          "[drawing][actors][padding]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithMixedConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto padding = std::dynamic_pointer_cast<drawing::actors::Padding>(root);
    REQUIRE(padding != nullptr);
    REQUIRE(padding->getChild() != nullptr);
    REQUIRE(padding->getLeft() == Catch::Approx(11));
    REQUIRE(padding->getRight() == Catch::Approx(12));
    REQUIRE(padding->getTop() == Catch::Approx(13));
    REQUIRE(padding->getBottom() == Catch::Approx(14));
}