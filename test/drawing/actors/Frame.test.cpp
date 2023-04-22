//
// Created by bobini on 22.04.23.
//

#include "drawing/actors/Frame.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("Frame correctly positions child", "[drawing][actors][Frame]")
{
    auto frame = drawing::actors::Frame::make();
    auto child = drawing::actors::Quad::make();
    frame->setChild(std::move(child));
    frame->setWidth(100);
    frame->setHeight(100);
    frame->setOffset({ 10, 10 });

    frame->setTransform(sf::Transform::Identity);
    REQUIRE(frame->getTransform() == sf::Transform::Identity);
    REQUIRE(frame->getGlobalBounds() == sf::FloatRect(0, 0, 100, 100));
    REQUIRE(frame->getChild()->getGlobalBounds() ==
            sf::FloatRect(10, 10, 0, 0));
}