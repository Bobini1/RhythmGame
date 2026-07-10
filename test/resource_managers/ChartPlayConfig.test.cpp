#include "resource_managers/ChartPlayConfig.h"

#include <catch2/catch_test_macros.hpp>

using resource_managers::ChartPlayConfig;
using resource_managers::ExactRandomSequence;

TEST_CASE("Arena chart play configuration defaults to ordinary play")
{
    const auto config = ChartPlayConfig{};

    CHECK(config.randomSequence.isEmpty());
    CHECK(config.noteOrderP1 == resource_managers::NoteOrderAlgorithm::Normal);
    CHECK(config.noteOrderP2 == resource_managers::NoteOrderAlgorithm::Normal);
    CHECK(config.dpMode == resource_managers::DpOptions::Off);
    CHECK(config.laneSeed == 0);
    CHECK(config.isSupported());
}

TEST_CASE("exact random sequence accepts every declared value exactly once")
{
    auto sequence = ExactRandomSequence({ 2, 1, 7 });

    CHECK(sequence.next(3) == 2);
    CHECK(sequence.next(1) == 1);
    CHECK(sequence.next(9) == 7);
    CHECK(sequence.complete());
}

TEST_CASE("exact random sequence rejects missing extra and out-of-range values")
{
    SECTION("missing")
    {
        auto sequence = ExactRandomSequence(QList<qint64>{});
        CHECK(sequence.next(4) == 1);
        CHECK_FALSE(sequence.complete());
    }

    SECTION("extra")
    {
        auto sequence = ExactRandomSequence({ 1, 2 });
        CHECK(sequence.next(1) == 1);
        CHECK_FALSE(sequence.complete());
    }

    SECTION("out of range")
    {
        auto sequence = ExactRandomSequence({ 3 });
        CHECK(sequence.next(2) == 1);
        CHECK_FALSE(sequence.complete());
    }
}

TEST_CASE("unknown randomization versions are rejected")
{
    auto config = ChartPlayConfig{};
    config.randomizationVersion = 2;

    CHECK_FALSE(config.isSupported());
}
