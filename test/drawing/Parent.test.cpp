//
// Created by bobini on 10.11.22.
//

#include "drawing/actors/VBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("The parent's lifetime is not extended by child", "[drawing][actors][parent]")
{
    auto child = drawing::actors::Quad::make();
    {
        auto parent = drawing::actors::VBox::make();
        parent->addChild(child);
    }
    REQUIRE(child.use_count() == 1);
    REQUIRE(child->getParent() == nullptr);
}

TEST_CASE("Parent can be reassigned", "[drawing][actors][parent]")
{
    auto child = drawing::actors::Quad::make();
    auto parent1 = drawing::actors::VBox::make();
    auto parent2 = drawing::actors::VBox::make();
    parent1->addChild(child);
    parent2->addChild(child);
    REQUIRE(child->getParent() == parent2);
}