#include "Lr2SelectItemModel.h"
#include "Lr2SelectBarModel.h"
#include "Lr2SelectBarCell.h"
#include "arena/ArenaAvailabilityIndex.h"

#include <catch2/catch_test_macros.hpp>

#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace {

auto
packedHash(const QByteArray& hex) -> QByteArray
{
    return QByteArray::fromHex(hex);
}

auto
chart(const QByteArray& sha256) -> QVariantMap
{
    return {
        { QStringLiteral("key"), QString::fromLatin1(sha256) },
        { QStringLiteral("type"), QStringLiteral("chart") },
        { QStringLiteral("title"), QStringLiteral("Chart") },
        { QStringLiteral("sha256"), QString::fromLatin1(sha256) },
    };
}

auto
folder() -> QVariantMap
{
    return {
        { QStringLiteral("key"), QStringLiteral("folder") },
        { QStringLiteral("type"), QStringLiteral("folder") },
        { QStringLiteral("title"), QStringLiteral("Folder") },
    };
}

} // namespace

TEST_CASE("LR2 select availability follows the atomic Arena index",
          "[arena][ArenaSelect][lr2]")
{
    const auto hash1 = QByteArray(64, '1');
    const auto hash2 = QByteArray(64, '2');
    auto availability = arena::ArenaAvailabilityIndex{};
    auto model = Lr2SelectItemModel{};
    model.setItems({ chart(hash1), folder() });

    QVector<QVector<int>> changes;
    QObject::connect(
      &model,
      &QAbstractItemModel::dataChanged,
      [&](const QModelIndex&, const QModelIndex&, const QVector<int>& roles) {
          changes.push_back(roles);
      });

    model.setArenaAvailability(&availability);
    REQUIRE(model.roleNames().value(
              Lr2SelectItemModel::ArenaAvailabilityRole) ==
            QByteArrayLiteral("arenaAvailability"));
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::NotApplicable));

    REQUIRE(availability.applyReset(1, packedHash(hash1)));
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::AvailableToAll));
    CHECK(model.data(model.index(1, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::NotApplicable));

    REQUIRE(availability.applyReset(2, packedHash(hash2)));
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::UnavailableToSome));

    availability.setSyncing();
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::Syncing));

    availability.clear();
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::NotApplicable));

    REQUIRE(changes.size() >= 5);
    for (const auto& roles : changes) {
        CHECK(roles ==
              QVector<int>{ Lr2SelectItemModel::DisplayTextRole,
                            Lr2SelectItemModel::TitleTypeRole,
                            Lr2SelectItemModel::BodyTypeRole,
                            Lr2SelectItemModel::TitleRole,
                            Lr2SelectItemModel::ArenaAvailabilityRole });
    }
}

TEST_CASE("LR2 select availability detaches cleanly from a replaced index",
          "[arena][ArenaSelect][lr2]")
{
    const auto hash = QByteArray(64, 'a');
    auto first = arena::ArenaAvailabilityIndex{};
    auto second = arena::ArenaAvailabilityIndex{};
    auto model = Lr2SelectItemModel{};
    model.setItems({ chart(hash) });
    model.setArenaAvailability(&first);
    model.setArenaAvailability(&second);

    REQUIRE(first.applyReset(1, packedHash(hash)));
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::NotApplicable));

    REQUIRE(second.applyReset(1, packedHash(hash)));
    CHECK(model.data(model.index(0, 0),
                     Lr2SelectItemModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::AvailableToAll));
}

TEST_CASE("LR2 select bars expose Arena availability to skin delegates",
          "[arena][ArenaSelect][lr2]")
{
    const auto hash = QByteArray(64, 'b');
    auto availability = arena::ArenaAvailabilityIndex{};
    auto source = Lr2SelectItemModel{};
    source.setItems({ chart(hash) });
    source.setArenaAvailability(&availability);

    auto bars = Lr2SelectBarModel{};
    bars.setSourceModel(&source);
    bars.setLogicalCount(1);
    bars.setRowCountLimit(1);

    REQUIRE(bars.roleNames().value(
              Lr2SelectBarModel::ArenaAvailabilityRole) ==
            QByteArrayLiteral("arenaAvailability"));
    CHECK(bars.data(bars.index(0, 0),
                    Lr2SelectBarModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::NotApplicable));

    REQUIRE(availability.applyReset(1, packedHash(hash)));
    CHECK(bars.data(bars.index(0, 0),
                    Lr2SelectBarModel::ArenaAvailabilityRole)
            .toInt() ==
          static_cast<int>(
            arena::ArenaAvailabilityIndex::Availability::AvailableToAll));
}

TEST_CASE("LR2 and Beatoraja decorate only locally present uncommon charts",
          "[arena][ArenaSelect][lr2]")
{
    const auto commonHash = QByteArray(64, 'a');
    const auto uncommonHash = QByteArray(64, 'b');
    auto availability = arena::ArenaAvailabilityIndex{};
    REQUIRE(availability.applyReset(1, packedHash(commonHash)));

    SECTION("LR2 prefixes uncommon charts and preserves missing table entries")
    {
        auto missingEntry = QVariantMap{
            { QStringLiteral("key"), QStringLiteral("missing") },
            { QStringLiteral("type"), QStringLiteral("entry") },
            { QStringLiteral("title"), QStringLiteral("Missing chart") },
        };
        auto model = Lr2SelectItemModel{};
        model.setArenaUnavailablePrefix(
          QStringLiteral("(arena unavailable) "));
        model.setItems({ chart(uncommonHash), missingEntry });
        model.setArenaAvailability(&availability);

        CHECK(model.data(model.index(0, 0),
                         Lr2SelectItemModel::DisplayTextRole)
                .toString() == QStringLiteral("(arena unavailable) Chart"));
        CHECK(model.data(model.index(0, 0), Lr2SelectItemModel::TitleRole)
                .toString() == QStringLiteral("(arena unavailable) Chart"));
        CHECK(model.data(model.index(1, 0),
                         Lr2SelectItemModel::DisplayTextRole)
                .toString() == QStringLiteral("(missing) Missing chart"));

        auto cell = Lr2SelectBarCell{};
        REQUIRE(model.populateBarCell(0, 0, &cell));
        CHECK(cell.text() == QStringLiteral("(arena unavailable) Chart"));
    }

    SECTION("Beatoraja uses unavailable body and title types without a prefix")
    {
        auto model = Lr2SelectItemModel{};
        model.setUseBeatorajaBarTextTypes(true);
        model.setBarTitleTypes({ 2, 8 });
        model.setBarBodyTypes({ 0, 4 });
        model.setArenaUnavailablePrefix(
          QStringLiteral("(arena unavailable) "));
        model.setItems({ chart(uncommonHash) });
        model.setArenaAvailability(&availability);

        CHECK(model.data(model.index(0, 0),
                         Lr2SelectItemModel::DisplayTextRole)
                .toString() == QStringLiteral("Chart"));
        CHECK(model.data(model.index(0, 0),
                         Lr2SelectItemModel::TitleTypeRole)
                .toInt() == 8);
        CHECK(model.data(model.index(0, 0),
                         Lr2SelectItemModel::BodyTypeRole)
                .toInt() == 4);

        auto cell = Lr2SelectBarCell{};
        REQUIRE(model.populateBarCell(0, 0, &cell));
        CHECK(cell.text() == QStringLiteral("Chart"));
        CHECK(cell.titleType() == 8);
        CHECK(cell.bodyType() == 4);
    }
}
