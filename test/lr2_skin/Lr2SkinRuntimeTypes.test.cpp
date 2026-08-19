#include "Lr2SkinRuntimeTypes.h"
#include "Lr2AnimationFrameState.h"
#include "Lr2BarPositionedItem.h"
#include "Lr2BlendSprite.h"
#include "Lr2GameplayJudgeState.h"
#include "Lr2SkinElementActiveOptionsState.h"
#include "Lr2SkinElementNumberState.h"
#include "Lr2SkinTimerState.h"
#include "Lr2TimelineFrameState.h"
#include "Lr2TimelineState.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <QVariantMap>

using namespace lr2skin::runtime;
using Catch::Matchers::WithinAbs;

TEST_CASE("LR2 animation frame state validates the complete source region",
          "[lr2][animation]")
{
    Lr2AnimationFrameState state;
    state.setTextureWidth(100);
    state.setTextureHeight(80);
    CHECK_THAT(state.textureSize().x(), WithinAbs(100.0, 0.0001));
    CHECK_THAT(state.textureSize().y(), WithinAbs(80.0, 0.0001));

    QVariantMap source{
        { QStringLiteral("x"), 10 },
        { QStringLiteral("y"), 20 },
        { QStringLiteral("w"), 80 },
        { QStringLiteral("h"), 60 },
        { QStringLiteral("div_x"), 4 },
        { QStringLiteral("div_y"), 3 },
    };
    state.setSourceData(source);
    CHECK_FALSE(state.sourceRegionExceedsTextureBounds());
    CHECK_THAT(state.effectiveSourceClipRect().height(), WithinAbs(20.0, 0.0001));

    state.setSourceHeightRatio(0.25);
    CHECK_THAT(state.effectiveSourceClipRect().height(), WithinAbs(5.0, 0.0001));
    CHECK_THAT(state.effectiveSourceRect().w(), WithinAbs(0.0625, 0.0001));

    source.insert(QStringLiteral("w"), 91);
    state.setSourceData(source);
    CHECK(state.sourceRegionExceedsTextureBounds());

    state.setTextureWidth(101);
    CHECK_FALSE(state.sourceRegionExceedsTextureBounds());
}

TEST_CASE("LR2 gameplay judge state records primitive hit data",
          "[lr2][gameplay]")
{
    Lr2GameplayJudgeState state;

    state.record(1, 5, -1'400'000, 0);
    state.record(1, 4, 2'600'000, 10);
    state.record(2, 3, -3'600'000, 11);

    CHECK(state.timingNumber(410, 1) == 1);
    CHECK(state.timingNumber(413, 1) == 1);
    CHECK(state.timingNumber(414, 2) == 1);
    CHECK(state.timingNumber(423, 1) == 1);
    CHECK(state.timingNumber(424, 1) == 1);
    CHECK(state.lastJudgeTiming1() == -3);
    CHECK(state.lastJudgeTiming2() == 4);
    CHECK(state.judgeNowValue1() == 2);
    CHECK(state.judgeNowValue2() == 3);
    CHECK(state.judgeValueForId(500) == 1);
    CHECK(state.judgeValueForId(1510) == 3);
    CHECK(state.judgeValueForId(1611) == 4);
    CHECK_THAT(state.timingMean(1), WithinAbs(-1.0, 0.0001));
    CHECK_THAT(state.timingStdDev(1), WithinAbs(2.0, 0.0001));

    state.reset();

    CHECK(state.timingNumber(410, 1) == 0);
    CHECK(state.lastJudgeTiming1() == 0);
    CHECK(state.judgeNowValue2() == 0);
    CHECK(state.judgeValueForId(1611) == 0);
}

TEST_CASE("LR2 custom scene-graph blending covers destination-dependent modes",
          "[lr2][skin][blend]")
{
    CHECK_FALSE(Lr2BlendSprite::supportsBlendMode(0));
    CHECK_FALSE(Lr2BlendSprite::supportsBlendMode(1));
    CHECK(Lr2BlendSprite::supportsBlendMode(2));
    CHECK(Lr2BlendSprite::supportsBlendMode(3));
    CHECK(Lr2BlendSprite::supportsBlendMode(4));
    CHECK(Lr2BlendSprite::supportsBlendMode(5));
    CHECK_FALSE(Lr2BlendSprite::supportsBlendMode(6));
    CHECK(Lr2BlendSprite::supportsBlendMode(9));
    CHECK_FALSE(Lr2BlendSprite::supportsBlendMode(10));
}

