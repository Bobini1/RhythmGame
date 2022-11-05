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
    auto HBox1 = std::make_shared<drawing::actors::HBox>();
    auto HBox2 = std::make_shared<drawing::actors::HBox>();
    auto child1 = std::make_shared<drawing::actors::Quad>();
    auto child2 = std::make_shared<drawing::actors::Quad>();
    auto child3 = std::make_shared<drawing::actors::Quad>();
    child1->setSize({ 100, 100 });
    child2->setSize({ 100, 100 });
    child3->setSize({ 100, 100 });
    HBox1->addChild(child1);
    HBox1->addChild(child2);
    HBox2->addChild(HBox1);
    HBox2->addChild(child3);
    HBox2->setLayout({0, 0, 300, 300});
    REQUIRE(child1->getLayout() == sf::FloatRect{0, 0, 100, 100});
    REQUIRE(child2->getLayout() == sf::FloatRect{100, 0, 100, 100});
    REQUIRE(child3->getLayout() == sf::FloatRect{200, 0, 100, 100});

}