//
// Created by bobini on 05.11.22.
//

//
// Created by bobini on 05.11.22.
//

#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Hbox is able to arrange its children", "[drawing][actors][hbox]")
{
    auto hbox1 = drawing::actors::HBox::make();
    auto hbox2 = drawing::actors::HBox::make();
    auto child1 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child2 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child3 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    hbox1->addChild(child1);
    hbox1->addChild(child2);
    hbox2->addChild(hbox1);
    hbox2->addChild(child3);
    hbox2->setTransform(sf::Transform::Identity);
    REQUIRE(child1->getTransform() == sf::Transform::Identity);
    REQUIRE(child2->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(100, 0));
    REQUIRE(child3->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(200, 0));
    REQUIRE(hbox1->getTransform() == sf::Transform::Identity);
}