namespace {

QVariantMap
dstMap(int time, int x, int y, int w, int h)
{
    return QVariantMap{
        { QStringLiteral("time"), time }, { QStringLiteral("x"), x },
        { QStringLiteral("y"), y },       { QStringLiteral("w"), w },
        { QStringLiteral("h"), h },       { QStringLiteral("a"), 255 },
        { QStringLiteral("r"), 255 },     { QStringLiteral("g"), 255 },
        { QStringLiteral("b"), 255 },
    };
}

} // namespace

TEST_CASE("LR2 runtime dst interpolation respects active options",
          "[lr2][runtime]")
{
    QVariantMap first = dstMap(0, 0, 0, 10, 10);
    first.insert(QStringLiteral("op1"), 7);
    QVariantMap second = dstMap(1000, 100, 40, 10, 10);

    const QVector<Dst> dsts = readDsts(QVariantList{ first, second });

    REQUIRE(analyzeDsts(dsts).usesActiveOptions);
    REQUIRE_FALSE(
      currentState(dsts, 500, 0, activeOptionSet(QVariantList{})).valid);

    const State state =
      currentState(dsts, 500, 0, activeOptionSet(QVariantList{ 7 }));
    REQUIRE(state.valid);
    REQUIRE_THAT(state.x, WithinAbs(50.0, 0.0001));
    REQUIRE_THAT(state.y, WithinAbs(20.0, 0.0001));
}

TEST_CASE("LR2 runtime dst active options require every positive gate",
          "[lr2][runtime]")
{
    Dst dst;
    dst.op1 = 2;
    dst.op2 = 160;

    REQUIRE(activeOptionsForDsts(dst, QVariantList{ 2 }).isEmpty());
    REQUIRE(activeOptionsForDsts(dst, QVariantList{ 160 }).isEmpty());

    const QVariantList active =
      activeOptionsForDsts(dst, QVariantList{ 2, 160 });
    REQUIRE(active.size() == 2);
    REQUIRE(active.at(0).toInt() == 2);
    REQUIRE(active.at(1).toInt() == 160);
}

TEST_CASE("LR2 runtime dst active options respect negative gates",
          "[lr2][runtime]")
{
    Dst dst;
    dst.op1 = 2;
    dst.op2 = -160;

    const QVariantList active = activeOptionsForDsts(dst, QVariantList{ 2 });
    REQUIRE(active.size() == 1);
    REQUIRE(active.at(0).toInt() == 2);
    REQUIRE(activeOptionsForDsts(dst, QVariantList{ 2, 160 }).isEmpty());
}

TEST_CASE("LR2 timeline state keeps inactive negative-only gates hidden",
          "[lr2][runtime]")
{
    QVariantMap first = dstMap(0, 0, 0, 10, 10);
    first.insert(QStringLiteral("op1"), -5);

    Lr2SkinElementActiveOptionsState activeOptionsState;
    Lr2TimelineState timelineState;
    timelineState.setDsts(QVariantList{ first });
    timelineState.setActiveOptionsState(&activeOptionsState);

    activeOptionsState.setActiveOptions(QVariantList{}, false);
    REQUIRE_FALSE(timelineState.hasState());

    activeOptionsState.setActiveOptions(QVariantList{}, true);
    REQUIRE(timelineState.hasState());
}

TEST_CASE("LR2 timeline frame ignores invalid override states",
          "[lr2][runtime]")
{
    Lr2TimelineFrameState frame;
    frame.setDsts(QVariantList{ dstMap(0, 12, 34, 56, 78) });
    frame.setTimerFire(0);

    REQUIRE(frame.hasState());
    REQUIRE(frame.x() == 12);

    frame.setStateOverrideEnabled(true);
    frame.setStateOverrideValue({});

    REQUIRE_FALSE(frame.hasState());
    REQUIRE_FALSE(frame.hasDirectState());
    REQUIRE(frame.a() == 0);

    Lr2TimelineStateValue validOverride;
    validOverride.valid = true;
    validOverride.x = 90;
    validOverride.y = 12;
    validOverride.w = 34;
    validOverride.h = 56;
    frame.setStateOverrideValue(validOverride);

    REQUIRE(frame.hasState());
    REQUIRE(frame.hasDirectState());
    REQUIRE(frame.x() == 90);
    REQUIRE(frame.y() == 12);
}

