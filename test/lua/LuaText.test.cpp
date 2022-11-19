//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Text.h"
#include "catch2/catch_test_macros.hpp"
#include "setupState.h"

// TODO: add other parameters when texture management is implemented
static constexpr auto scriptWithArrayConstructor = R"(
    local text = Text.new()
    return text
)";

TEST_CASE("Text can be constructed from lua", "[drawing][actors][text]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto text = std::dynamic_pointer_cast<drawing::actors::Text>(root);
    REQUIRE(text != nullptr);
}