//
// Created by bobini on 05.11.22.
//

#include "drawing/actors/VBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Vbox is able to arrange its children", "[drawing][vbox]")
{
    auto vbox1 = std::make_shared<drawing::actors::VBox>();
    auto vbox2 = std::make_shared<drawing::actors::VBox>();
    auto child1 = std::make_shared<drawing::actors::Quad>();
    auto child2 = std::make_shared<drawing::actors::Quad>();
    auto child3 = std::make_shared<drawing::actors::Quad>();
    child1->setSize({ 100, 100 });
    child2->setSize({ 100, 100 });
    child3->setSize({ 100, 100 });
    vbox1->addChild(child1);
    vbox1->addChild(child2);
    vbox2->addChild(vbox1);
    vbox2->addChild(child3);
    ;
    vbox2->setTransform(sf::Transform::Identity);
    REQUIRE(child1->getTransform() == sf::Transform::Identity);
    REQUIRE(child2->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(0, 100));
    REQUIRE(child3->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(0, 200));
}