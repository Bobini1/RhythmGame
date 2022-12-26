//
// Created by bobini on 21.11.22.
//

#include <catch2/catch_approx.hpp>
#include "../setupState.h"
#include "catch2/catch_test_macros.hpp"
#include "drawing/actors/Quad.h"
#include "events/GlobalEvent.h"

static constexpr auto script = R"(
    local quad = Quad.new{
                    onInit = function(self) self.width = 100 end
                }
    return quad
)";

TEST_CASE("Events can be subscribed to in lua", "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<> event{ &setup.getState() };
    setup.addEventToState<>(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
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
                    onInit = function(self, val) self.width = val end
                }
    return quad
)";

TEST_CASE("Events with args can be subscribed to in lua", "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithArgs);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event(100);
    REQUIRE(quad->getWidth() == Catch::Approx(100));
}

static constexpr auto scriptWithDeletedCallback = R"(
    local quad = Quad.new{
                    onInit = function(self, val, val2) self.width = val + val2 end
                }
    quad.onInit = nil
    return quad
)";

TEST_CASE("Events with args can be subscribed to in lua and deleted",
          "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int, int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithDeletedCallback);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event(100, 200);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
}

static constexpr auto scriptWithBadOperations =
  R"(
    local quad = Quad.new{
                    initEvent = 1,
                    onInvalid = function(self, val, val2) self.width = val + val2 end
                }
    quad.onInit = 1
    return quad
)";

TEST_CASE("Subscriptions to invalid event names or not function don't crash",
          "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int, int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithBadOperations);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
}

static constexpr auto scriptWithLocalFunction = R"(
    local quad = Quad.new{
                    onInit = function(self, val, val2) self.width = val + val2 end
                }
    local getFunc = function()
        local function localFunc(self, val, val2)
            self.width = val + val2
        end
        return localFunc
    end

    quad.onInit = getFunc()
    return quad
)";

TEST_CASE("Subscribing with local functions works", "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int, int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithLocalFunction);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event(100, 200);
    REQUIRE(quad->getWidth() == Catch::Approx(300));
}

static constexpr auto scriptWithGetter = R"(
    local quad = Quad.new{
                    onInit = function(self, val, val2) self.width = val + val2 end
                }
    local otherQuad = Quad.new{
                    onInit = function(self, val, val2) self.width = val * val2 end
                }
    otherQuad.onInit = quad.onInit
    return otherQuad
)";

TEST_CASE("Subscriptions can be copied from one actor to another",
          "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int, int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithGetter);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event(100, 200);
    REQUIRE(quad->getWidth() == Catch::Approx(300));
}

static constexpr auto scriptWithUnsubscribe = R"(
    local quad = Quad.new{
                    onInit = function(self, val, val2) self.width = val + val2 end
                }
    quad.onInit = nil
    return quad
)";

TEST_CASE("Subscriptions can be unsubscribed from with nil", "[actors][events]")
{
    StateSetup setup;
    events::GlobalEvent<int, int> event{ &setup.getState() };
    setup.addEventToState(event, "init");
    setup.defineTypes();
    auto& state = setup.getState();
    auto result = state.script(scriptWithUnsubscribe);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
    event(100, 200);
    REQUIRE(quad->getWidth() == Catch::Approx(0));
}