//
// Created by bobini on 19.11.22.
//

#include "drawing/actors/Layers.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Layers has the size of biggest children if no main layer is set",
          "[drawing][actors][Layers]")
{
    auto layers = drawing::actors::Layers::make();
    auto child1 = drawing::actors::Quad::make({ 100, 100 });
    auto child2 = drawing::actors::Quad::make({ 200, 200 });
    layers->addChild(child1);
    layers->addChild(child2);
    REQUIRE(layers->getWidth() == Catch::Approx(200));
    REQUIRE(layers->getHeight() == Catch::Approx(200));
}

TEST_CASE("Layers has the size of main layer if it is set",
          "[drawing][actors][Layers]")
{
    auto layers = drawing::actors::Layers::make();
    auto child1 = drawing::actors::Quad::make({ 100, 200 });
    auto child2 = drawing::actors::Quad::make({ 200, 100 });
    layers->addChild(child1);
    layers->addChild(child2);
    layers->setMainLayer(child2);
    REQUIRE(layers->getWidth() == Catch::Approx(200));
    REQUIRE(layers->getHeight() == Catch::Approx(100));
}

TEST_CASE("Layers sets the size of main layer if it is managed",
          "[drawing][actors][Layers]")
{
    auto layers = drawing::actors::Layers::make();
    auto child1 = drawing::actors::Quad::make({ 100, 100 });
    child1->setIsWidthManaged(true);
    child1->setIsHeightManaged(true);
    auto child2 = drawing::actors::Quad::make({ 200, 100 });
    auto child3 = drawing::actors::Quad::make({ 100, 100 });
    layers->addChild(child1);
    layers->addChild(child2);
    layers->addChild(child3);
    layers->setMainLayer(child2);
    SECTION("width managed")
    {
        child2->setIsWidthManaged(true);
        REQUIRE(layers->getIsWidthManaged() == true);
        REQUIRE(layers->getIsHeightManaged() == false);
        layers->setWidth(300);
        layers->setTransform(sf::Transform::Identity);
        REQUIRE(child2->getWidth() == Catch::Approx(300));
        REQUIRE(child2->getHeight() == Catch::Approx(100));
    }
    SECTION("height managed")
    {
        child2->setIsHeightManaged(true);
        REQUIRE(layers->getIsHeightManaged() == true);
        REQUIRE(layers->getIsWidthManaged() == false);
        layers->setHeight(300);
        layers->setTransform(sf::Transform::Identity);
        REQUIRE(child2->getWidth() == Catch::Approx(200));
        REQUIRE(child2->getHeight() == Catch::Approx(300));
    }
    SECTION("both managed")
    {
        child2->setIsHeightManaged(true);
        child2->setIsWidthManaged(true);
        REQUIRE(layers->getIsHeightManaged() == true);
        REQUIRE(layers->getIsWidthManaged() == true);
        layers->setWidth(300);
        layers->setHeight(300);
        layers->setTransform(sf::Transform::Identity);
        REQUIRE(child2->getWidth() == Catch::Approx(300));
        REQUIRE(child2->getHeight() == Catch::Approx(300));
    }

    REQUIRE(child1->getWidth() == Catch::Approx(child2->getWidth()));
    REQUIRE(child1->getHeight() == Catch::Approx(child2->getHeight()));
    REQUIRE(child3->getWidth() == Catch::Approx(100));
    REQUIRE(child3->getHeight() == Catch::Approx(100));
}
