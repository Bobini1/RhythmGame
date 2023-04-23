//
// Created by bobini on 05.11.22.
//

#include "drawing/actors/VBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Vbox is able to arrange its children", "[drawing][actors][vbox]")
{
    auto vbox1 = drawing::actors::VBox::make();
    auto vbox2 = drawing::actors::VBox::make();
    auto child1 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child2 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child3 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    vbox1->addChild(child1);
    vbox1->addChild(child2);
    vbox2->addChild(vbox1);
    vbox2->addChild(child3);
    vbox2->setTransform(sf::Transform::Identity);
    REQUIRE(child1->getTransform() == sf::Transform::Identity);
    REQUIRE(child2->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(0, 100));
    REQUIRE(child3->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(0, 200));
}

TEST_CASE("VBox items can be removed and added", "[drawing][actors][vbox]")
{
    auto vbox = drawing::actors::VBox::make();
    auto child1 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child2 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child3 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    vbox->addChild(child1);
    vbox->addChild(child2);
    vbox->addChild(child3);
    vbox->removeChild(child2);
    vbox->addChild(child2);
    vbox->removeChild(child1);
    vbox->setTransform(sf::Transform::Identity);
    REQUIRE(child2->getTransform() ==
            sf::Transform(sf::Transform::Identity).translate(0, 100));
    REQUIRE(child3->getTransform() == sf::Transform::Identity);
}

TEST_CASE("VBox reports the element at the mouse position",
          "[drawing][actors][vbox]")
{
    auto vbox = drawing::actors::VBox::make();
    auto child1 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    child1->setIsObstructing(false);
    auto child2 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    auto child3 = drawing::actors::Quad::make(sf::Vector2f{ 100.F, 100.F });
    vbox->addChild(child1);
    vbox->addChild(child2);
    vbox->addChild(child3);
    vbox->setTransform(sf::Transform::Identity);
    std::set<std::weak_ptr<const drawing::actors::Actor>,
             std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
      result;
    vbox->getAllActorsAtMousePosition(sf::Vector2f{ 50.F, 50.F }, result);
    REQUIRE(result.size() == 2);
    REQUIRE(result.contains(child1));
    REQUIRE(result.contains(vbox));
}