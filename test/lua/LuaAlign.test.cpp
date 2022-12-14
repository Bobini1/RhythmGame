//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Align.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local align = Align.new{mode = AlignMode.TopLeft, child = Quad.new{}}
    return align
)";

TEST_CASE("Align can be constructed from lua", "[drawing][actors][align]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto align = std::dynamic_pointer_cast<drawing::actors::Align>(root);
    REQUIRE(align != nullptr);
    REQUIRE(align->getMode() == drawing::actors::Align::Mode::TopLeft);
    REQUIRE(align->getChild() != nullptr);
}

static constexpr auto scriptWithParensConstructor = R"(
    local align = Align.new(Quad.new{}, AlignMode.Top)
    return align
)";

TEST_CASE("Align can be constructed from lua with parens",
          "[drawing][actors][align]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithParensConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto align = std::dynamic_pointer_cast<drawing::actors::Align>(root);
    REQUIRE(align != nullptr);
    REQUIRE(align->getMode() == drawing::actors::Align::Mode::Top);
    REQUIRE(align->getChild() != nullptr);
}

static constexpr auto scriptWithParensConstructorDefaultCenter = R"(
    local align = Align.new(Quad.new{})
    return align
)";

TEST_CASE("Align can be constructed from lua with parens and default center",
          "[drawing][actors][align]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithParensConstructorDefaultCenter);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto align = std::dynamic_pointer_cast<drawing::actors::Align>(root);
    REQUIRE(align != nullptr);
    REQUIRE(align->getMode() == drawing::actors::Align::Mode::Center);
    REQUIRE(align->getChild() != nullptr);
}