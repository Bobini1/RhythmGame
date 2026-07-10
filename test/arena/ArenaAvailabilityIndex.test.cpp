#include "arena/ArenaAvailabilityIndex.h"

#include <catch2/catch_test_macros.hpp>

#include <QByteArray>
#include <QtEndian>

namespace {

auto
hashRecord(quint32 value) -> QByteArray
{
    auto result = QByteArray(arena::ArenaSha256Bytes, '\0');
    qToBigEndian(value, result.data() + 28);
    return result;
}

auto
packed(std::initializer_list<quint32> values) -> QByteArray
{
    QByteArray result;
    result.reserve(static_cast<qsizetype>(values.size()) *
                   arena::ArenaSha256Bytes);
    for (const auto value : values) {
        result.append(hashRecord(value));
    }
    return result;
}

auto
hexHash(quint32 value) -> QString
{
    return QString::fromLatin1(hashRecord(value).toHex());
}

auto
packedSequence(int count) -> QByteArray
{
    auto result = QByteArray(count * arena::ArenaSha256Bytes, '\0');
    for (int i = 0; i < count; ++i) {
        qToBigEndian(static_cast<quint32>(i + 1),
                     result.data() + i * arena::ArenaSha256Bytes + 28);
    }
    return result;
}

} // namespace

TEST_CASE("ArenaAvailabilityIndex has explicit lifecycle states and lookup",
          "[arena][ArenaAvailability]")
{
    arena::ArenaAvailabilityIndex index;
    int changes = 0;
    QObject::connect(
      &index, &arena::ArenaAvailabilityIndex::changed, [&] { ++changes; });

    CHECK(index.state() == arena::ArenaAvailabilityIndex::State::NotApplicable);
    CHECK(index.revision() == 0);
    CHECK(index.availability(hexHash(1)) ==
          arena::ArenaAvailabilityIndex::Availability::NotApplicable);

    index.setSyncing();
    CHECK(index.state() == arena::ArenaAvailabilityIndex::State::Syncing);
    CHECK(index.availability(hexHash(1)) ==
          arena::ArenaAvailabilityIndex::Availability::Syncing);
    CHECK(changes == 1);

    REQUIRE(index.applyReset(4, packed({ 1, 3, 5 })));
    CHECK(index.state() == arena::ArenaAvailabilityIndex::State::Ready);
    CHECK(index.revision() == 4);
    CHECK(index.availability(hexHash(3)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(index.availability(hexHash(2)) ==
          arena::ArenaAvailabilityIndex::Availability::UnavailableToSome);
    CHECK(index.availability(hexHash(3).toUpper()) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(changes == 2);

    index.clear();
    CHECK(index.state() == arena::ArenaAvailabilityIndex::State::NotApplicable);
    CHECK(index.revision() == 0);
    CHECK(changes == 3);
}

TEST_CASE("ArenaAvailabilityIndex applies reset atomically",
          "[arena][ArenaAvailability]")
{
    arena::ArenaAvailabilityIndex index;
    REQUIRE(index.applyReset(7, packed({ 2, 4 })));
    int changes = 0;
    QObject::connect(
      &index, &arena::ArenaAvailabilityIndex::changed, [&] { ++changes; });

    CHECK_FALSE(index.applyReset(8, packed({ 4, 2 })));
    CHECK_FALSE(index.applyReset(8, packed({ 2, 2 })));
    CHECK_FALSE(index.applyReset(8, QByteArray(31, '\0')));
    CHECK_FALSE(index.applyReset(0, packed({ 1 })));
    CHECK_FALSE(index.applyReset(6, packed({ 1 })));
    CHECK(index.revision() == 7);
    CHECK(index.availability(hexHash(2)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(index.availability(hexHash(1)) ==
          arena::ArenaAvailabilityIndex::Availability::UnavailableToSome);
    CHECK(changes == 0);

    REQUIRE(index.applyReset(7, packed({ 1, 3 })));
    CHECK(index.revision() == 7);
    CHECK(index.availability(hexHash(1)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(changes == 1);
}

TEST_CASE("ArenaAvailabilityIndex validates and merges deltas atomically",
          "[arena][ArenaAvailability]")
{
    arena::ArenaAvailabilityIndex index;
    REQUIRE(index.applyReset(10, packed({ 1, 3, 5 })));
    int changes = 0;
    QObject::connect(
      &index, &arena::ArenaAvailabilityIndex::changed, [&] { ++changes; });

    REQUIRE(index.applyDelta(10, 11, packed({ 2, 6 }), packed({ 1, 5 })));
    CHECK(index.revision() == 11);
    CHECK(index.availability(hexHash(1)) ==
          arena::ArenaAvailabilityIndex::Availability::UnavailableToSome);
    CHECK(index.availability(hexHash(2)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(index.availability(hexHash(3)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(index.availability(hexHash(6)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(changes == 1);

    CHECK_FALSE(index.applyDelta(10, 12, {}, {}));
    CHECK_FALSE(index.applyDelta(11, 11, {}, {}));
    CHECK_FALSE(index.applyDelta(11, 12, packed({ 2 }), {}));
    CHECK_FALSE(index.applyDelta(11, 12, {}, packed({ 4 })));
    CHECK_FALSE(index.applyDelta(11, 12, {}, packed({ 1, 2, 3, 4 })));
    CHECK_FALSE(index.applyDelta(11, 12, packed({ 7, 7 }), {}));
    CHECK_FALSE(index.applyDelta(11, 12, packed({ 7 }), packed({ 7 })));
    CHECK(index.revision() == 11);
    CHECK(index.availability(hexHash(2)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(changes == 1);
}

TEST_CASE("ArenaAvailabilityIndex accepts 250000 hashes but not 250001",
          "[arena][ArenaAvailability]")
{
    arena::ArenaAvailabilityIndex index;
    REQUIRE(
      index.applyReset(1, packedSequence(arena::ArenaMaxInventoryHashes)));
    CHECK(index.availability(hexHash(arena::ArenaMaxInventoryHashes)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);

    CHECK_FALSE(
      index.applyReset(2, packedSequence(arena::ArenaMaxInventoryHashes + 1)));
    CHECK(index.revision() == 1);
    CHECK(index.availability(hexHash(arena::ArenaMaxInventoryHashes)) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
}
