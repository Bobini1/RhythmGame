//
// Created by bobini on 21.11.22.
//

#include <catch2/catch_approx.hpp>
#include "../setupState.h"
#include "catch2/catch_test_macros.hpp"
#include "drawing/actors/Quad.h"
#include "events/Signals2Event.h"

static constexpr auto script = R"(
    local quad = Quad.new{
                    events = {initEvent = function(self) self.width = 100 end}
                }
    return quad
)";

TEST_CASE("Events can be subscribed to in lua", "[actors][events]")
{
    StateSetup setup;
    events::Signals2Event<> event;
    setup.addEventToState<>(event, "init");
    auto state = sol::state(std::move(setup));
    auto result = state.script(script);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event();
    REQUIRE(quad->getWidth() == Catch::Approx(100));
}

static constexpr auto scriptWithArgs = R"(
    local quad = Quad.new{
                    events = {initEvent = function(self, val) self.width = val end}
                }
    return quad
)";

TEST_CASE("Events with args can be subscribed to in lua", "[actors][events]")
{
    StateSetup setup;
    events::Signals2Event<int> event;
    setup.addEventToState<int>(event, "init");
    auto state = sol::state(std::move(setup));
    auto result = state.script(script);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    std::as_const(event)(100);
    REQUIRE(quad->getWidth() == Catch::Approx(100));
}

static constexpr auto scriptWithDeletedCallback = R"(
    local quad = Quad.new{
                    events = {initEvent = function(self, val, val2) self.width = val + val2 end}
                }
    quad.initEvent = nil
    return quad
)";

TEST_CASE("Events with args can be subscribed to in lua and deleted",
          "[actors][events]")
{
    StateSetup setup;
    events::Signals2Event<int, int> event;
    setup.addEventToState<int, int>(event, "init");
    auto state = sol::state(std::move(setup));
    auto result = state.script(scriptWithDeletedCallback);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    std::as_const(event)(100, 200);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
}

static constexpr auto scriptWithBadOperations =
  R"(
    local quad = Quad.new{
                    events = {initEvent = 1,
                              invalid = function(self, val, val2) self.width = val + val2 end}
                }
    quad.initEvent = nil
    return quad
)";

TEST_CASE("Subscriptions to invalid event names or not function don't crash",
          "[actors][events]")
{
    StateSetup setup;
    events::Signals2Event<int, int> event;
    setup.addEventToState<int, int>(event, "init");
    auto state = sol::state(std::move(setup));
    auto result = state.script(scriptWithDeletedCallback);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
}