//
// Created by bobini on 10.11.22.
//

#include "drawing/actors/VBox.h"
#include "drawing/actors/Quad.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("The parent's lifetime is not extended by child", "[drawing][parent]")
{
    auto child = std::make_shared<drawing::actors::Quad>();
    {
        auto parent = std::make_shared<drawing::actors::VBox>();
        parent->addChild(child);
    }
    REQUIRE(child.use_count() == 1);
    REQUIRE(child->getParent() == nullptr);
}

TEST_CASE("Parent can be reassigned", "[drawing][parent]")
{
    auto child = std::make_shared<drawing::actors::Quad>();
    auto parent1 = std::make_shared<drawing::actors::VBox>();
    auto parent2 = std::make_shared<drawing::actors::VBox>();
    parent1->addChild(child);
    parent2->addChild(child);
    REQUIRE(child->getParent() == parent2);
}