//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Layers.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local layers = Layers.new{children = {Quad.new{}}, mainLayer = Quad.new{}}
    return layers
)";

TEST_CASE("Layers can be constructed from lua", "[drawing][actors][layers]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto layers = std::dynamic_pointer_cast<drawing::actors::Layers>(root);
    REQUIRE(layers != nullptr);
    REQUIRE(layers->getMainLayer() != nullptr);
    REQUIRE(layers->getSize() == 2);
}

static constexpr auto scriptWithArrayConstructorNoMainLayer = R"(
    local layers = Layers.new{children = {Quad.new{}}, width = 100, height = 100}
    return layers
)";

TEST_CASE("Layers can be constructed from lua without main layer",
          "[drawing][actors][layers]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructorNoMainLayer);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto layers = std::dynamic_pointer_cast<drawing::actors::Layers>(root);
    REQUIRE(layers != nullptr);
    REQUIRE(layers->getMainLayer() == nullptr);
    REQUIRE(layers->getSize() == 1);
    REQUIRE(layers->getWidth() == Catch::Approx(100));
    REQUIRE(layers->getHeight() == Catch::Approx(100));
}