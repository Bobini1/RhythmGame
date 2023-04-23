//
// Created by bobini on 19.11.22.
//

#include "drawing/actors/Align.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Align sets the position of its child according to the mode",
          "[drawing][actors][Align]")
{
    auto align = drawing::actors::Align::make();
    auto child = drawing::actors::Quad::make({ 100, 100 });
    SECTION("Align::Left")
    {
        align->setMode(drawing::actors::Align::Mode::Left);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(0, 50));
    }
    SECTION("Align::Right")
    {
        align->setMode(drawing::actors::Align::Mode::Right);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(100, 50));
    }
    SECTION("Align::Center")
    {
        align->setMode(drawing::actors::Align::Mode::Center);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(50, 50));
    }
    SECTION("Align::Top")
    {
        align->setMode(drawing::actors::Align::Mode::Top);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(50, 0));
    }
    SECTION("Align::Bottom")
    {
        align->setMode(drawing::actors::Align::Mode::Bottom);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(50, 100));
    }

    SECTION("Align::TopLeft")
    {
        align->setMode(drawing::actors::Align::Mode::TopLeft);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(0, 0));
    }

    SECTION("Align::TopRight")
    {
        align->setMode(drawing::actors::Align::Mode::TopRight);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(100, 0));
    }

    SECTION("Align::BottomLeft")
    {
        align->setMode(drawing::actors::Align::Mode::BottomLeft);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(0, 100));
    }

    SECTION("Align::BottomRight")
    {
        align->setMode(drawing::actors::Align::Mode::BottomRight);
        align->setChild(child);
        align->setWidth(200);
        align->setHeight(200);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(100, 100));
    }

    REQUIRE(child->getWidth() == Catch::Approx(100));
    REQUIRE(child->getHeight() == Catch::Approx(100));
}

TEST_CASE("Align resizes the child to full width if the sizes are managed",
          "[drawing][actors][Align]")
{
    auto align = drawing::actors::Align::make();
    auto child = drawing::actors::Quad::make({ 100, 100 });
    align->setMode(drawing::actors::Align::Mode::Center);
    align->setChild(child);
    align->setWidth(200);
    align->setHeight(200);
    SECTION("width managed")
    {
        child->setIsWidthManaged(/*isWidthManaged=*/true);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getWidth() == Catch::Approx(200));
        REQUIRE(child->getHeight() == Catch::Approx(100));
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(0, 50));
    }
    SECTION("height managed")
    {
        child->setIsHeightManaged(/*isHeightManaged=*/true);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getWidth() == Catch::Approx(100));
        REQUIRE(child->getHeight() == Catch::Approx(200));
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(50, 0));
    }
    SECTION("both managed")
    {
        child->setIsWidthManaged(/*isWidthManaged=*/true);
        child->setIsHeightManaged(/*isHeightManaged=*/true);
        align->setTransform(sf::Transform::Identity);
        REQUIRE(child->getWidth() == Catch::Approx(200));
        REQUIRE(child->getHeight() == Catch::Approx(200));
        REQUIRE(child->getTransform() ==
                sf::Transform(sf::Transform::Identity).translate(0, 0));
    }
}

TEST_CASE("Align reports the element at the mouse position",
          "[drawing][actors][align]")
{
    auto align = drawing::actors::Align::make();
    auto child = drawing::actors::Quad::make({ 100, 100 });
    child->setIsObstructing(false);
    align->setMode(drawing::actors::Align::Mode::Center);
    align->setChild(child);
    align->setWidth(200);
    align->setHeight(200);
    align->setTransform(sf::Transform::Identity);
    std::set<std::weak_ptr<const drawing::actors::Actor>,
             std::owner_less<std::weak_ptr<const drawing::actors::Actor>>>
      result;
    SECTION("mouse inside")
    {
        align->getAllActorsAtMousePosition({ 50, 50 }, result);
        REQUIRE(result.size() == 2);
        REQUIRE(result.contains(align));
        REQUIRE(result.contains(child));
    }
    SECTION("mouse outside")
    {
        align->getAllActorsAtMousePosition({ 25, 25 }, result);
        REQUIRE(result.size() == 1);
        REQUIRE(result.contains(align));
    }
}