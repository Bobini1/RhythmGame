//
// Created by bobini on 19.11.22.
//

#include <catch2/catch_approx.hpp>
#include "drawing/actors/Text.h"
#include "catch2/catch_test_macros.hpp"
#include "../setupState.h"

static constexpr auto scriptWithArrayConstructor = R"(
    local text = Text.new{text="Hello", characterSize=5,
        fillColor = {0, 0, 0, 200}, outlineColor = {0, 0, 0, 201},
        outlineThickness = 5,
        lineSpacing = 22,
        letterSpacing = 33
    }
    return text
)";

TEST_CASE("Text can be constructed from lua", "[drawing][actors][text]")
{
    auto stateSetup = StateSetup{};
    auto state = sol::state(std::move(stateSetup));
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto text = std::dynamic_pointer_cast<drawing::actors::Text>(root);
    REQUIRE(text != nullptr);
    REQUIRE(text->getText() == "Hello");
    REQUIRE(text->getCharacterSize() == 5);
    REQUIRE(text->getFillColor() == sf::Color(0, 0, 0, 200));
    REQUIRE(text->getOutlineColor() == sf::Color(0, 0, 0, 201));
    REQUIRE(text->getOutlineThickness() == Catch::Approx(5));
    REQUIRE(text->getLineSpacing() == Catch::Approx(22));
    REQUIRE(text->getLetterSpacing() == Catch::Approx(33));
    REQUIRE(text->getFont() != nullptr);
}