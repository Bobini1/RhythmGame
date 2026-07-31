#include "gameplay_logic/ChartStartGate.h"

#include <catch2/catch_test_macros.hpp>

using gameplay_logic::ChartStartGate;

TEST_CASE("a normal start waits for readiness and then launches")
{
    auto gate = ChartStartGate{};

    CHECK_FALSE(gate.requestStart(false));
    CHECK(gate.hasPendingStart());
    CHECK(gate.onReady());
    CHECK_FALSE(gate.hasPendingStart());
}

TEST_CASE("a held start remains latched until release")
{
    auto gate = ChartStartGate{};

    gate.hold();
    CHECK_FALSE(gate.requestStart(true));
    CHECK_FALSE(gate.onReady());
    CHECK(gate.isHeld());
    CHECK(gate.hasPendingStart());

    CHECK(gate.release(true));
    CHECK_FALSE(gate.isHeld());
    CHECK_FALSE(gate.hasPendingStart());
}

TEST_CASE("release is idempotent and never invents a start request")
{
    auto gate = ChartStartGate{};

    gate.hold();
    CHECK_FALSE(gate.release(true));
    CHECK_FALSE(gate.release(true));

    gate.hold();
    CHECK_FALSE(gate.requestStart(false));
    CHECK_FALSE(gate.release(false));
    CHECK(gate.hasPendingStart());
    CHECK(gate.onReady());
}

TEST_CASE("reset cancels both the hold and the pending request")
{
    auto gate = ChartStartGate{};
    gate.hold();
    CHECK_FALSE(gate.requestStart(false));

    gate.reset();

    CHECK_FALSE(gate.isHeld());
    CHECK_FALSE(gate.hasPendingStart());
    CHECK_FALSE(gate.onReady());
}
