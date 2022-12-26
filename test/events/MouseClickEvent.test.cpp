//
// Created by bobini on 25.12.22.
//

#include <catch2/catch_approx.hpp>
#include "../setupState.h"
#include "catch2/catch_test_macros.hpp"
#include "drawing/actors/Quad.h"
#include "events/MouseClickEvent.h"
#include "drawing/actors/Padding.h"
#include "drawing/actors/Layers.h"

static constexpr auto scriptWithMouseEvent = R"(
    local quad = Quad.new{
                    width = 1,
                    height = 1,
                    onLeftClick = function(self)
                            self.width = self.width + 100
                    end
                }
    local padding = Padding.new{child = quad, onLeftClick = function(self) self.top = self.top + 100 end}
    return padding
)";

TEST_CASE("MouseClickEvent calls all clicked actors", "[events][mouseclick]")
{
    auto setup = StateSetup{};
    auto& state = setup.getState();
    auto event = events::MouseClickEvent{ &state };

    setup.addEventToState<>(event, "leftClick");
    setup.defineTypes();
    auto result = state.script(scriptWithMouseEvent);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto padding = std::dynamic_pointer_cast<drawing::actors::Padding>(root);
    REQUIRE(padding != nullptr);
    auto quad =
      std::dynamic_pointer_cast<drawing::actors::Quad>(padding->getChild());
    root->setTransform(sf::Transform::Identity);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getWidth() == Catch::Approx(1));
    event(*padding, { 0, 0 });
    root->setTransform(sf::Transform::Identity);
    REQUIRE(quad->getWidth() == Catch::Approx(101));
    REQUIRE(padding->getTop() == Catch::Approx(100));
    event(*padding, { 0, 0 });
    root->setTransform(sf::Transform::Identity);
    REQUIRE(quad->getWidth() == Catch::Approx(101));
    REQUIRE(padding->getTop() == Catch::Approx(200));
}

static constexpr auto scriptWithObstructingLayer = R"(
    local quad1 = Quad.new{
                    width = 1,
                    height = 1,
                    onLeftClick = function(self)
                            self.width = self.width + 100
                    end
                }
    local quad2 = Quad.new{
                    width = 1,
                    height = 1,
                    onLeftClick = function(self)
                            self.width = self.width + 100
                    end,
                    isObstructing = true
                }
    local layers = Layers.new{children = {quad1, quad2}}
    return layers
)";

TEST_CASE("MouseClickEvent does not touch obstructed actors",
          "[events][mouseclick]")
{
    auto setup = StateSetup{};
    auto& state = setup.getState();
    auto event = events::MouseClickEvent{ &state };

    setup.addEventToState<>(event, "leftClick");
    setup.defineTypes();
    auto result = state.script(scriptWithObstructingLayer);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto layers = std::dynamic_pointer_cast<drawing::actors::Layers>(root);
    REQUIRE(layers != nullptr);
    auto quad1 =
      std::dynamic_pointer_cast<drawing::actors::Quad>(layers->operator[](0));
    REQUIRE(quad1 != nullptr);
    auto quad2 =
      std::dynamic_pointer_cast<drawing::actors::Quad>(layers->operator[](1));
    REQUIRE(quad2 != nullptr);
    REQUIRE(quad1->getWidth() == Catch::Approx(1));
    REQUIRE(quad2->getWidth() == Catch::Approx(1));
    event(*layers, { 0, 0 });
    REQUIRE(quad1->getWidth() == Catch::Approx(1));
    REQUIRE(quad2->getWidth() == Catch::Approx(101));
}