TEST_CASE("LR2 timeline frame treats blend 0 alpha 0 as opaque no-blend",
          "[lr2][runtime]")
{
    QVariantMap noBlend = dstMap(0, 12, 34, 56, 78);
    noBlend.insert(QStringLiteral("a"), 0);
    noBlend.insert(QStringLiteral("blend"), 0);

    Lr2TimelineFrameState noBlendFrame;
    noBlendFrame.setDsts(QVariantList{ noBlend });
    noBlendFrame.setTimerFire(0);

    REQUIRE(noBlendFrame.hasState());
    REQUIRE(noBlendFrame.isRenderable());
    REQUIRE(noBlendFrame.a() == 0);
    REQUIRE_THAT(noBlendFrame.opacity(), WithinAbs(1.0, 0.0001));

    QVariantMap alphaBlend = dstMap(0, 12, 34, 56, 78);
    alphaBlend.insert(QStringLiteral("a"), 0);
    alphaBlend.insert(QStringLiteral("blend"), 1);

    Lr2TimelineFrameState alphaBlendFrame;
    alphaBlendFrame.setDsts(QVariantList{ alphaBlend });
    alphaBlendFrame.setTimerFire(0);

    REQUIRE(alphaBlendFrame.hasState());
    REQUIRE_FALSE(alphaBlendFrame.isRenderable());
    REQUIRE(alphaBlendFrame.a() == 0);
    REQUIRE_THAT(alphaBlendFrame.opacity(), WithinAbs(0.0, 0.0001));
}

TEST_CASE("LR2 timeline frame preserves supported destination blend modes",
          "[lr2][runtime][blend]")
{
    for (const int blendMode : { 3, 4, 5, 9 }) {
        QVariantMap dst = dstMap(0, 12, 34, 56, 78);
        dst.insert(QStringLiteral("blend"), blendMode);

        Lr2TimelineFrameState frame;
        frame.setDsts(QVariantList{ dst });
        frame.setTimerFire(0);

        CAPTURE(blendMode);
        REQUIRE(frame.rawBlendMode() == blendMode);
        REQUIRE(frame.blendMode() == blendMode);
    }
}

TEST_CASE("LR2 bar positioned item notifies usePositionMap changes",
          "[lr2][runtime]")
{
    Lr2BarPositionedItem item;
    int changed = 0;
    QObject::connect(&item,
                     &Lr2BarPositionedItem::usePositionMapChanged,
                     [&changed]() { ++changed; });

    item.setUsePositionMap(false);

    REQUIRE_FALSE(item.usePositionMap());
    REQUIRE(changed == 1);
}

TEST_CASE("LR2 runtime dst loops wrap inside the loop segment",
          "[lr2][runtime]")
{
    QVariantMap first = dstMap(0, 0, 0, 10, 10);
    first.insert(QStringLiteral("loop"), 100);
    const QVariantMap middle = dstMap(100, 100, 0, 10, 10);
    const QVariantMap last = dstMap(300, 300, 0, 10, 10);

    const QVector<Dst> dsts = readDsts(QVariantList{ first, middle, last });

    REQUIRE(analyzeDsts(dsts).loopsContinuously);
    const State state = currentState(dsts, 450, 0, {});
    REQUIRE(state.valid);
    REQUIRE_THAT(state.x, WithinAbs(250.0, 0.0001));
}

TEST_CASE("LR2 runtime classifies slider sources and track geometry",
          "[lr2][runtime]")
{
    Source source;
    source.valid = true;
    source.slider = true;
    source.sliderType = 1;
    source.sliderRange = 100;
    source.sliderDirection = 3;
    source.sliderDisabled = 0;

    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source) ==
            SelectScrollSpriteStateOverride);

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
    REQUIRE_THAT(sliderPositionFromPointer(source, track, 100, 20),
                 WithinAbs(1.0, 0.0001));
    REQUIRE_THAT(sliderPositionFromPointer(source, track, 210, 20),
                 WithinAbs(0.0, 0.0001));
}

