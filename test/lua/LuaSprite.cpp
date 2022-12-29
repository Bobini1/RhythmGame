//
// Created by bobini on 19.11.22.
//

#include "drawing/actors/Sprite.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../setupState.h"

// TODO: add other parameters when texture management is implemented
static constexpr auto scriptWithArrayConstructor = R"(
    local sprite = Sprite.new{minWidth = 100, minHeight = 100,
                              color = Color.new(255, 0, 0, 255)}
    return sprite
)";

#ifndef DISABLE_WINDOW_TESTS
TEST_CASE("Sprite can be constructed from lua", "[drawing][actors][sprite]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto sprite = std::dynamic_pointer_cast<drawing::actors::Sprite>(root);
    REQUIRE(sprite != nullptr);
    REQUIRE(sprite->getMinWidth() == Catch::Approx(100));
    REQUIRE(sprite->getMinHeight() == Catch::Approx(100));
    REQUIRE(sprite->getColor() == sf::Color(255, 0, 0, 255));
}

static constexpr auto scriptWithSimpleCtor = R"(
    local sprite = Sprite.new("Fallback")
    return sprite
)";

TEST_CASE("Sprite simple contructor works", "[drawing][actors][sprite]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithSimpleCtor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto sprite = std::dynamic_pointer_cast<drawing::actors::Sprite>(root);
    REQUIRE(sprite != nullptr);
    REQUIRE(sprite->getTexture() != nullptr);
}

#endif