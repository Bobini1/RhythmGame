//
// Created by bobini on 05.11.22.
//

//
// Created by bobini on 05.11.22.
//

#include "drawing/actors/HBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Hbox is able to arrange its children", "[drawing][hbox]")
{
    auto hbox1 = std::make_shared<drawing::actors::HBox>();
    auto hbox2 = std::make_shared<drawing::actors::HBox>();
    auto child1 = std::make_shared<drawing::actors::Quad>();
    auto child2 = std::make_shared<drawing::actors::Quad>();
    auto child3 = std::make_shared<drawing::actors::Quad>();
    child1->setSize({ 100, 100 });
    child2->setSize({ 100, 100 });
    child3->setSize({ 100, 100 });
    hbox1->addChild(child1);
    hbox1->addChild(child2);
    hbox2->addChild(hbox1);
    hbox2->addChild(child3);
    hbox2->setTransform(sf::Transform::Identity);
    REQUIRE(child1->getTransform() == sf::Transform::Identity);
    REQUIRE(child2->getTransform() == sf::Transform(sf::Transform::Identity).translate(100, 0));
    REQUIRE(child3->getTransform() == sf::Transform(sf::Transform::Identity).translate(200, 0));
}