TEST_CASE("LR2 runtime sprite override ids match the QML contract",
          "[lr2][runtime]")
{
    Source source;
    source.valid = true;
    source.slider = true;
    source.sliderRange = 100;

    source.sliderType = 1;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source) ==
            1);

    source.sliderType = 6;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), true, source) == 2);

    source.sliderType = 4;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), true, source) == 3);

    source.sliderType = 8;
    source.sliderRefNumber = true;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("play"), false, source) ==
            4);

    source.sliderRefNumber = false;
    REQUIRE(spriteStateOverrideKind(QStringLiteral("select"), false, source) ==
            5);
}

TEST_CASE("LR2 runtime hit testing accepts negative dst sizes",
          "[lr2][runtime]")
{
    const QVariantMap rect{
        { QStringLiteral("x"), 100 },
        { QStringLiteral("y"), 80 },
        { QStringLiteral("w"), -40 },
        { QStringLiteral("h"), -30 },
    };

    REQUIRE(rectContains(rect, 75, 65));
    REQUIRE(rectContains(rect, 100, 80));
    REQUIRE_FALSE(rectContains(rect, 59, 65));
    REQUIRE_FALSE(rectContains(rect, 75, 49));
}

TEST_CASE("LR2 timer state exposes decide start-input timer", "[lr2][runtime]")
{
    Lr2SkinTimerState timerState;
    timerState.setScreenKey(QStringLiteral("decide"));
    timerState.setStartInput(300);
    timerState.setRenderSkinTime(250);
    timerState.setAcceptsInput(false);

    CHECK(timerState.skinTimerCanFire(1));
    CHECK_FALSE(timerState.skinTimerCanFire(2));
    CHECK(timerState.skinTimerFireTime(1) == -1);

    timerState.setRenderSkinTime(450);
    timerState.setAcceptsInput(true);

    CHECK(timerState.skinTimerFireTime(1) == 300);
}

TEST_CASE("LR2 runtime note sort ignores sparse empty note lanes",
          "[lr2][runtime]")
{
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
    CHECK(staticNoteElementSortId(
            QVariantList{ QVariant::fromValue(QVariantList{}),
                          QVariant::fromValue(QVariantList{}) }) == 0);
}

TEST_CASE("LR2 gameplay numbers keep their targeted dependency masks",
          "[lr2][runtime][number]")
{
    Source source;
    source.valid = true;

    const auto maskFor = [&source](int number) {
        source.num = number;
        return gameplayNumberDependencyMask(source);
    };

    CHECK(maskFor(20) == 128);
    CHECK(maskFor(11) == 10);
    CHECK(maskFor(430) == 15);
    CHECK(maskFor(423) == 133);
    CHECK(maskFor(201) == 5);
    CHECK(maskFor(370) == 10);
    CHECK(maskFor(42) == 32);
    CHECK(maskFor(0) == 67);

    source.nowCombo = true;
    source.side = 1;
    CHECK(gameplayNumberDependencyMask(source) == 4);
    source.side = 2;
    CHECK(gameplayNumberDependencyMask(source) == 8);
}

TEST_CASE("LR2 gameplay hit history is retained only for live chart elements",
          "[lr2][runtime][chart]")
{
    CHECK(elementTypeUsesGameplayHitEvents(10));
    CHECK(elementTypeUsesGameplayHitEvents(11));
    CHECK(elementTypeUsesGameplayHitEvents(14));
    CHECK(elementTypeUsesGameplayHitEvents(15));
    CHECK_FALSE(elementTypeUsesGameplayHitEvents(0));
    CHECK_FALSE(elementTypeUsesGameplayHitEvents(1));
    CHECK_FALSE(elementTypeUsesGameplayHitEvents(12));
}

TEST_CASE("LR2 element number state notifies only on real state changes",
          "[lr2][runtime][number]")
{
    Lr2SkinElementNumberState state;
    int dependencyChanges = 0;
    int revisionChanges = 0;
    QObject::connect(&state,
                     &Lr2SkinElementNumberState::dependencyMaskChanged,
                     [&dependencyChanges]() { ++dependencyChanges; });
    QObject::connect(&state,
                     &Lr2SkinElementNumberState::revisionChanged,
                     [&revisionChanges]() { ++revisionChanges; });

    state.setDependencyMask(5);
    state.setDependencyMask(5);
    CHECK(state.dependencyMask() == 5);
    CHECK(dependencyChanges == 1);

    state.refresh();
    state.refresh();
    CHECK(state.revision() == 2);
    CHECK(revisionChanges == 2);
}
