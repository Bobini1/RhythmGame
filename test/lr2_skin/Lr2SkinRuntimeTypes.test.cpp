#include "Lr2SkinRuntimeTypes.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QVariantMap>

using namespace lr2skin::runtime;
using Catch::Matchers::WithinAbs;

namespace {

QVariantMap dstMap(int time, int x, int y, int w, int h) {
    return QVariantMap {
        {QStringLiteral("time"), time},
        {QStringLiteral("x"), x},
        {QStringLiteral("y"), y},
        {QStringLiteral("w"), w},
        {QStringLiteral("h"), h},
        {QStringLiteral("a"), 255},
        {QStringLiteral("r"), 255},
        {QStringLiteral("g"), 255},
        {QStringLiteral("b"), 255},
    };
}

} // namespace

TEST_CASE("LR2 runtime dst interpolation respects active options", "[lr2][runtime]") {
    QVariantMap first = dstMap(0, 0, 0, 10, 10);
    first.insert(QStringLiteral("op1"), 7);
    QVariantMap second = dstMap(1000, 100, 40, 10, 10);

    const QVector<Dst> dsts = readDsts(QVariantList {first, second});

    REQUIRE(analyzeDsts(dsts).usesActiveOptions);
    REQUIRE_FALSE(currentState(dsts, 500, 0, activeOptionSet(QVariantList {})).valid);

    const State state = currentState(dsts, 500, 0, activeOptionSet(QVariantList {7}));
    REQUIRE(state.valid);
    REQUIRE_THAT(state.x, WithinAbs(50.0, 0.0001));
    REQUIRE_THAT(state.y, WithinAbs(20.0, 0.0001));
}

TEST_CASE("LR2 runtime dst active options require every positive gate", "[lr2][runtime]") {
    Dst dst;
    dst.op1 = 2;
    dst.op2 = 160;

    REQUIRE(activeOptionsForDsts(dst, QVariantList {2}).isEmpty());
    REQUIRE(activeOptionsForDsts(dst, QVariantList {160}).isEmpty());

    const QVariantList active = activeOptionsForDsts(dst, QVariantList {2, 160});
    REQUIRE(active.size() == 2);
    REQUIRE(active.at(0).toInt() == 2);
    REQUIRE(active.at(1).toInt() == 160);
}

TEST_CASE("LR2 runtime dst active options respect negative gates", "[lr2][runtime]") {
    Dst dst;
    dst.op1 = 2;
    dst.op2 = -160;

    const QVariantList active = activeOptionsForDsts(dst, QVariantList {2});
    REQUIRE(active.size() == 1);
    REQUIRE(active.at(0).toInt() == 2);
    REQUIRE(activeOptionsForDsts(dst, QVariantList {2, 160}).isEmpty());
}

TEST_CASE("LR2 runtime dst loops wrap inside the loop segment", "[lr2][runtime]") {
    QVariantMap first = dstMap(0, 0, 0, 10, 10);
    first.insert(QStringLiteral("loop"), 100);
    const QVariantMap middle = dstMap(100, 100, 0, 10, 10);
    const QVariantMap last = dstMap(300, 300, 0, 10, 10);

    const QVector<Dst> dsts = readDsts(QVariantList {first, middle, last});

    REQUIRE(analyzeDsts(dsts).loopsContinuously);
    const State state = currentState(dsts, 450, 0, {});
    REQUIRE(state.valid);
    REQUIRE_THAT(state.x, WithinAbs(250.0, 0.0001));
}

TEST_CASE("LR2 runtime classifies slider sources and track geometry", "[lr2][runtime]") {
    Source source;
    source.valid = true;
    source.slider = true;
    source.sliderType = 1;
    source.sliderRange = 100;
    source.sliderDirection = 3;
    source.sliderDisabled = 0;

    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source)
            == SelectScrollSpriteStateOverride);

    State base;
    base.valid = true;
    base.x = 200;
    base.y = 10;
    base.w = 20;
    base.h = 40;

    const State track = sliderTrackState(source, base);
    REQUIRE(track.valid);
    REQUIRE(track.x == 100.0);
    REQUIRE(track.w == 120.0);
    REQUIRE_THAT(sliderPositionFromPointer(source, track, 100, 20), WithinAbs(1.0, 0.0001));
    REQUIRE_THAT(sliderPositionFromPointer(source, track, 210, 20), WithinAbs(0.0, 0.0001));
}

TEST_CASE("LR2 runtime sprite override ids match the QML contract", "[lr2][runtime]") {
    Source source;
    source.valid = true;
    source.slider = true;
    source.sliderRange = 100;

    source.sliderType = 1;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source) == 1);

    source.sliderType = 6;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), true, source) == 2);

    source.sliderType = 4;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), true, source) == 3);

    source.sliderType = 8;
    source.sliderRefNumber = true;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), false, source) == 4);

    source.sliderRefNumber = false;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source) == 5);
}

TEST_CASE("LR2 runtime hit testing accepts negative dst sizes", "[lr2][runtime]") {
    const QVariantMap rect {
        {QStringLiteral("x"), 100},
        {QStringLiteral("y"), 80},
        {QStringLiteral("w"), -40},
        {QStringLiteral("h"), -30},
    };

    REQUIRE(rectContains(rect, 75, 65));
    REQUIRE(rectContains(rect, 100, 80));
    REQUIRE_FALSE(rectContains(rect, 59, 65));
    REQUIRE_FALSE(rectContains(rect, 75, 49));
}

TEST_CASE("LR2 runtime note sort ignores sparse empty note lanes",
          "[lr2][runtime]") {
    QVariantMap lane0 = dstMap(0, 0, 0, 10, 10);
    lane0.insert(QStringLiteral("sortId"), 120);
    QVariantMap lane10 = dstMap(0, 0, 0, 10, 10);
    lane10.insert(QStringLiteral("sortId"), 210);

    QVariantList noteDsts;
    noteDsts.append(QVariant::fromValue(QVariantList{ lane0 }));
    noteDsts.append(QVariant::fromValue(QVariantList{}));
    noteDsts.append(QVariant::fromValue(QVariantList{}));
    noteDsts.append(QVariant::fromValue(QVariantList{ lane10 }));

    CHECK(staticNoteElementSortId(noteDsts) == 120);
    CHECK(staticNoteElementSortId(QVariantList {
              QVariant::fromValue(QVariantList {}),
              QVariant::fromValue(QVariantList {})
          })
          == 0);
}
