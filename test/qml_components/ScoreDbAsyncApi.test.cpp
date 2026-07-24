#include "qml_components/ScoreDb.h"
#include "support/PendingReply.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QFile>
#include <QPointer>
#include <QTemporaryDir>

#include <concepts>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

namespace {

template<typename T>
concept HasCancelPending = requires(T& value) { value.cancelPending(); };

static_assert(!HasCancelPending<qml_components::ScoreDb>);
static_assert(std::same_as<
              decltype(std::declval<qml_components::ScoreDb&>().getScoresForMd5(
                std::declval<const QList<QString>&>())),
              support::PendingReply*>);
static_assert(std::same_as<decltype(std::declval<qml_components::ScoreDb&>()
                                      .getTotalStats()),
                           support::PendingReply*>);

void
ensureCoreApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    if (!QCoreApplication::instance()) {
        [[maybe_unused]] static auto* app = new QCoreApplication(argc, argv);
    }
}

auto
readSource(std::string_view relativePath) -> QByteArray
{
    const auto path =
      QStringLiteral(RHYTHMGAME_SOURCE_DIR "/") +
      QString::fromUtf8(relativePath.data(),
                        static_cast<qsizetype>(relativePath.size()));
    auto file = QFile{ path };
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

} // namespace

TEST_CASE("ScoreDb async API exposes cancellation on returned replies",
          "[ScoreDb][PendingReply]")
{
    SUCCEED();
}

TEST_CASE("ScoreDb QML owners retain and cancel their replies",
          "[ScoreDb][PendingReply][qml-contract]")
{
    struct Contract
    {
        std::string_view path;
        std::initializer_list<std::string_view> requiredText;
    };

    const auto contracts = {
        Contract{ "RhythmGameQml/Lr2/Lr2ResultState.qml",
                  { "property var pendingResultScoreDbReplies: []",
                    "cancelResultScoreDbReplies();",
                    "trackResultScoreDbReply(scoreDb.getScoresForCourseId",
                    "trackResultScoreDbReply(scoreDb.getScoresForMd5",
                    "Component.onDestruction" } },
        Contract{ "RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml",
                  { "property var pendingGameplayScoreDbReply: null",
                    "cancelGameplayScoreDbReply();",
                    "trackGameplayScoreDbReply(scoreDb.getScoresForMd5",
                    "Component.onDestruction" } },
        Contract{
          "share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml",
          { "property var pendingScoreDbReply: null",
            "cancelScoreDbReply();",
            "trackScoreDbReply("
            "chart.player1.profile.scoreDb.getScoresForMd5",
            "Component.onDestruction" } },
        Contract{ "share/RhythmGame/themes/Default/scripts/result/Side.qml",
                  { "property var pendingScoreDbReply: null",
                    "cancelScoreDbReply();",
                    "trackScoreDbReply(profile.scoreDb.getScoresForCourseId",
                    "trackScoreDbReply(profile.scoreDb.getScoresForMd5",
                    "Component.onDestruction" } },
    };

    for (const auto& contract : contracts) {
        const auto source = readSource(contract.path);
        for (const auto requiredText : contract.requiredText) {
            CAPTURE(contract.path, requiredText);
            CHECK(source.contains(
              QByteArrayView{ requiredText.data(),
                              static_cast<qsizetype>(requiredText.size()) }));
        }
    }
}

TEST_CASE("LR2 folder lamp queries have a dedicated cancellation scope",
          "[ScoreDb][PendingReply][qml-contract]")
{
    const auto source = readSource("RhythmGameQml/Lr2/Lr2SelectContext.qml");
    CHECK(source.contains("property var pendingFolderLampScoreDbReplies: []"));
    CHECK(
      source.contains("trackFolderLampScoreDbReply(db.getScoreSummary(item))"));

    const auto refreshStart = source.indexOf("function refreshFolderLamps()");
    const auto refreshEnd =
      source.indexOf("function clearFolderLampState()", refreshStart);
    REQUIRE(refreshStart >= 0);
    REQUIRE(refreshEnd > refreshStart);
    const auto refreshBody =
      QByteArrayView{ source }.sliced(refreshStart, refreshEnd - refreshStart);
    CHECK(refreshBody.contains("cancelFolderLampScoreDbReplies();"));
    CHECK_FALSE(refreshBody.contains("cancelScoreDbReplies();"));
}

TEST_CASE("ScoreDb teardown settles pending replies before child destruction",
          "[ScoreDb][PendingReply][lifetime]")
{
    ensureCoreApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto database = db::SqliteCppDb{ std::filesystem::path{
      directory.filePath(QStringLiteral("scores.db")).toStdString() } };
    database.execute(
      "CREATE TABLE score("
      "clear_type TEXT, perfect INTEGER, great INTEGER, good INTEGER, "
      "bad INTEGER, poor INTEGER, empty_poor INTEGER, max_combo INTEGER)");

    constexpr auto queryCount = 16;
    auto finishedCount = 0;
    auto replies = QList<QPointer<support::PendingReply>>{};
    {
        auto scoreDb = std::make_unique<qml_components::ScoreDb>(&database);
        for (auto index = 0; index < queryCount; ++index) {
            auto* reply = scoreDb->getTotalStats();
            replies.append(reply);
            QObject::connect(reply,
                             &support::PendingReply::finished,
                             [&finishedCount] { ++finishedCount; });
        }
    }

    CHECK(finishedCount == queryCount);
    for (const auto& reply : replies)
        CHECK(reply.isNull());
    QCoreApplication::sendPostedEvents(QCoreApplication::instance(),
                                       QEvent::MetaCall);
}

TEST_CASE("ScoreDb teardown tolerates sibling deletion from finished handlers",
          "[ScoreDb][PendingReply][lifetime]")
{
    ensureCoreApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    auto database = db::SqliteCppDb{ std::filesystem::path{
      directory.filePath(QStringLiteral("scores.db")).toStdString() } };
    database.execute(
      "CREATE TABLE score("
      "clear_type TEXT, perfect INTEGER, great INTEGER, good INTEGER, "
      "bad INTEGER, poor INTEGER, empty_poor INTEGER, max_combo INTEGER)");

    auto firstReply = QPointer<support::PendingReply>{};
    auto secondReply = QPointer<support::PendingReply>{};
    {
        auto scoreDb = std::make_unique<qml_components::ScoreDb>(&database);
        firstReply = scoreDb->getTotalStats();
        secondReply = scoreDb->getTotalStats();
        QObject::connect(firstReply,
                         &support::PendingReply::finished,
                         [&secondReply] { delete secondReply.data(); });
        QObject::connect(secondReply,
                         &support::PendingReply::finished,
                         [&firstReply] { delete firstReply.data(); });
    }

    CHECK(firstReply.isNull());
    CHECK(secondReply.isNull());
    QCoreApplication::sendPostedEvents(QCoreApplication::instance(),
                                       QEvent::MetaCall);
}
