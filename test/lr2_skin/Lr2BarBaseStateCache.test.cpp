#include "Lr2BarBaseStateCache.h"

#include <catch2/catch_test_macros.hpp>

#include <QVariantMap>

namespace {

QVariantMap dstMap(int time, int x, int y) {
    return QVariantMap {
        {QStringLiteral("time"), time},
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("w"), 100},
        {QStringLiteral("h"), 20},
        {QStringLiteral("a"), 255},
        {QStringLiteral("r"), 255},
        {QStringLiteral("g"), 255},
        {QStringLiteral("b"), 255},
    };
}

QVariantMap rowWithDst(const QVariantMap& dst) {
    return QVariantMap {
        {QStringLiteral("offDsts"), QVariantList {dst}},
        {QStringLiteral("onDsts"), QVariantList {dst}},
    };
}

} // namespace

TEST_CASE("LR2 bar base state cache clamps non-looping single dst rows",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateCache cache;
    QVariantMap dst = dstMap(0, 10, 20);
    dst.insert(QStringLiteral("loop"), -1);

    int baseStateChanges = 0;
    QObject::connect(&cache,
                     &Lr2BarBaseStateCache::baseStatesChanged,
                     [&baseStateChanges]() { ++baseStateChanges; });

    cache.setBarRows(QVariantList {rowWithDst(dst)});

    REQUIRE(cache.animationLimit() == 1);
    REQUIRE(baseStateChanges == 1);
    REQUIRE(cache.baseStates().size() == 1);
    REQUIRE(cache.baseStates().at(0).toMap().value(QStringLiteral("x")).toInt() == 10);

    cache.setSkinTime(20);
    REQUIRE(baseStateChanges == 2);
    REQUIRE(cache.baseStates().size() == 1);
    REQUIRE_FALSE(cache.baseStates().at(0).isValid());

    cache.setSkinTime(40);
    REQUIRE(baseStateChanges == 2);
}

TEST_CASE("LR2 bar base state cache keeps static single dst rows from ticking",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateCache cache;

    int baseStateChanges = 0;
    QObject::connect(&cache,
                     &Lr2BarBaseStateCache::baseStatesChanged,
                     [&baseStateChanges]() { ++baseStateChanges; });

    cache.setBarRows(QVariantList {rowWithDst(dstMap(0, 30, 40))});

    REQUIRE(cache.animationLimit() == 0);
    REQUIRE(baseStateChanges == 1);

    cache.setSkinTime(1000);
    REQUIRE(baseStateChanges == 1);
    REQUIRE(cache.baseStates().size() == 1);
    REQUIRE(cache.baseStates().at(0).toMap().value(QStringLiteral("x")).toInt() == 30);
}
