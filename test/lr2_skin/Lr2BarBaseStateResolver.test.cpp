#include "Lr2BarBaseStateResolver.h"
#include "Lr2BarPositionMap.h"

#include <catch2/catch_test_macros.hpp>

#include <QVariantMap>

namespace {

QVariantMap
dstMap(int time, int x, int y)
{
    return QVariantMap{
        { QStringLiteral("time"), time }, { QStringLiteral("x"), x },
        { QStringLiteral("y"), y },       { QStringLiteral("w"), 100 },
        { QStringLiteral("h"), 20 },      { QStringLiteral("a"), 255 },
        { QStringLiteral("r"), 255 },     { QStringLiteral("g"), 255 },
        { QStringLiteral("b"), 255 },
    };
}

QVariantMap
dstMapWithAlpha(int time, int x, int y, int alpha)
{
    QVariantMap dst = dstMap(time, x, y);
    dst[QStringLiteral("a")] = alpha;
    return dst;
}

QVariantMap
rowWithDst(const QVariantMap& dst)
{
    return QVariantMap{
        { QStringLiteral("offDsts"), QVariantList{ dst } },
        { QStringLiteral("onDsts"), QVariantList{ dst } },
    };
}

} // namespace

TEST_CASE("LR2 bar base state resolver clamps non-looping single dst rows",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateResolver resolver;
    QVariantMap dst = dstMap(0, 10, 20);
    dst.insert(QStringLiteral("loop"), -1);

    int baseStateChanges = 0;
    QObject::connect(&resolver,
                     &Lr2BarBaseStateResolver::baseStatesChanged,
                     [&baseStateChanges]() { ++baseStateChanges; });

    resolver.setBarRows(QVariantList{ rowWithDst(dst) });

    REQUIRE(resolver.animationLimit() == 1);
    REQUIRE(baseStateChanges == 1);
    REQUIRE(resolver.stateCount() == 1);
    REQUIRE(resolver.stateValueAt(0).valid);
    REQUIRE(resolver.stateValueAt(0).x == 10);

    resolver.setSkinTime(20);
    REQUIRE(baseStateChanges == 2);
    REQUIRE(resolver.stateCount() == 1);
    REQUIRE_FALSE(resolver.stateValueAt(0).valid);

    resolver.setSkinTime(40);
    REQUIRE(baseStateChanges == 2);
}

TEST_CASE(
  "LR2 bar base state resolver keeps static single dst rows from ticking",
  "[lr2][runtime][select]")
{
    Lr2BarBaseStateResolver resolver;

    int baseStateChanges = 0;
    QObject::connect(&resolver,
                     &Lr2BarBaseStateResolver::baseStatesChanged,
                     [&baseStateChanges]() { ++baseStateChanges; });

    resolver.setBarRows(QVariantList{ rowWithDst(dstMap(0, 30, 40)) });

    REQUIRE(resolver.animationLimit() == 0);
    REQUIRE(baseStateChanges == 1);

    resolver.setSkinTime(1000);
    REQUIRE(baseStateChanges == 1);
    REQUIRE(resolver.stateCount() == 1);
    REQUIRE(resolver.stateValueAt(0).valid);
    REQUIRE(resolver.stateValueAt(0).x == 30);
}

TEST_CASE("LR2 bar position map reads resolver coordinates on attach",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateResolver resolver;
    resolver.setBarRows(QVariantList{
      rowWithDst(dstMap(0, 10, 20)),
      rowWithDst(dstMap(0, 30, 40)),
    });

    Lr2BarPositionMap positionMap;
    positionMap.setBaseStateResolver(&resolver);

    REQUIRE(positionMap.count() == 2);
    REQUIRE(positionMap.xAt(0) == 10.0);
    REQUIRE(positionMap.yAt(0) == 20.0);
    REQUIRE(positionMap.xAt(1) == 30.0);
    REQUIRE(positionMap.yAt(1) == 40.0);
}

TEST_CASE("LR2 bar base state resolver uses fast scroll for uniform rows",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateResolver resolver;
    resolver.setSelectedRow(1);
    resolver.setBarRows(QVariantList{
      rowWithDst(dstMap(0, 10, 20)),
      rowWithDst(dstMap(0, 10, 40)),
      rowWithDst(dstMap(0, 10, 60)),
    });

    REQUIRE(resolver.fastScrollActive());
    REQUIRE(resolver.fastScrollDx() == 0.0);
    REQUIRE(resolver.fastScrollDy() == 20.0);
}

TEST_CASE("LR2 bar base state resolver disables fast scroll when selected row "
          "state differs",
          "[lr2][runtime][select]")
{
    Lr2BarBaseStateResolver resolver;
    resolver.setSelectedRow(1);
    resolver.setBarRows(QVariantList{
      rowWithDst(dstMapWithAlpha(0, 10, 20, 128)),
      rowWithDst(dstMapWithAlpha(0, 10, 40, 255)),
      rowWithDst(dstMapWithAlpha(0, 10, 60, 128)),
    });

    REQUIRE_FALSE(resolver.fastScrollActive());
}
