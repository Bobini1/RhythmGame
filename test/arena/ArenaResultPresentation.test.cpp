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
        "readonly property var arenaOverlayPlacementFrame",
        "ArenaOverlayPlacementFrame",
        "directMoveEnabled: true",
        "directResizeEnabled: true",
        "ArenaResultPanel",
        "const arenaSession = Rg.arenaSession",
        "if (root.arenaRoundId.length > 0 && arenaSession)",
        "arenaSession.endResultPresentation(root.arenaRoundId)" });

    const auto legacyResult =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(legacyResult,
                    { "property string arenaRoundId",
                      "property bool arenaResultCustomizationActive",
                      "function setArenaResultCustomizationActive",
                      "&& !root.arenaResultCustomizationActive",
                      "const arenaSession = root.arenaSession",
                      "if (root.arenaRoundId.length > 0 && arenaSession)",
                      "arenaSession.endResultPresentation(root.arenaRoundId)" });
}

TEST_CASE("ArenaResultPresentation: covered Arena gameplay is removed after result",
          "[arena][ArenaResultPresentation]")
{
    const auto contentFrame = qmlSource("RhythmGameQml/ContentFrame.qml");
    requireContains(contentFrame,
                    { "property Item pendingArenaGameplayCloseItem",
                      "pendingArenaGameplayCloseItem = item",
                      "item.arenaPendingAutoClose = true",
                      "currentItem === globalRoot.pendingArenaGameplayCloseItem",
                      "sceneStack.popCurrentItem()" });

    const auto defaultGameplay = qmlSource(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    const auto legacyGameplay =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(defaultGameplay,
                    { "property bool arenaPendingAutoClose: false",
                      "if (root.arenaPendingAutoClose)" });
    requireContains(legacyGameplay,
                    { "property bool arenaPendingAutoClose: false",
                      "if (root.arenaPendingAutoClose)" });
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

TEST_CASE("ArenaResultPresentation: final standings are localized and accessible",
          "[arena][ArenaResultPresentation]")
{
    const auto competitionText =
      qmlSource("RhythmGameQml/Arena/ArenaCompetitionText.qml");
    requireContains(competitionText,
                    { "function gaugeTypeText",
                      "function clearTypeText",
                      "function dnfReasonText",
                      "function winsText",
                      "case \"result_unavailable\"",
                      "case \"grace_expired\"",
                      "qsTr(\"%n win(s)\"" });

    const auto announcer =
      qmlSource("RhythmGameQml/Arena/ArenaResultAnnouncer.qml");
    requireContains(announcer,
                    { "Instantiator",
                      "required property var result",
                      "Did not finish: %3",
                      "dnfReasonText",
                      "Accessible.announce",
                      "lastAnnouncementText",
                      "announcementCount" });

    const auto legacy =
      qmlSource("RhythmGameQml/Arena/ArenaResultOverlay.qml");
    requireContains(legacy,
                    { "ArenaCompetitionText",
                      "ArenaResultAnnouncer",
                      "Accessible.role: Accessible.Pane",
                      "Accessible.role: Accessible.List",
                      "Accessible.role: Accessible.ListItem",
                      "activeFocusOnTab: visible && count > 0",
                      "activeFocusOnTab: false",
                      "keyNavigationEnabled: true",
                      "Qt.Key_End",
                      "focusIndicatorVisible",
                      "readonly property alias announcementCount",
                      "lastAnnouncementText" });
    CHECK_FALSE(legacy.contains(
      QStringLiteral(".arg(standingDelegate.dnfReason)")));
    CHECK_FALSE(legacy.contains(
      QStringLiteral(".arg(standingDelegate.clearType)")));
    CHECK_FALSE(legacy.contains(
      QStringLiteral(".arg(standingDelegate.gaugeType)")));

    const auto native = qmlSource(
      "share/RhythmGame/themes/Default/scripts/result/ArenaResultPanel.qml");
    requireContains(native,
                    { "ArenaCompetitionText",
                      "ArenaResultAnnouncer",
                      "Accessible.role: Accessible.Pane",
                      "Accessible.role: Accessible.List",
                      "Accessible.role: Accessible.ListItem",
                      "activeFocusOnTab: visible && count > 0",
                      "activeFocusOnTab: false",
                      "keyNavigationEnabled: true",
                      "Qt.Key_End",
                      "focusIndicatorVisible",
                      "font.contextFontMerging: true",
                      "readonly property alias announcementCount",
                      "lastAnnouncementText" });
    CHECK_FALSE(native.contains(QStringLiteral(".arg(row.dnfReason)")));
    CHECK_FALSE(native.contains(QStringLiteral(".arg(row.clearType)")));
    CHECK_FALSE(native.contains(QStringLiteral(".arg(row.gaugeType)")));

    const auto defaultResult =
      qmlSource("share/RhythmGame/themes/Default/scripts/result/Result.qml");
    requireContains(defaultResult,
                    { "id: resultBackground",
                      "id: resultTitleFont",
                      "parent: resultBackground",
                      "statsFontFamily: resultStatsFont.fontFamily",
                      "textFontFamily: resultTitleFont.fontFamily" });

    const auto qmlModule = qmlSource("RhythmGameQml/CMakeLists.txt");
    CHECK(qmlModule.contains(
      QStringLiteral("Arena/ArenaResultAnnouncer.qml")));
}
