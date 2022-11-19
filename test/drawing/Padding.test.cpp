//
// Created by bobini on 19.11.22.
//

#include "drawing/actors/Padding.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Padding is able to arrange its child", "[drawing][padding]")
{
    auto padding1 = drawing::actors::Padding::make();
    auto padding2 = drawing::actors::Padding::make();
    padding1->setTop(10);
    padding1->setBottom(10);
    padding1->setLeft(10);
    padding1->setRight(10);
    padding2->setTop(20);
    padding2->setBottom(20);
    padding2->setLeft(20);
    padding2->setRight(20);
    auto child1 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    padding1->setChild(child1);
    padding2->setChild(padding1);
    padding2->setTransform(sf::Transform::Identity);
    REQUIRE(padding2->getTransform() == sf::Transform::Identity);
    REQUIRE(padding2->getWidth() == Catch::Approx(160));
    REQUIRE(padding2->getHeight() == Catch::Approx(160));
    REQUIRE(padding1->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(20, 20));
    REQUIRE(padding1->getWidth() == Catch::Approx(120));
    REQUIRE(padding1->getHeight() == Catch::Approx(120));
    REQUIRE(child1->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(30, 30));
    REQUIRE(child1->getWidth() == Catch::Approx(100));
    REQUIRE(child1->getHeight() == Catch::Approx(100));
}

TEST_CASE("Padding can be managed if child is managed", "[drawing][padding]") {}