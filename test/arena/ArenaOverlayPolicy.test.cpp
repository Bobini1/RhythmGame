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

TEST_CASE("ArenaOverlayPolicy: overlay host follows only the current session runner",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/Arena/ArenaOverlayHost.qml");
    requireContains(source,
                    { "required property var session",
                      "required property var currentItem",
                      "readonly property bool ownsArenaRunner",
                      "session.arenaGameplayActive === true",
                      "root.currentItem.chart === root.session.arenaRunner",
                      "active: root.ownsArenaRunner",
                      "sequence: \"F8\"",
                      "session.toggleGameplayChat()",
                      "placementKind: \"gameplayLeaderboard\"",
                      "topMargin: 24",
                      "rightMargin: 24",
                      "Math.min(420",
                      "root.height - 48" });
}

TEST_CASE("ArenaOverlayPolicy: gameplay overlay is forced, bounded, and complete",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source =
      qmlSource("RhythmGameQml/Arena/ArenaGameplayOverlay.qml");
    requireContains(source,
                    { "required property var session",
                      "required property string placementKind",
                      "required property string resolvedSkinId",
                      "required property string layoutVariant",
                      "property bool expanded: false",
                      "model: root.session.liveStandings",
                      "required property string displayName",
                      "required property string competitionState",
                      "required property int rank",
                      "required property bool hasScore",
                      "required property var exScore",
                      "required property int progressPermille",
                      "required property int maxCombo",
                      "required property int badPoorCount",
                      "required property int perfect",
                      "required property int great",
                      "required property int good",
                      "required property int bad",
                      "required property int poor",
                      "required property int emptyPoor",
                      "required property string gaugeType",
                      "required property int gaugeValueMilli",
                      "root.session.arenaOptionsSummary",
                      "clip: true",
                      "ScrollBar.vertical",
                      "textFormat: Text.PlainText",
                      "active: root.session.gameplayChatOpen === true" });

    CHECK_FALSE(source.contains(QStringLiteral("MouseArea")));
    CHECK_FALSE(source.contains(QStringLiteral("DragHandler")));
    CHECK_FALSE(source.contains(QStringLiteral("TapHandler")));
    CHECK_FALSE(source.contains(QStringLiteral("Settings {")));
}

TEST_CASE("ArenaOverlayPolicy: gameplay chat stays plain text and owns keyboard submission",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/Arena/ArenaGameplayChat.qml");
    requireContains(source,
                    { "required property var session",
                      "model: root.session.chat",
                      "root.session.sendChat(message)",
                      "root.session.setGameplayChatOpen(false)",
                      "event.key === Qt.Key_Escape",
                      "Qt.Key_Return",
                      "Qt.Key_Enter",
                      "Qt.ShiftModifier",
                      "textFormat: Text.PlainText" });
    CHECK(source.count(QStringLiteral("textFormat: Text.PlainText")) >= 2);
    CHECK_FALSE(source.contains(QStringLiteral("pause("), Qt::CaseInsensitive));
}

TEST_CASE("ArenaOverlayPolicy: gameplay Escape closes chat before one abandon command",
          "[arena][ArenaOverlayPolicy]")
{
    const auto defaultSource = qmlSource(
      "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml");
    requireContains(defaultSource,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "readonly property bool arenaGameplayOwned",
                      "root.arenaSession.gameplayChatOpen",
                      "root.arenaSession.setGameplayChatOpen(false)",
                      "root.arenaSession.abandonCurrentRound()" });
    const auto defaultEscape = sectionFrom(
      defaultSource, QStringLiteral("id: escapeShortcut"), 1800);
    const auto defaultClose =
      defaultEscape.indexOf(QStringLiteral("setGameplayChatOpen(false)"));
    const auto defaultAbandon =
      defaultEscape.indexOf(QStringLiteral("abandonCurrentRound()"));
    const auto defaultExisting =
      defaultEscape.indexOf(QStringLiteral("if (nothingWasHit)"));
    REQUIRE(defaultClose >= 0);
    REQUIRE(defaultAbandon >= 0);
    REQUIRE(defaultExisting >= 0);
    CHECK(defaultClose < defaultAbandon);
    CHECK(defaultAbandon < defaultExisting);
    CHECK(defaultEscape.count(QStringLiteral("abandonCurrentRound()")) == 1);

    const auto legacySource =
      qmlSource("RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml");
    requireContains(legacySource,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "readonly property bool arenaGameplayOwned",
                      "root.arenaSession.gameplayChatOpen",
                      "root.arenaSession.setGameplayChatOpen(false)",
                      "root.arenaSession.abandonCurrentRound()" });
    const auto legacyEscape = sectionFrom(
      legacySource, QStringLiteral("function handleGameplayEscape()"), 1800);
    const auto legacyClose =
      legacyEscape.indexOf(QStringLiteral("setGameplayChatOpen(false)"));
    const auto legacyAbandon =
      legacyEscape.indexOf(QStringLiteral("abandonCurrentRound()"));
    const auto legacyExisting =
      legacyEscape.indexOf(QStringLiteral("if (root.gameplayNothingWasHit)"));
    REQUIRE(legacyClose >= 0);
    REQUIRE(legacyAbandon >= 0);
    REQUIRE(legacyExisting >= 0);
    CHECK(legacyClose < legacyAbandon);
    CHECK(legacyAbandon < legacyExisting);
    CHECK(legacyEscape.count(QStringLiteral("abandonCurrentRound()")) == 1);
}

TEST_CASE("ArenaOverlayPolicy: browser makes pre-competition connections browse-only",
          "[arena][ArenaOverlayPolicy]")
{
    const auto browser = qmlSource("RhythmGameQml/Arena/ArenaBrowser.qml");
    requireContains(browser,
                    { "required property var session",
                      "session.directoryReady",
                      "!session.competitionAvailable",
                      "&& !updateRequired",
                      "active: !root.updateRequired",
                      "admissionAllowed: !root.updateRequired",
                      "Update RhythmGame to create or join Arena rooms." });

    const auto login = qmlSource("RhythmGameQml/Arena/ArenaLoginPanel.qml");
    requireContains(login,
                    { "required property bool admissionAllowed",
                      "enabled: root.admissionAllowed",
                      "if (!root.admissionAllowed" });
}

TEST_CASE("ArenaOverlayPolicy: ContentFrame hosts Arena above the active gameplay skin",
          "[arena][ArenaOverlayPolicy]")
{
    const auto source = qmlSource("RhythmGameQml/ContentFrame.qml");
    requireContains(source,
                    { "readonly property var arenaSession: Rg.arenaSession",
                      "function gameplayLayoutVariant",
                      "mainProfile.themeConfig[layoutVariant]",
                      "ArenaOverlayHost {",
                      "session: globalRoot.arenaSession",
                      "currentItem: sceneStack.currentItem",
                      "resolvedSkinId:",
                      "layoutVariant:" });
    CHECK(source.indexOf(QStringLiteral("id: sceneStack")) <
          source.indexOf(QStringLiteral("ArenaOverlayHost {")));
}
