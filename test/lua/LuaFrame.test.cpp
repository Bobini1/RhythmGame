//
// Created by bobini on 22.04.23.
//

#include "drawing/actors/Frame.h"
#include "drawing/actors/Quad.h"
#include "../setupState.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

static constexpr auto frameTest = R"(
local frame = Frame.new()
frame.width = 100
frame.height = 100
frame.offset = {10, 10}
frame.child = Quad.new()
frame.isWidthManaged = true
frame.isHeightManaged = true
frame.minWidth = 1
frame.minHeight = 1
return frame
)";

TEST_CASE("Frame can be constructed in lua", "[drawing][actors][Frame]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto frame =
      state.script(frameTest).get<std::shared_ptr<drawing::actors::Frame>>();
    REQUIRE(frame->getWidth() == Catch::Approx(100));
    REQUIRE(frame->getHeight() == Catch::Approx(100));
    REQUIRE(frame->getOffset() == sf::Vector2f(10, 10));
    REQUIRE(frame->getIsWidthManaged() == true);
    REQUIRE(frame->getIsHeightManaged() == true);
    REQUIRE(frame->getMinWidth() == Catch::Approx(1));
    REQUIRE(frame->getMinHeight() == Catch::Approx(1));

    auto child =
      std::dynamic_pointer_cast<drawing::actors::Quad>(frame->getChild());
    REQUIRE(child != nullptr);
}

static constexpr auto scriptWithArrayConstructor = R"(
local frame = Frame.new({width = 100,
                         height = 100,
                         offset = {10, 10},
                         child = Quad.new(),
                         isWidthManaged = true,
                         isHeightManaged = true,
                         minWidth = 1,
                         minHeight = 1})
return frame
)";

TEST_CASE("Frame with array constructor", "[drawing][actors][Frame]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto frame = state.script(scriptWithArrayConstructor)
                   .get<std::shared_ptr<drawing::actors::Frame>>();
    REQUIRE(frame->getWidth() == Catch::Approx(100));
    REQUIRE(frame->getHeight() == Catch::Approx(100));
    REQUIRE(frame->getOffset() == sf::Vector2f(10, 10));
    REQUIRE(frame->getIsWidthManaged() == true);
    REQUIRE(frame->getIsHeightManaged() == true);
    REQUIRE(frame->getMinWidth() == Catch::Approx(1));
    REQUIRE(frame->getMinHeight() == Catch::Approx(1));

    auto child =
      std::dynamic_pointer_cast<drawing::actors::Quad>(frame->getChild());
    REQUIRE(child != nullptr);
}