//
// Created by bobini on 19.11.22.
//

#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "setupState.h"

static constexpr auto luaScript = R"(
    local root = HBox.new()
    root.spacing = 10
    local quad = Quad.new()
    quad.width = 100
    quad.height = 100
    quad.outlineColor = Color.new(0, 0, 0, 255)
    quad.outlineThickness = 1
    quad.fillColor = Color.new(255, 0, 0, 255)
    local quad2 = Quad.new()
    quad2.width = 100
    quad2.height = 100
    quad2.fillColor = Color.new(0, 255, 0, 255)
    quad2.outlineColor = Color.new(0, 0, 0, 255)
    quad2.outlineThickness = 1
    local quad3 = Quad.new()
    quad3.width = 100
    quad3.height = 100
    quad3.fillColor = Color.new(0, 0, 255, 255)
    quad3.outlineColor = Color.new(0, 0, 0, 255)
    quad3.outlineThickness = 1
    root:addChild(quad)
    root:addChild(quad2)
    root:addChild(quad3)
    root:getChild(1).width = 200
    return root
)";

TEST_CASE("Lua Hbox is able to arrange its children", "[drawing][hbox][lua]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(luaScript);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto hbox = std::dynamic_pointer_cast<drawing::actors::HBox>(root);
    hbox->setTransform(sf::Transform::Identity);
    REQUIRE(hbox != nullptr);
    REQUIRE(hbox->getSize() == 3);
    REQUIRE(hbox->getSpacing() == Catch::Approx(10));
    auto child1 = std::dynamic_pointer_cast<drawing::actors::Quad>((*hbox)[0]);
    auto child2 = std::dynamic_pointer_cast<drawing::actors::Quad>((*hbox)[1]);
    auto child3 = std::dynamic_pointer_cast<drawing::actors::Quad>((*hbox)[2]);
    REQUIRE(child1 != nullptr);
    REQUIRE(child2 != nullptr);
    REQUIRE(child3 != nullptr);
    REQUIRE(child1->getWidth() == Catch::Approx(200));
    REQUIRE(child2->getWidth() == Catch::Approx(100));
    REQUIRE(child3->getWidth() == Catch::Approx(100));
    REQUIRE(child1->getHeight() == Catch::Approx(100));
    REQUIRE(child2->getHeight() == Catch::Approx(100));
    REQUIRE(child3->getHeight() == Catch::Approx(100));
    REQUIRE(child1->getTransform() == sf::Transform::Identity);
    REQUIRE(child2->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(210, 0));
    REQUIRE(child3->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(320, 0));
    REQUIRE(hbox->getTransform() == sf::Transform::Identity);
    REQUIRE(child1->getFillColor() == sf::Color(255, 0, 0, 255));
    REQUIRE(child2->getFillColor() == sf::Color(0, 255, 0, 255));
    REQUIRE(child3->getFillColor() == sf::Color(0, 0, 255, 255));
    REQUIRE(child1->getOutlineColor() == sf::Color(0, 0, 0, 255));
    REQUIRE(child2->getOutlineColor() == sf::Color(0, 0, 0, 255));
    REQUIRE(child3->getOutlineColor() == sf::Color(0, 0, 0, 255));
    REQUIRE(child1->getOutlineThickness() == Catch::Approx(1));
    REQUIRE(child2->getOutlineThickness() == Catch::Approx(1));
    REQUIRE(child3->getOutlineThickness() == Catch::Approx(1));
}

static constexpr auto scriptWithArrayConstructor = R"(
return HBox.new{spacing = 10, width = 100, height = 100,
                verticalSizeMode = SizeMode.Managed,
                horizontalSizeMode = SizeMode.Managed,
                contentAlignment = HBoxContentAlignment.Center,
                children = {Quad.new()}}
)";

TEST_CASE("HBox constructor parameters get set correctly", "[drawing][hbox]")
{
    auto state = getStateWithAllDefinitions();
    auto result = state.script(scriptWithArrayConstructor);
    auto root = result.get<drawing::actors::Actor*>()->shared_from_this();
    auto hbox = std::dynamic_pointer_cast<drawing::actors::HBox>(root);
    REQUIRE(hbox != nullptr);
    REQUIRE(hbox->getSpacing() == Catch::Approx(10));
    REQUIRE(hbox->getWidth() == Catch::Approx(100));
    REQUIRE(hbox->getHeight() == Catch::Approx(100));
    REQUIRE(hbox->getIsWidthManaged());
    REQUIRE(hbox->getIsHeightManaged());
    REQUIRE(hbox->getSize() == 1);
    REQUIRE(hbox->getContentAlignment() ==
            drawing::actors::HBox::ContentAlignment::Center);
}