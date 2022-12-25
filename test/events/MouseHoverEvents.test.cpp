//
// Created by bobini on 25.12.22.
//

#include <catch2/catch_approx.hpp>
#include "../setupState.h"
#include "catch2/catch_test_macros.hpp"
#include "drawing/actors/Quad.h"
#include "events/MouseHoverEvents.h"
#include "drawing/actors/Padding.h"

static constexpr auto scriptWithMouseEnterAndLeftEvent = R"(
    local quad = Quad.new{
                    width = 1,
                    height = 1,
                    events = {
                        mouseEnterEvent = function(self)
                                self.height = 100
                        end,
                        mouseLeftEvent = function(self)
                                self.height = 200
                        end
                    }
                }
    return quad
)";

TEST_CASE("MouseHoverEvents trigger once on enter and once on leave",
          "[events][mousehover]")
{
    auto setup = StateSetup{};
    auto& state = setup.getState();
    auto event = events::MouseHoverEvents{ &state };

    setup.addEventToState<>(event.onMouseEnter, "mouseEnter");
    setup.addEventToState<>(event.onMouseLeave, "mouseLeft");
    setup.defineTypes();
    auto result = state.script(scriptWithMouseEnterAndLeftEvent);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto quad = std::dynamic_pointer_cast<drawing::actors::Quad>(root);
    REQUIRE(quad != nullptr);
    REQUIRE(quad->getHeight() == Catch::Approx(1));
    event.update(*quad, { 0, 0 });
    REQUIRE(quad->getHeight() == Catch::Approx(100));
    event.update(*quad, { 0, 0 });
    REQUIRE(quad->getHeight() == Catch::Approx(100));
    event.update(*quad, { 1, 1 });
    REQUIRE(quad->getHeight() == Catch::Approx(200));
    event.update(*quad, { 1, 1 });
    REQUIRE(quad->getHeight() == Catch::Approx(200));
}