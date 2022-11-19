//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Align.h"
#include "catch2/catch_test_macros.hpp"
#include "setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local align = Align.new{mode = AlignMode.Center, child = Quad.new{}}
    return align
)";

TEST_CASE("Align can be constructed from lua", "[drawing][actors][align]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto align = std::dynamic_pointer_cast<drawing::actors::Align>(root);
    REQUIRE(align != nullptr);
    REQUIRE(align->getMode() == drawing::actors::Align::Mode::Center);
    REQUIRE(align->getChild() != nullptr);
}
