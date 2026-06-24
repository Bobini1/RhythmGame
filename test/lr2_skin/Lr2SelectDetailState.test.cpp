#include "Lr2SelectDetailState.h"

#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsScore.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QString>
#include <QVariantList>

#include <memory>

namespace {

void
ensureCoreApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    static std::unique_ptr<QCoreApplication> app;
    if (!QCoreApplication::instance()) {
        app = std::make_unique<QCoreApplication>(argc, argv);
    }
}

gameplay_logic::BmsScore*
scoreWithValues(const QString& clearType,
                double maxPoints,
                double points,
                QList<int> judgementCounts,
                int maxCombo,
                QObject* parent)
{
    const QString guid =
      QStringLiteral("test-%1-%2-%3").arg(clearType).arg(maxPoints).arg(points);
    auto result = std::make_unique<gameplay_logic::BmsResult>(
      maxPoints,
      0,
      0,
      0,
      0,
      0,
      0,
      clearType,
      judgementCounts,
      0,
      points,
      maxCombo,
      0,
      0,
      QList<qint64>{},
      0,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      gameplay_logic::ChartData::Keymode::K7,
      guid,
      QStringLiteral("sha256-%1").arg(guid),
      QStringLiteral("md5-%1").arg(guid));
    return new gameplay_logic::BmsScore(
      std::move(result),
      std::make_unique<gameplay_logic::BmsReplayData>(
        QList<gameplay_logic::HitEvent>{}, guid),
      std::make_unique<gameplay_logic::BmsGaugeHistory>(
        QList<gameplay_logic::BmsGaugeInfo>{}, guid),
      parent);
}

gameplay_logic::BmsScore*
scoreWithClearType(const QString& clearType, QObject* parent)
{
    return scoreWithValues(
      clearType, 100.0, 0.0, QList<int>{ 0, 0, 0, 0, 0, 0 }, 0, parent);
}

void
refreshState(Lr2SelectDetailState& state,
             const QString& itemKey,
             const QVariantList& scores,
             int selectedDifficulty,
             const QVariantList& difficultyCounts,
             const QVariantList& difficultyLevels,
             const QVariantList& difficultyLamps,
             bool useBeatorajaSemantics,
             bool buildScoreOptionIds)
{
    state.refreshSelectedFromVariantIdentityForIdentifier(
      itemKey,
      QString{},
      false,
      0,
      0,
      QVariant{},
      QVariant{},
      scores,
      itemKey,
      selectedDifficulty,
      difficultyCounts,
      difficultyLevels,
      difficultyLamps,
      useBeatorajaSemantics,
      buildScoreOptionIds,
      true,
      !difficultyLamps.isEmpty());
}

int
summaryLampFor(const QString& clearType, bool useBeatorajaSemantics)
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithClearType(clearType, &state);
    refreshState(
      state,
      QStringLiteral("key-%1-%2").arg(clearType).arg(useBeatorajaSemantics),
      QVariantList{ QVariant::fromValue(score) },
      0,
      QVariantList{},
      QVariantList{},
      QVariantList{},
      useBeatorajaSemantics,
      true);
    return state.summary()->lamp();
}

} // namespace

TEST_CASE("LR2 select detail state uses exact Beatoraja clear lamp ids",
          "[lr2][runtime][select]")
{
    CHECK(summaryLampFor(QStringLiteral("NOPLAY"), true) == 0);
    CHECK(summaryLampFor(QStringLiteral("FAILED"), true) == 1);
    CHECK(summaryLampFor(QStringLiteral("AEASY"), true) == 2);
    CHECK(summaryLampFor(QStringLiteral("LIGHTASSIST"), true) == 3);
    CHECK(summaryLampFor(QStringLiteral("EASY"), true) == 4);
    CHECK(summaryLampFor(QStringLiteral("NORMAL"), true) == 5);
    CHECK(summaryLampFor(QStringLiteral("HARD"), true) == 6);
    CHECK(summaryLampFor(QStringLiteral("EXHARD"), true) == 7);
    CHECK(summaryLampFor(QStringLiteral("EXHARDDAN"), true) == 7);
    CHECK(summaryLampFor(QStringLiteral("FC"), true) == 8);
    CHECK(summaryLampFor(QStringLiteral("PERFECT"), true) == 9);
    CHECK(summaryLampFor(QStringLiteral("MAX"), true) == 10);
}

TEST_CASE("LR2 select detail state keeps LR2 clear lamp buckets",
          "[lr2][runtime][select]")
{
    CHECK(summaryLampFor(QStringLiteral("AEASY"), false) == 1);
    CHECK(summaryLampFor(QStringLiteral("LIGHTASSIST"), false) == 1);
    CHECK(summaryLampFor(QStringLiteral("EASY"), false) == 2);
    CHECK(summaryLampFor(QStringLiteral("NORMAL"), false) == 3);
    CHECK(summaryLampFor(QStringLiteral("HARD"), false) == 4);
    CHECK(summaryLampFor(QStringLiteral("EXHARD"), false) == 4);
    CHECK(summaryLampFor(QStringLiteral("FC"), false) == 5);
    CHECK(summaryLampFor(QStringLiteral("PERFECT"), false) == 5);
    CHECK(summaryLampFor(QStringLiteral("MAX"), false) == 5);
}

