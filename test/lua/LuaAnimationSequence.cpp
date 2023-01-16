//
// Created by bobini on 14/12/2022.
//

#include <catch2/catch_approx.hpp>
#include "drawing/animations/Linear.h"
#include "drawing/animations/AnimationSequence.h"
#include "drawing/actors/Quad.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto script = R"(
return function(quad)
    linear = Linear.new{duration = 1, from = 0, to = 1,
        onFinished = function()
            if not linear then return nil end
            if linear.isFinished then
                linear.getIsLooping = true
                quad.width = 500
            end
        end
    }
    linear2 = Linear.new{duration = 1, from = 0, to = 1,
        onFinished = function()
            if not linear2 then return nil end
            if linear2.isFinished then
                quad.height = 1000
            end
        end
    }
    sequence = AnimationSequence.new{animations = {linear, linear2}}
    sequence.duration = 4

    return sequence
end
)";

TEST_CASE("Animation sequence properties work",
          "[drawing][animations][animationsequence]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto quad = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    const std::function<drawing::animations::Animation*(
      std::shared_ptr<drawing::actors::Quad>)>
      result = state.script(script);
    auto funcResult = result(quad)->shared_from_this();
    auto animationSequence =
      std::dynamic_pointer_cast<drawing::animations::AnimationSequence>(
        funcResult);
    REQUIRE(animationSequence != nullptr);
    REQUIRE(animationSequence->getDuration() == std::chrono::seconds{ 4 });
    REQUIRE(animationSequence->getProgress() == Catch::Approx(0));
    REQUIRE(animationSequence->getIsLooping() == false);
    animationSequence->update(std::chrono::seconds{ 2 });
    REQUIRE(animationSequence->getProgress() == Catch::Approx(0.5));
    REQUIRE(animationSequence->getIsFinished() == false);
    REQUIRE(animationSequence->getIsLooping() == false);
    REQUIRE(quad->getWidth() == Catch::Approx(500));
    REQUIRE(quad->getHeight() == Catch::Approx(100));
    animationSequence->update(std::chrono::seconds{ 2 });
    REQUIRE(animationSequence->getProgress() == Catch::Approx(1));
    REQUIRE(animationSequence->getIsFinished() == true);
    REQUIRE(animationSequence->getIsLooping() == false);
    REQUIRE(quad->getWidth() == Catch::Approx(500));
    REQUIRE(quad->getHeight() == Catch::Approx(1000));
}