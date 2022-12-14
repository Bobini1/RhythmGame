//
// Created by PC on 14/12/2022.
//

#include <catch2/catch_approx.hpp>
#include "drawing/animations/Linear.h"
#include "drawing/actors/Quad.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto script = R"(
return function(quad)
    linear = Linear.new{duration = 1, from = 0, to = 1,
        onFinished = function()
            if not linear then return nil end
            if linear.isFinished then
                linear.isLooping = true
                quad.width = 500
            end
        end
    }
    linear.progress = 0.5
    linear.isLooping = false
    return linear
end
)";

TEST_CASE("Base animation properties work", "[drawing][animations][animation]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    const std::function<drawing::animations::Animation*(
      std::shared_ptr<drawing::actors::Quad>)>
      result = state.script(script);
    auto funcResult = result(quad)->shared_from_this();
    auto linear =
      std::dynamic_pointer_cast<drawing::animations::Linear>(funcResult);
    REQUIRE(linear != nullptr);
    REQUIRE(linear->getDuration() == std::chrono::seconds{ 1 });
    REQUIRE(linear->getFrom() == Catch::Approx(0));
    REQUIRE(linear->getTo() == Catch::Approx(1));
    REQUIRE(linear->getProgress() == Catch::Approx(0.5));
    REQUIRE(linear->getIsLooping() == false);
    linear->update(std::chrono::seconds{ 1 });
    REQUIRE(linear->getProgress() == Catch::Approx(1));
    REQUIRE(linear->getIsFinished() == true);
    REQUIRE(linear->getIsLooping() == true);
    REQUIRE(quad->getWidth() == Catch::Approx(500));
}