TEST_CASE("LR2 select detail state fills selected difficulty lamp from summary",
          "[lr2][runtime][select]")
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

    refreshState(state,
                 QStringLiteral("selected-difficulty-lamp"),
                 QVariantList{ QVariant::fromValue(score) },
                 3,
                 QVariantList{ 0, 0, 0, 1, 0, 0 },
                 QVariantList{ 0, 0, 0, 10, 0, 0 },
                 QVariantList{ 0, 0, 0, 0, 0, 0 },
                 true,
                 true);

    CHECK(state.difficultyModel()->lampForDifficulty(3) == 4);
    const QVariantList optionIds =
      state.difficultyModel()->optionIdsForKeymode(7, true);
    CHECK(optionIds.contains(507));
    CHECK(optionIds.contains(72));
    CHECK(optionIds.contains(512));
    CHECK(optionIds.contains(544));
}

TEST_CASE("LR2 select detail state can skip selected difficulty lamp",
          "[lr2][runtime][select]")
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

    refreshState(state,
                 QStringLiteral("selected-difficulty-lamp-skipped"),
                 QVariantList{ QVariant::fromValue(score) },
                 3,
                 QVariantList{ 0, 0, 0, 1, 0, 0 },
                 QVariantList{ 0, 0, 0, 10, 0, 0 },
                 QVariantList{},
                 true,
                 true);

    CHECK(state.difficultyModel()->lampForDifficulty(3) == 0);
}

TEST_CASE("LR2 select detail state can skip score option ids",
          "[lr2][runtime][select]")
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

    refreshState(state,
                 QStringLiteral("score-option-skip"),
                 QVariantList{ QVariant::fromValue(score) },
                 0,
                 QVariantList{},
                 QVariantList{},
                 QVariantList{},
                 true,
                 false);

    CHECK(state.summary()->lamp() == 4);
    CHECK(state.scoreOptionIds().toList().isEmpty());
}

TEST_CASE("LR2 select detail state keeps zero-point played scores as rank F",
          "[lr2][runtime][select]")
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithValues(QStringLiteral("FAILED"),
                                  0.0,
                                  0.0,
                                  QList<int>{ 0, 0, 0, 0, 0, 559 },
                                  0,
                                  &state);

    refreshState(state,
                 QStringLiteral("zero-point-failed"),
                 QVariantList{ QVariant::fromValue(score) },
                 0,
                 QVariantList{},
                 QVariantList{},
                 QVariantList{},
                 true,
                 true);

    REQUIRE(state.summary()->bestStatsObject() != nullptr);
    CHECK(state.summary()->lamp() == 1);
    CHECK(state.summary()->rank() == 1);
    CHECK(state.summary()->scoreRate() == 0.0);
    CHECK(state.summary()->scoreCounts()->play() == 1);
    CHECK(state.summary()->scoreCounts()->fail() == 1);
}

TEST_CASE("LR2 select detail state resolves selected score numbers in C++",
          "[lr2][runtime][select]")
{
    ensureCoreApplication();
    Lr2SelectDetailState state;
    auto* score = scoreWithValues(QStringLiteral("NORMAL"),
                                  42.0,
                                  21.0,
                                  QList<int>{ 5, 6, 4, 3, 2, 1 },
                                  7,
                                  &state);

    refreshState(state,
                 QStringLiteral("selected-number-values"),
                 QVariantList{ QVariant::fromValue(score) },
                 0,
                 QVariantList{},
                 QVariantList{},
                 QVariantList{},
                 true,
                 true);

    CHECK(state.resolvesSelectedNumberValue(101));
    CHECK_FALSE(state.resolvesSelectedNumberValue(30));
    CHECK(state.selectedNumberValue(80) == 1);
    CHECK(state.selectedNumberValue(81) == 2);
    CHECK(state.selectedNumberValue(82) == 3);
    CHECK(state.selectedNumberValue(83) == 4);
    CHECK(state.selectedNumberValue(84) == 5);
    CHECK(state.selectedNumberValue(101) == 21);
    CHECK(state.selectedNumberValue(102) == 50);
    CHECK(state.selectedNumberValue(104) == 7);
    CHECK(state.selectedNumberValue(425) == 9);
    CHECK(state.selectedNumberValue(426) == 11);
    CHECK(state.selectedNumberValue(427) == 15);
    CHECK(state.selectedBarGraphValue(40) == Catch::Approx(1.0 / 21.0));
    CHECK(state.selectedBarGraphValue(41) == Catch::Approx(2.0 / 21.0));
    CHECK(state.selectedBarGraphValue(46) == Catch::Approx(0.5));
}
