#include <catch2/catch_test_macros.hpp>

#include <QDir>
#include <QFile>
#include <QString>

#include <initializer_list>

namespace {

auto
qmlSource(const char* relativePath) -> QString
{
    const auto path = QDir(QStringLiteral(ARENA_QML_SOURCE_ROOT))
                        .filePath(QString::fromUtf8(relativePath));
    QFile file(path);
    INFO("QML source: " << path.toStdString());
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

} // namespace

TEST_CASE(
  "ArenaResultPresentation: central result capture owns presentation lifecycle",
  "[arena][ArenaResultPresentation]")
{
    const auto contentFrame = qmlSource("RhythmGameQml/ContentFrame.qml");
    const auto openResult =
      sectionFrom(contentFrame, QStringLiteral("function openResult("), 2600);
    requireContains(openResult,
                    { "submitLocalResult(scores[0])",
                      "presentedResult.roundId",
                      "arenaRoundId",
                      "sceneStack.pushItem(resultComponent, props)" });
    CHECK(openResult.indexOf(QStringLiteral("submitLocalResult(scores[0])")) <
          openResult.indexOf(
            QStringLiteral("sceneStack.pushItem(resultComponent, props)")));

    const auto defaultResult =
      qmlSource("share/RhythmGame/themes/Default/scripts/result/Result.qml");
    requireContains(
      defaultResult,
      { "property string arenaRoundId",
        "readonly property bool arenaNativeResultPresentation: true",
        "ArenaResultPanel",
        "endResultPresentation(root.arenaRoundId)" });

    const auto legacyResult =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(legacyResult,
                    { "property string arenaRoundId",
                      "property bool arenaResultCustomizationActive",
                      "function setArenaResultCustomizationActive",
                      "&& !root.arenaResultCustomizationActive",
                      "endResultPresentation(root.arenaRoundId)" });
}

TEST_CASE("ArenaResultPresentation: Default ranking source stays screen-local",
          "[arena][ArenaResultPresentation]")
{
    const auto side =
      qmlSource("share/RhythmGame/themes/Default/scripts/result/Side.qml");
    requireContains(side,
                    { "effectiveResultSource",
                      "\"arena\"",
                      "Arena -> RhythmGame -> Tachi -> LR2IR",
                      "[\"arena\", \"rhythmGame\", \"tachi\", \"lr2ir\"]",
                      "onLeftClicked: cycleEffectiveResultSource(1)",
                      "onRightClicked: cycleEffectiveResultSource(-1)",
                      "presentedResult",
                      "arenaSelected",
                      "setEffectiveResultSource" });
    CHECK_FALSE(side.contains(QStringLiteral("OnlineRankingModel.Arena")));

    const auto setter = sectionFrom(
      side, QStringLiteral("function setEffectiveResultSource"), 1200);
    requireContains(
      setter,
      { "if (!side.arenaResultMatches)", "generalVars.rankingProvider" });

    const auto rankingPosition = qmlSource(
      "share/RhythmGame/themes/Default/scripts/result/RankingPosition.qml");
    requireContains(rankingPosition,
                    { "required property bool arenaSelected",
                      "arenaFinalized",
                      "arenaLocalDnf",
                      "arenaLocalRank",
                      "arenaParticipantCount",
                      "visible: !rankingPosition.arenaSelected",
                      "visible: rankingPosition.arenaSelected" });

    const auto panel = qmlSource(
      "share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml");
    requireContains(panel,
                    { "property bool expanded: true",
                      "panel.result.winnerNames",
                      "panel.result.localDnf",
                      "panel.result.localRank",
                      "panel.result.participantCount",
                      "model: panel.result && panel.result.valid",
                      "required property int lobbyWinsAfter",
                      "required property int perfect",
                      "required property int emptyPoor",
                      "textFormat: Text.PlainText" });
}

TEST_CASE("ArenaResultPresentation: legacy fallback uses result theme vars and "
          "narrow values",
          "[arena][ArenaResultPresentation]")
{
    const auto overlay =
      qmlSource("RhythmGameQml/Arena/ArenaResultOverlay.qml");
    requireContains(overlay,
                    { "required property var session",
                      "required property string placementKind",
                      "required property string resolvedSkinId",
                      "required property string layoutVariant",
                      "session.presentedResult",
                      "winnerNames",
                      "lobbyWinsAfter",
                      "textFormat: Text.PlainText" });

    const auto qmlModule = qmlSource("RhythmGameQml/CMakeLists.txt");
    CHECK(qmlModule.contains(QStringLiteral("Arena/ArenaResultOverlay.qml")));

    const auto host = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    requireContains(host,
                    { "readonly property bool ownsArenaResult",
                      "arenaNativeResultPresentation",
                      "resultThemeVars",
                      "ArenaOverlayPlacementFrame",
                      "themeVars: root.resultThemeVars",
                      "placementKind: \"resultStandings\"",
                      "layoutVariant: \"result\"",
                      "ArenaResultOverlay",
                      "resultInputGuardActive",
                      "Qt.callLater" });
    CHECK_FALSE(host.contains(QStringLiteral("placementStore")));

    const auto resolver =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinValueResolver.qml");
    const auto numbers =
      sectionFrom(resolver, QStringLiteral("case 179:"), 900);
    requireContains(numbers,
                    { "arenaResultMatches",
                      "presentedResult.finalized",
                      "presentedResult.localRank",
                      "rankingState.playerRank()",
                      "case 180:",
                      "presentedResult.participantCount",
                      "rankingState.currentPlayerCount" });
}
