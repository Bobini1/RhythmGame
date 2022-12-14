//
// Created by bobini on 14/12/2022.
//

#include <catch2/catch_approx.hpp>
#include "drawing/animations/Linear.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local linear = Linear.new{duration = 1, from = 0, to = 1}
    return linear
)";

TEST_CASE("Linear can be constructed from lua", "[drawing][animations][linear]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithArrayConstructor);
    auto root =
      result.get<drawing::animations::Animation*>()->shared_from_this();
    auto linear = std::dynamic_pointer_cast<drawing::animations::Linear>(root);
    REQUIRE(linear != nullptr);
    REQUIRE(linear->getDuration() == std::chrono::seconds{ 1 });
    REQUIRE(linear->getFrom() == Catch::Approx(0));
    REQUIRE(linear->getTo() == Catch::Approx(1));
}
