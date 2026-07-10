#include "arena/ArenaAvailabilityIndex.h"

#include <catch2/catch_test_macros.hpp>

#include <QByteArray>
#include <QString>

TEST_CASE("Arena availability exposes the exact QML chart query",
          "[arena][ArenaSelect]")
{
    const auto commonHash = QString(64, QLatin1Char('a'));
    const auto uncommonHash = QString(64, QLatin1Char('b'));
    auto index = arena::ArenaAvailabilityIndex{};

    REQUIRE(index.metaObject()->indexOfMethod("availabilityFor(QString)") >= 0);
    CHECK(index.availabilityFor(commonHash) ==
          arena::ArenaAvailabilityIndex::Availability::NotApplicable);

    REQUIRE(index.applyReset(7, QByteArray::fromHex(commonHash.toLatin1())));
    CHECK(index.availabilityFor(commonHash) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(index.availabilityFor(uncommonHash) ==
          arena::ArenaAvailabilityIndex::Availability::UnavailableToSome);

    index.setSyncing();
    CHECK(index.availabilityFor(commonHash) ==
          arena::ArenaAvailabilityIndex::Availability::Syncing);
}
