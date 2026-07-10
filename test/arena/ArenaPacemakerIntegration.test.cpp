#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QString>

#include <initializer_list>

namespace {

auto
sourceFile(const char* relativePath) -> QString
{
    const auto path = QDir(QStringLiteral(ARENA_QML_SOURCE_ROOT))
                        .filePath(QString::fromUtf8(relativePath));
    QFile file(path);
    INFO("Source: " << path.toStdString());
    REQUIRE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    return QString::fromUtf8(file.readAll());
}

void
requireContains(const QString& source,
                std::initializer_list<const char*> fragments)
{
    for (const auto* fragment : fragments) {
        const auto expected = QString::fromUtf8(fragment);
        INFO("Expected fragment: " << expected.toStdString());
        CHECK(source.contains(expected));
    }
}

auto
sectionFrom(const QString& source, const QString& marker, qsizetype length)
  -> QString
{
    const auto start = source.indexOf(marker);
    REQUIRE(start >= 0);
    return source.mid(start, length);
}

void
requireArenaBefore(const QString& section, const QString& fallback)
{
    const auto arena =
      section.indexOf(QStringLiteral("if (root.arenaGameplayOwned)"));
    const auto local = section.indexOf(fallback);
    REQUIRE(arena >= 0);
    REQUIRE(local >= 0);
    CHECK(arena < local);
}

} // namespace

TEST_CASE("ArenaPacemaker: Default gameplay forces the live opponent first",
          "[arena][ArenaPacemaker]")
{
    const auto source = sourceFile(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    requireContains(source,
                    { "readonly property bool arenaOpponentTargetAvailable",
                      "root.arenaSession.opponentTarget.available === true",
                      "root.arenaSession.opponentTarget.exScore" });

    const auto current =
      sectionFrom(source, QStringLiteral("property real targetPoints1:"), 1000);
    requireContains(current,
                    { "if (root.arenaGameplayOwned)",
                      "root.arenaOpponentTargetAvailable",
                      "root.arenaSession.opponentTarget.exScore",
                      ": 0",
                      "if (isBattle)",
                      "if (targetScore1)" });
    requireArenaBefore(current, QStringLiteral("if (isBattle)"));
    requireArenaBefore(current, QStringLiteral("if (targetScore1)"));

    const auto final =
      sectionFrom(source,
                  QStringLiteral("readonly property real targetFinalPoints1:"),
                  900);
    requireContains(final,
                    { "if (root.arenaGameplayOwned)",
                      "root.arenaOpponentTargetAvailable",
                      "root.arenaSession.opponentTarget.exScore",
                      ": 0",
                      "if (isBattle)",
                      "if (targetScore1)" });
    requireArenaBefore(final, QStringLiteral("if (isBattle)"));
    requireArenaBefore(final, QStringLiteral("if (targetScore1)"));
}

TEST_CASE("ArenaPacemaker: legacy gameplay forces one transient target",
          "[arena][ArenaPacemaker]")
{
    const auto source =
      sourceFile("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(source,
                    { "function arenaOpponentTargetAvailable() : bool",
                      "root.arenaGameplayOwned",
                      "root.arenaSession.opponentTarget.available === true" });

    const auto current = sectionFrom(
      source, QStringLiteral("function gameplayTargetScorePoints"), 1200);
    requireContains(current,
                    { "if (root.arenaGameplayOwned)",
                      "root.arenaOpponentTargetAvailable()",
                      "root.arenaSession.opponentTarget.exScore",
                      ": 0",
                      "if (root.battleModeActive())",
                      "root.gameplayTargetSavedScore()" });
    requireArenaBefore(current, QStringLiteral("if (root.battleModeActive())"));
    requireArenaBefore(current,
                       QStringLiteral("if (root.gameplayTargetSavedScore())"));

    const auto final = sectionFrom(
      source, QStringLiteral("function gameplayTargetFinalPoints"), 1200);
    requireContains(final,
                    { "if (root.arenaGameplayOwned)",
                      "root.arenaOpponentTargetAvailable()",
                      "root.arenaSession.opponentTarget.exScore",
                      ": 0",
                      "if (root.battleModeActive())",
                      "root.gameplayTargetSavedScore()" });
    requireArenaBefore(final, QStringLiteral("if (root.battleModeActive())"));
    requireArenaBefore(final,
                       QStringLiteral("root.gameplayTargetSavedScore()"));
}

TEST_CASE(
  "ArenaPacemaker: legacy formulas consume only effective target points",
  "[arena][ArenaPacemaker]")
{
    const auto source =
      sourceFile("RhythmGameQml/Lr2/Lr2SkinValueResolver.qml");
    requireContains(
      source,
      { "root.gameplayExScore(s1) - root.gameplayTargetScorePoints(1)",
        "root.gameplayTargetFinalPoints(1)",
        "root.gameplayScoreRateInteger(root.gameplayTargetFinalPoints(1), s1)",
        "root.gameplayScoreRateDecimal(root.gameplayTargetFinalPoints(1), "
        "s1)" });
    CHECK_FALSE(source.contains(QStringLiteral("ArenaSession")));
    CHECK_FALSE(source.contains(QStringLiteral("OnlineRankingModel")));
}

TEST_CASE(
  "ArenaPacemaker: gameplay leaves persisted target machinery untouched",
  "[arena][ArenaPacemaker]")
{
    const auto defaultSource = sourceFile(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    const auto legacySource =
      sourceFile("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");

    requireContains(defaultSource,
                    { "case ScoreTarget.BestScore",
                      "case ScoreTarget.LastScore",
                      "ScoreTarget.NextRank",
                      "vars.targetScoreFraction" });
    requireContains(legacySource,
                    { "case ScoreTarget.BestScore",
                      "case ScoreTarget.LastScore",
                      "ScoreTarget.NextRank",
                      "gameplayTargetFraction()" });

    for (const auto& source : { defaultSource, legacySource }) {
        CHECK_FALSE(
          source.contains(QStringLiteral("generalVars.scoreTarget = ")));
        CHECK_FALSE(source.contains(
          QStringLiteral("generalVars.targetScoreFraction = ")));
        CHECK_FALSE(source.contains(QStringLiteral("setScoreTarget(")));
        CHECK_FALSE(source.contains(QStringLiteral("setTargetScoreFraction(")));
        CHECK_FALSE(source.contains(QStringLiteral("OnlineRankingModel")));
    }

    CHECK(defaultSource.count(QStringLiteral("ScoreReplayer {")) == 2);
    CHECK(legacySource.count(QStringLiteral("ScoreReplayer {")) == 2);
}
