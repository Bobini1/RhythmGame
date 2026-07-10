#include "FakeArenaGameplaySource.h"
#include "FakeArenaIdentityProvider.h"
#include "FakeArenaInventorySource.h"
#include "FakeArenaRoundLoader.h"
#include "FakeArenaScheduler.h"
#include "FakeArenaTransport.h"
#include "arena/ArenaSession.h"
#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/NoteState.h"
#include "gameplay_logic/rules/HitRules.h"
#include "qml_components/Bga.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPromise>
#include <QThread>

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using namespace std::chrono_literals;

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "ArenaSessionCompetitionTests";
    static char* argv[]{ applicationName, nullptr };
    static const auto application =
      std::make_unique<QCoreApplication>(argc, argv);
}

template<typename T>
auto
readyFuture(T value) -> QFuture<T>
{
    QPromise<T> promise;
    promise.start();
    auto future = promise.future();
    promise.addResult(std::move(value));
    promise.finish();
    return future;
}

auto
makeRunnerChart() -> std::unique_ptr<gameplay_logic::ChartData>
{
    return std::make_unique<gameplay_logic::ChartData>(
      QStringLiteral("Arena competition"),
      QStringLiteral("Composer"),
      QString{},
      QString{},
      QString{},
      QString{},
      QString{},
      QString{},
      75.0,
      100.0,
      1,
      1,
      false,
      QList<qint64>{},
      0,
      0,
      0,
      0,
      0,
      1'000,
      120.0,
      120.0,
      120.0,
      120.0,
      120.0,
      0.0,
      0.0,
      0.0,
      QStringLiteral("arena-competition.bms"),
      0,
      QString(64, QChar(u'1')),
      QString(32, QChar(u'2')),
      gameplay_logic::ChartData::Keymode::K7,
      QList<QList<qint64>>{},
      QList<gameplay_logic::BpmChange>{},
      0);
}

struct RunnerFixture
{
    std::unique_ptr<gameplay_logic::ChartRunner> runner;
    gameplay_logic::Player* player{};
};

auto
makeReadyRunner(std::chrono::nanoseconds length = 1s) -> RunnerFixture
{
    ensureCoreApplication();
    auto* score = new gameplay_logic::BmsLiveScore{
        0,
        0,
        0,
        0,
        0,
        0,
        0.0,
        {},
        {},
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::NoteOrderAlgorithm::Normal,
        resource_managers::DpOptions::Off,
        {},
        0,
        length.count(),
        QString(64, QChar(u'1')),
        QString(32, QChar(u'2')),
        gameplay_logic::ChartData::Keymode::K7,
        0,
        QStringLiteral("arena-score-guid"),
    };
    std::array<std::vector<charts::BmsNotesData::Note>,
               charts::BmsNotesData::columnNumber>
      rawNotes{};
    auto referee = gameplay_logic::BmsGameReferee{
        std::move(rawNotes),
        {},
        { charts::BmsNotesData::BpmChangeValues{
          .bpm = 120.0,
          .scroll = 1.0,
          .timestamp = {},
        } },
        {},
        score,
        std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{},
        gameplay_logic::rules::HitRules{
          {},
          [](std::chrono::nanoseconds, gameplay_logic::Judgement) {
              return 0.0;
          } },
    };
    auto* player = new gameplay_logic::Player{
        new gameplay_logic::BmsNotes{},
        score,
        new gameplay_logic::GameplayState{
          {}, new gameplay_logic::BarLinesState{ {} } },
        nullptr,
        readyFuture(std::move(referee)),
        length,
        120.0,
    };
    auto runner = std::make_unique<gameplay_logic::ChartRunner>(
      makeRunnerChart().release(),
      readyFuture(std::make_unique<qml_components::BgaContainer>(
        QList<qml_components::Bga*>{},
        std::vector<QMediaPlayer*>{},
        std::vector<std::unique_ptr<QVideoFrame>>{})),
      gameplay_logic::ChartData::Keymode::K7,
      player,
      nullptr);
    QElapsedTimer timeout;
    timeout.start();
    while (runner->getStatus() != gameplay_logic::ChartRunner::Ready &&
           timeout.elapsed() < 2'000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    REQUIRE(runner->getStatus() == gameplay_logic::ChartRunner::Ready);
    return { std::move(runner), player };
}

auto
gaugeInfo() -> gameplay_logic::BmsGaugeInfo
{
    return { .maxGauge = 100.0,
             .threshold = 80.0,
             .name = QStringLiteral("NORMAL"),
             .courseGauge = false,
             .gaugeHistory = {
               gameplay_logic::rules::GaugeHistoryEntry{ 0, 81.0 } } };
}

auto
makeScore(QString guid = QStringLiteral("arena-score-guid"))
  -> std::unique_ptr<gameplay_logic::BmsScore>
{
    auto result = std::make_unique<gameplay_logic::BmsResult>(
      200.0,
      100,
      100,
      0,
      0,
      0,
      0,
      QStringLiteral("NORMAL"),
      QList<int>{ 0, 0, 0, 0, 0, 0 },
      0,
      0.0,
      0,
      0,
      1'000'000'000,
      QList<qint64>{},
      0,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::NoteOrderAlgorithm::Normal,
      resource_managers::DpOptions::Off,
      gameplay_logic::ChartData::Keymode::K7,
      guid,
      QString(64, QChar(u'1')),
      QString(32, QChar(u'2')));
    return std::make_unique<gameplay_logic::BmsScore>(
      std::move(result),
      std::make_unique<gameplay_logic::BmsReplayData>(
        QList<gameplay_logic::HitEvent>{}, guid),
      std::make_unique<gameplay_logic::BmsGaugeHistory>(
        QList<gameplay_logic::BmsGaugeInfo>{ gaugeInfo() }, guid));
}

auto
compact(QJsonObject value) -> QString
{
    return QString::fromUtf8(
      QJsonDocument(std::move(value)).toJson(QJsonDocument::Compact));
}

auto
messageObject(const QString& text) -> QJsonObject
{
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
    REQUIRE(error.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());
    return document.object();
}

auto
identity(QString id, QString name) -> QJsonObject
{
    return { { QStringLiteral("userId"), std::move(id) },
             { QStringLiteral("displayName"), std::move(name) },
             { QStringLiteral("avatarUrl"), QJsonValue::Null } };
}

auto
member(QString id,
       QString name,
       qint64 wins = 0,
       qint64 inventoryRevision = 6,
       qint64 availabilityRevision = 5) -> QJsonObject
{
    const auto userId = id == QStringLiteral("member-1")
                          ? QStringLiteral("user-1")
                          : QStringLiteral("user-2");
    return {
        { QStringLiteral("memberId"), std::move(id) },
        { QStringLiteral("identity"), identity(userId, std::move(name)) },
        { QStringLiteral("status"), QStringLiteral("connected") },
        { QStringLiteral("lobbyWins"), wins },
        { QStringLiteral("ready"), false },
        { QStringLiteral("inventoryState"), QStringLiteral("ready") },
        { QStringLiteral("inventoryRevision"), inventoryRevision },
        { QStringLiteral("availabilityAppliedRevision"), availabilityRevision },
        { QStringLiteral("roundState"), QStringLiteral("eligible") },
    };
}

auto
selection() -> QJsonObject
{
    return {
        { QStringLiteral("sha256"), QString(64, QChar(u'1')) },
        { QStringLiteral("title"), QStringLiteral("Competition chart") },
        { QStringLiteral("subtitle"), QString{} },
        { QStringLiteral("artist"), QStringLiteral("Composer") },
        { QStringLiteral("keyMode"), 7 },
        { QStringLiteral("randomSequence"), QJsonArray{ 1, 2, 3 } },
        { QStringLiteral("noteOrderP1"), QStringLiteral("random") },
        { QStringLiteral("noteOrderP2"), QStringLiteral("mirror") },
        { QStringLiteral("dpMode"), QStringLiteral("off") },
        { QStringLiteral("laneSeed"), QStringLiteral("0123456789abcdef") },
        { QStringLiteral("randomizationVersion"), 1 },
    };
}

auto
frozenParticipant(QString id, QString name) -> QJsonObject
{
    const auto userId = id == QStringLiteral("member-1")
                          ? QStringLiteral("user-1")
                          : QStringLiteral("user-2");
    return {
        { QStringLiteral("memberId"), std::move(id) },
        { QStringLiteral("inventoryRevision"), 6 },
        { QStringLiteral("identity"), identity(userId, std::move(name)) },
    };
}

auto
frozenRound(QString stage, bool twoPlayers = false) -> QJsonObject
{
    auto participants = QJsonArray{ frozenParticipant(
      QStringLiteral("member-1"), QStringLiteral("Alice")) };
    if (twoPlayers) {
        participants.append(
          frozenParticipant(QStringLiteral("member-2"), QStringLiteral("Bob")));
    }
    auto result = QJsonObject{
        { QStringLiteral("roundId"), QStringLiteral("round-1") },
        { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
        { QStringLiteral("selectionRevision"), 4 },
        { QStringLiteral("availabilityRevision"), 5 },
        { QStringLiteral("selection"), selection() },
        { QStringLiteral("participants"), std::move(participants) },
        { QStringLiteral("stage"), std::move(stage) },
    };
    if (result.value(QStringLiteral("stage")).toString() ==
          QStringLiteral("scheduled") ||
        result.value(QStringLiteral("stage")).toString() ==
          QStringLiteral("playing")) {
        result.insert(QStringLiteral("playDeadlineAtServerMs"), 400'000);
    }
    return result;
}

auto
competitionHello(bool authenticated,
                 std::optional<QJsonObject> resumedRoom = std::nullopt)
  -> QString
{
    auto data = QJsonObject{
        { QStringLiteral("protocolMajor"), 1 },
        { QStringLiteral("protocolMinor"), 2 },
        { QStringLiteral("capabilities"),
          QJsonArray{ QStringLiteral("rooms-v1"),
                      QStringLiteral("rounds-v1"),
                      QStringLiteral("competition-v1") } },
    };
    if (authenticated) {
        data.insert(
          QStringLiteral("identity"),
          identity(QStringLiteral("user-1"), QStringLiteral("Alice")));
    }
    data.insert(
      QStringLiteral("resume"),
      resumedRoom
        ? QJsonObject{ { QStringLiteral("status"),
                         QStringLiteral("succeeded") },
                       { QStringLiteral("room"), std::move(*resumedRoom) } }
        : QJsonObject{
            { QStringLiteral("status"), QStringLiteral("not_requested") } });
    return compact({ { QStringLiteral("type"), QStringLiteral("server_hello") },
                     { QStringLiteral("data"), std::move(data) } });
}

auto
legacyHello() -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("server_hello") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("protocolMajor"), 1 },
          { QStringLiteral("protocolMinor"), 0 },
          { QStringLiteral("capabilities"),
            QJsonArray{ QStringLiteral("rooms-v1") } },
          { QStringLiteral("resume"),
            QJsonObject{
              { QStringLiteral("status"), QStringLiteral("not_requested") } } },
        } },
    });
}

auto
roomData(QString phase = QStringLiteral("selecting"),
         std::optional<QString> roundStage = std::nullopt,
         bool twoPlayers = false,
         qint64 connectionGeneration = 3,
         QJsonValue liveStandings = QJsonValue::Null,
         QJsonValue lastResult = QJsonValue::Null) -> QJsonObject
{
    auto members =
      QJsonArray{ member(QStringLiteral("member-1"), QStringLiteral("Alice")) };
    if (twoPlayers) {
        members.append(
          member(QStringLiteral("member-2"), QStringLiteral("Bob")));
    }
    auto result = QJsonObject{
        { QStringLiteral("roomId"), QStringLiteral("room-1") },
        { QStringLiteral("roomGeneration"), 2 },
        { QStringLiteral("name"), QStringLiteral("Competition room") },
        { QStringLiteral("phase"), std::move(phase) },
        { QStringLiteral("hasPassword"), false },
        { QStringLiteral("maxCount"), 16 },
        { QStringLiteral("ownerMemberId"), QStringLiteral("member-1") },
        { QStringLiteral("self"),
          QJsonObject{
            { QStringLiteral("memberId"), QStringLiteral("member-1") },
            { QStringLiteral("connectionGeneration"), connectionGeneration },
            { QStringLiteral("resumeToken"),
              QStringLiteral("resume-token") } } },
        { QStringLiteral("members"), std::move(members) },
        { QStringLiteral("chat"), QJsonArray{} },
        { QStringLiteral("selection"),
          roundStage ? QJsonValue{ selection() } : QJsonValue::Null },
        { QStringLiteral("selectionRevision"), roundStage ? 4 : 0 },
        { QStringLiteral("availabilityRevision"), roundStage ? 5 : 0 },
        { QStringLiteral("liveStandings"), std::move(liveStandings) },
        { QStringLiteral("lastRoundResult"), std::move(lastResult) },
    };
    if (roundStage) {
        result.insert(QStringLiteral("round"),
                      frozenRound(*roundStage, twoPlayers));
    }
    return result;
}

auto
roomSnapshot(QString requestId, QJsonObject room) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("room_snapshot") },
      { QStringLiteral("requestId"), std::move(requestId) },
      { QStringLiteral("data"), std::move(room) },
    });
}

auto
loadingStarted(bool twoPlayers = false) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_loading_started") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("round"),
            frozenRound(QStringLiteral("probing"), twoPlayers) },
        } },
    });
}

auto
loadRequested(bool twoPlayers = false) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_load_requested") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("connectionGeneration"), 3 },
          { QStringLiteral("round"),
            frozenRound(QStringLiteral("loading"), twoPlayers) },
        } },
    });
}

auto
startScheduled(qint64 connectionGeneration = 3, qint64 startAfterMs = 250)
  -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_start_scheduled") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("connectionGeneration"), connectionGeneration },
          { QStringLiteral("roundId"), QStringLiteral("round-1") },
          { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
          { QStringLiteral("startAtServerMs"), 100'000 },
          { QStringLiteral("startAfterMs"), startAfterMs },
          { QStringLiteral("playDeadlineAtServerMs"), 400'000 },
        } },
    });
}

auto
roundStarted() -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_started") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("roundId"), QStringLiteral("round-1") },
          { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
          { QStringLiteral("playDeadlineAtServerMs"), 400'000 },
        } },
    });
}

auto
judgements(qint64 exScore, qint64 badPoorCount = 0) -> QJsonObject
{
    return {
        { QStringLiteral("perfect"), exScore / 2 },
        { QStringLiteral("great"), exScore % 2 },
        { QStringLiteral("good"), 0 },
        { QStringLiteral("bad"), badPoorCount },
        { QStringLiteral("poor"), 0 },
        { QStringLiteral("emptyPoor"), 0 },
    };
}

auto
telemetryJson(qint64 sequence, qint64 exScore) -> QJsonObject
{
    return {
        { QStringLiteral("sequence"), sequence },
        { QStringLiteral("exScore"), exScore },
        { QStringLiteral("progressPermille"), 500 },
        { QStringLiteral("maxCombo"), 42 },
        { QStringLiteral("badPoorCount"), 0 },
        { QStringLiteral("judgements"), judgements(exScore) },
        { QStringLiteral("gauge"),
          QJsonObject{ { QStringLiteral("type"), QStringLiteral("normal") },
                       { QStringLiteral("valueMilli"), 60'000 } } },
        { QStringLiteral("playStatus"), QStringLiteral("playing") },
    };
}

auto
activeStanding(QString memberId, int rank, qint64 sequence, qint64 exScore)
  -> QJsonObject
{
    return {
        { QStringLiteral("memberId"), std::move(memberId) },
        { QStringLiteral("connectionStatus"), QStringLiteral("connected") },
        { QStringLiteral("competitionState"), QStringLiteral("playing") },
        { QStringLiteral("rank"), rank },
        { QStringLiteral("telemetry"), telemetryJson(sequence, exScore) },
    };
}

auto
dnfStanding(QString memberId, QString reason) -> QJsonObject
{
    return {
        { QStringLiteral("memberId"), std::move(memberId) },
        { QStringLiteral("connectionStatus"), QStringLiteral("reserved") },
        { QStringLiteral("competitionState"), QStringLiteral("dnf") },
        { QStringLiteral("rank"), QJsonValue::Null },
        { QStringLiteral("dnfReason"), std::move(reason) },
    };
}

auto
standingsData(qint64 revision,
              qint64 selfScore = 100,
              qint64 opponentScore = 200,
              bool twoPlayers = true) -> QJsonObject
{
    auto entries = QJsonArray{ activeStanding(
      QStringLiteral("member-1"),
      !twoPlayers || selfScore > opponentScore ? 1 : 2,
      revision,
      selfScore) };
    if (twoPlayers) {
        entries.append(activeStanding(QStringLiteral("member-2"),
                                      opponentScore > selfScore ? 1 : 2,
                                      revision,
                                      opponentScore));
    }
    return {
        { QStringLiteral("roomId"), QStringLiteral("room-1") },
        { QStringLiteral("roomGeneration"), 2 },
        { QStringLiteral("roundId"), QStringLiteral("round-1") },
        { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
        { QStringLiteral("standingsRevision"), revision },
        { QStringLiteral("entries"), std::move(entries) },
    };
}

auto
standingsEvent(qint64 revision,
               qint64 selfScore = 100,
               qint64 opponentScore = 200) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_standings") },
      { QStringLiteral("data"),
        standingsData(revision, selfScore, opponentScore) },
    });
}

auto
finalResultJson(qint64 exScore = 100) -> QJsonObject
{
    return {
        { QStringLiteral("exScore"), exScore },
        { QStringLiteral("maxCombo"), 64 },
        { QStringLiteral("badPoorCount"), 0 },
        { QStringLiteral("judgements"), judgements(exScore) },
        { QStringLiteral("clearType"), QStringLiteral("normal") },
        { QStringLiteral("finalGauge"),
          QJsonObject{ { QStringLiteral("type"), QStringLiteral("normal") },
                       { QStringLiteral("valueMilli"), 75'000 } } },
    };
}

auto
finishedFinalStanding(QString memberId,
                      QString userId,
                      QString name,
                      int rank,
                      qint64 exScore,
                      qint64 wins) -> QJsonObject
{
    return {
        { QStringLiteral("memberId"), std::move(memberId) },
        { QStringLiteral("identity"),
          identity(std::move(userId), std::move(name)) },
        { QStringLiteral("lobbyWinsAfter"), wins },
        { QStringLiteral("competitionState"), QStringLiteral("finished") },
        { QStringLiteral("rank"), rank },
        { QStringLiteral("result"), finalResultJson(exScore) },
    };
}

auto
resultSnapshot(bool twoPlayers = false) -> QJsonObject
{
    auto entries = QJsonArray{ finishedFinalStanding(QStringLiteral("member-1"),
                                                     QStringLiteral("user-1"),
                                                     QStringLiteral("Alice"),
                                                     twoPlayers ? 2 : 1,
                                                     100,
                                                     0) };
    auto winners = QJsonArray{ QStringLiteral("member-1") };
    if (twoPlayers) {
        entries.append(finishedFinalStanding(QStringLiteral("member-2"),
                                             QStringLiteral("user-2"),
                                             QStringLiteral("Bob"),
                                             1,
                                             200,
                                             1));
        winners = QJsonArray{ QStringLiteral("member-2") };
    }
    return {
        { QStringLiteral("resultRevision"), 1 },
        { QStringLiteral("roundId"), QStringLiteral("round-1") },
        { QStringLiteral("selectionRevision"), 4 },
        { QStringLiteral("finalizedAtServerMs"), 500'000 },
        { QStringLiteral("participantCount"), twoPlayers ? 2 : 1 },
        { QStringLiteral("selection"), selection() },
        { QStringLiteral("winnerMemberIds"), std::move(winners) },
        { QStringLiteral("entries"), std::move(entries) },
    };
}

auto
terminalAccepted(QString requestId, QString terminal) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_terminal_accepted") },
      { QStringLiteral("requestId"), std::move(requestId) },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("roundId"), QStringLiteral("round-1") },
          { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
          { QStringLiteral("terminal"), std::move(terminal) },
        } },
    });
}

auto
commandError(QString requestId, QString code) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("command_error") },
      { QStringLiteral("requestId"), std::move(requestId) },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("code"), std::move(code) },
          { QStringLiteral("displayMessageKey"),
            QStringLiteral("arena.error.competitionStateStale") },
        } },
    });
}

auto
finalizedEvent(bool twoPlayers = false) -> QString
{
    auto members =
      QJsonArray{ member(QStringLiteral("member-1"), QStringLiteral("Alice")) };
    if (twoPlayers) {
        members.append(
          member(QStringLiteral("member-2"), QStringLiteral("Bob"), 1));
    }
    return compact({
      { QStringLiteral("type"), QStringLiteral("round_finalized") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 2 },
          { QStringLiteral("roundId"), QStringLiteral("round-1") },
          { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
          { QStringLiteral("result"), resultSnapshot(twoPlayers) },
          { QStringLiteral("members"), std::move(members) },
        } },
    });
}

auto
telemetryValue(quint32 sequence, qint64 exScore) -> arena::TelemetrySnapshot
{
    return {
        .sequence = sequence,
        .exScore = exScore,
        .progressPermille = 500,
        .maxCombo = 42,
        .badPoorCount = 0,
        .judgements = { .perfect = exScore / 2,
                        .great = exScore % 2,
                        .good = 0,
                        .bad = 0,
                        .poor = 0,
                        .emptyPoor = 0 },
        .gauge = { .type = arena::GaugeType::Normal, .valueMilli = 60'000 },
    };
}

auto
finalValue(qint64 exScore = 100) -> arena::FinalResult
{
    return {
        .exScore = exScore,
        .maxCombo = 64,
        .badPoorCount = 0,
        .judgements = { .perfect = exScore / 2,
                        .great = exScore % 2,
                        .good = 0,
                        .bad = 0,
                        .poor = 0,
                        .emptyPoor = 0 },
        .clearType = arena::ClearType::Normal,
        .finalGauge = { .type = arena::GaugeType::Normal,
                        .valueMilli = 75'000 },
    };
}

auto
messagesOfType(const arena::test::FakeArenaTransport& transport,
               QStringView type) -> QVector<QJsonObject>
{
    QVector<QJsonObject> result;
    for (const auto& call : transport.textCalls) {
        const auto object = messageObject(call.message);
        if (object.value(QStringLiteral("type")).toString() == type) {
            result.push_back(object);
        }
    }
    return result;
}

struct Fixture
{
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identityProvider;
    arena::test::FakeArenaScheduler scheduler;
    arena::test::FakeArenaInventorySource inventorySource;
    arena::test::FakeArenaRoundLoader roundLoader;
    arena::test::FakeArenaGameplaySource gameplaySource;
    arena::ArenaSession session;

    explicit Fixture(bool injectGameplaySource = true)
      : session(&transport,
                &identityProvider,
                &scheduler,
                QUrl(QStringLiteral("ws://127.0.0.1:3001/ws")),
                QStringLiteral("2026.7.10"),
                &inventorySource,
                &roundLoader,
                injectGameplaySource ? &gameplaySource : nullptr)
    {
    }

    void browse()
    {
        session.connectForBrowsing();
        REQUIRE(transport.connectCalls.size() == 1);
        transport.injectConnected(1);
        transport.injectText(1, competitionHello(false));
        REQUIRE(session.getState() == arena::ArenaSession::State::Browsing);
        REQUIRE(session.competitionAvailable());
    }

    void enterRoom(bool twoPlayers = false)
    {
        browse();
        identityProvider.setLoggedIn(true);
        session.createRoom(QStringLiteral("Competition room"), QString{});
        REQUIRE(identityProvider.ticketRequests.size() == 1);
        identityProvider.succeedTicket(identityProvider.ticketRequests.back(),
                                       QStringLiteral("ticket"));
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        transport.injectText(generation, competitionHello(true));
        const auto creates = messagesOfType(transport, u"room_create");
        REQUIRE(creates.size() == 1);
        const auto requestId =
          creates.front().value(QStringLiteral("requestId")).toString();
        transport.injectText(
          generation,
          roomSnapshot(
            requestId,
            roomData(QStringLiteral("selecting"), std::nullopt, twoPlayers)));
        REQUIRE(session.getState() == arena::ArenaSession::State::InRoom);
    }

    void loadRound(gameplay_logic::ChartRunner* runner, bool twoPlayers = false)
    {
        const auto generation = transport.connectCalls.back().generation;
        transport.injectText(generation, loadingStarted(twoPlayers));
        transport.injectText(generation, loadRequested(twoPlayers));
        REQUIRE(roundLoader.loads.size() == 1);
        emit roundLoader.loadFinished(roundLoader.loads.front().first, runner);
        REQUIRE(messagesOfType(transport, u"round_load_result").size() == 1);
    }

    void startRound(qint64 startAfterMs = 250)
    {
        const auto generation = transport.connectCalls.back().generation;
        transport.injectText(generation, startScheduled(3, startAfterMs));
        scheduler.advanceBy(startAfterMs);
        transport.injectText(generation, roundStarted());
        REQUIRE(session.getRoomPhase() == arena::RoomPhase::Playing);
        REQUIRE(session.arenaGameplayActive());
    }

    auto reconnectWithPlayingRoom(bool twoPlayers = false,
                                  QJsonValue standings = QJsonValue::Null)
      -> arena::ArenaTransport::Generation
    {
        const auto oldGeneration = transport.connectCalls.back().generation;
        transport.injectDisconnected(oldGeneration);
        REQUIRE(session.getState() == arena::ArenaSession::State::Reconnecting);
        REQUIRE_FALSE(identityProvider.ticketRequests.isEmpty());
        identityProvider.succeedTicket(identityProvider.ticketRequests.back(),
                                       QStringLiteral("resume-ticket"));
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        if (standings.isNull()) {
            standings = standingsData(1, 100, 200, twoPlayers);
        }
        transport.injectText(
          generation,
          competitionHello(true,
                           roomData(QStringLiteral("playing"),
                                    QStringLiteral("playing"),
                                    twoPlayers,
                                    4,
                                    std::move(standings))));
        REQUIRE(session.getState() == arena::ArenaSession::State::InRoom);
        return generation;
    }
};

} // namespace

TEST_CASE("ArenaSessionCompetition: negotiates competition and permits one "
          "legacy browse fallback",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();

    SECTION("current hello and admission")
    {
        Fixture fixture;
        fixture.session.connectForBrowsing();
        fixture.transport.injectConnected(1);
        const auto hello =
          messagesOfType(fixture.transport, u"client_hello").front();
        const auto data = hello.value(QStringLiteral("data")).toObject();
        CHECK(data.value(QStringLiteral("protocolMinor")).toInt() == 2);
        CHECK(data.value(QStringLiteral("capabilities")).toArray() ==
              QJsonArray{ QStringLiteral("rooms-v1"),
                          QStringLiteral("rounds-v1"),
                          QStringLiteral("competition-v1") });
        fixture.transport.injectText(1, competitionHello(false));
        CHECK(fixture.session.getRoundsAvailable());
        CHECK(fixture.session.competitionAvailable());
    }

    SECTION("negotiated competition rejects a downgraded room snapshot")
    {
        Fixture fixture;
        fixture.browse();
        fixture.identityProvider.setLoggedIn(true);
        fixture.session.createRoom(QStringLiteral("Competition room"),
                                   QString{});
        fixture.identityProvider.succeedTicket(
          fixture.identityProvider.ticketRequests.back(),
          QStringLiteral("ticket"));
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);
        fixture.transport.injectText(generation, competitionHello(true));
        const auto create =
          messagesOfType(fixture.transport, u"room_create").back();
        auto downgraded = roomData();
        downgraded.remove(QStringLiteral("liveStandings"));
        downgraded.remove(QStringLiteral("lastRoundResult"));
        fixture.transport.injectText(
          generation,
          roomSnapshot(create.value(QStringLiteral("requestId")).toString(),
                       std::move(downgraded)));

        CHECK(fixture.session.getState() == arena::ArenaSession::State::Error);
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("malformed_message"));
    }

    SECTION("negotiated competition rejects a downgraded frozen round")
    {
        Fixture fixture;
        fixture.enterRoom();
        auto loading = messageObject(loadingStarted());
        auto data = loading.value(QStringLiteral("data")).toObject();
        auto round = data.value(QStringLiteral("round")).toObject();
        auto participants =
          round.value(QStringLiteral("participants")).toArray();
        auto participant = participants.at(0).toObject();
        participant.remove(QStringLiteral("identity"));
        participants[0] = participant;
        round.insert(QStringLiteral("participants"), participants);
        data.insert(QStringLiteral("round"), round);
        loading.insert(QStringLiteral("data"), data);
        fixture.transport.injectText(
          fixture.transport.connectCalls.back().generation,
          compact(std::move(loading)));

        CHECK(fixture.session.getState() == arena::ArenaSession::State::Error);
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("malformed_message"));
    }

    SECTION("one pre-auth legacy retry is browse only")
    {
        Fixture fixture;
        fixture.session.connectForBrowsing();
        fixture.transport.injectConnected(1);
        const auto incompatible = compact({
          { QStringLiteral("type"), QStringLiteral("fatal_error") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("code"),
                QStringLiteral("protocol_incompatible") },
              { QStringLiteral("displayMessageKey"),
                QStringLiteral("arena.error.protocolIncompatible") },
            } },
        });
        fixture.transport.injectText(1, incompatible);
        REQUIRE(fixture.transport.connectCalls.size() == 2);
        const auto fallbackGeneration =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(fallbackGeneration);
        const auto fallback =
          messagesOfType(fixture.transport, u"client_hello").back();
        const auto fallbackData =
          fallback.value(QStringLiteral("data")).toObject();
        CHECK(fallbackData.value(QStringLiteral("protocolMinor")).toInt() == 0);
        CHECK(fallbackData.value(QStringLiteral("capabilities")).toArray() ==
              QJsonArray{ QStringLiteral("rooms-v1") });
        fixture.transport.injectText(fallbackGeneration, legacyHello());
        CHECK_FALSE(fixture.session.competitionAvailable());
        fixture.identityProvider.setLoggedIn(true);
        fixture.session.createRoom(QStringLiteral("blocked"), QString{});
        CHECK(fixture.identityProvider.ticketRequests.isEmpty());
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("competition_capability_required"));

        const auto connectionCount = fixture.transport.connectCalls.size();
        fixture.transport.injectText(fallbackGeneration, incompatible);
        CHECK(fixture.transport.connectCalls.size() == connectionCount);
        CHECK(fixture.session.getState() == arena::ArenaSession::State::Error);
    }
}

TEST_CASE("ArenaSessionCompetition: reports exact length and attaches before "
          "synchronized start",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner(1'000'001ns);
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.loadRound(runner.runner.get());

    const auto load =
      messagesOfType(fixture.transport, u"round_load_result").back();
    const auto loadData = load.value(QStringLiteral("data")).toObject();
    CHECK(loadData.value(QStringLiteral("chartLengthMs")).toInteger() == 2);

    int startedSignals = 0;
    QObject::connect(
      &fixture.session,
      &arena::ArenaSession::roundRunnerStarted,
      [&](const QString& roundId, gameplay_logic::ChartRunner* value) {
          CHECK(fixture.gameplaySource.attachCount == 1);
          CHECK(fixture.session.arenaRunner() == runner.runner.get());
          CHECK(fixture.session.arenaGameplayActive());
          CHECK(roundId == QStringLiteral("round-1"));
          CHECK(value == runner.runner.get());
          ++startedSignals;
      });

    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectText(generation, startScheduled(99, 250));
    fixture.scheduler.advanceBy(250);
    CHECK(startedSignals == 0);
    fixture.transport.injectText(generation, startScheduled(3, 250));
    fixture.scheduler.advanceBy(249);
    CHECK(startedSignals == 0);
    fixture.scheduler.advanceBy(1);
    fixture.transport.injectText(generation, roundStarted());

    CHECK(startedSignals == 1);
    CHECK(fixture.gameplaySource.attachedRunner == runner.runner.get());
    CHECK(fixture.session.arenaRunner() == runner.runner.get());
    CHECK(fixture.session.arenaOptionsSummary() ==
          QStringLiteral("P1 Random | P2 Mirror | DP Off"));
    CHECK_FALSE(fixture.session.arenaOptionsSummary().contains(
      QStringLiteral("0123456789abcdef")));
}

TEST_CASE("ArenaSessionCompetition: missing gameplay source fails closed "
          "before exposing the runner",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture{ false };
    fixture.enterRoom();
    fixture.loadRound(runner.runner.get());

    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto connectionCount = fixture.transport.connectCalls.size();
    fixture.transport.injectText(generation, startScheduled());

    CHECK(fixture.session.arenaRunner() == nullptr);
    CHECK_FALSE(fixture.session.arenaGameplayActive());
    CHECK(runner.runner->getStatus() == gameplay_logic::ChartRunner::Ready);
    CHECK(messagesOfType(fixture.transport, u"round_abandon").isEmpty());
    fixture.transport.injectText(generation, roundStarted());
    const auto abandons = messagesOfType(fixture.transport, u"round_abandon");
    REQUIRE(abandons.size() == 1);
    CHECK(fixture.transport.connectCalls.size() == connectionCount);
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getErrorCode().isEmpty());
    CHECK(abandons.front()
            .value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("reason"))
            .toString() == QStringLiteral("result_unavailable"));
}

TEST_CASE("ArenaSessionCompetition: samples at anchored 200 ms cadence and "
          "repairs standings gaps",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom(true);
    int calls = 0;
    fixture.gameplaySource.sampleHandler =
      [&](
        quint32 sequence) -> std::expected<arena::TelemetrySnapshot,
                                           arena::ArenaGameplayCaptureFailure> {
        ++calls;
        if (calls == 1) {
            return std::unexpected(
              arena::ArenaGameplayCaptureFailure::InvalidNumber);
        }
        return telemetryValue(sequence, calls * 10);
    };
    fixture.loadRound(runner.runner.get(), true);
    fixture.startRound();

    fixture.scheduler.advanceBy(199);
    CHECK(fixture.gameplaySource.sampledSequences.isEmpty());
    fixture.scheduler.advanceBy(1);
    REQUIRE(fixture.gameplaySource.sampledSequences == QVector<quint32>{ 1 });
    CHECK(messagesOfType(fixture.transport, u"round_telemetry").isEmpty());
    fixture.scheduler.advanceBy(200);
    CHECK(fixture.gameplaySource.sampledSequences == QVector<quint32>{ 1, 1 });
    auto telemetryMessages =
      messagesOfType(fixture.transport, u"round_telemetry");
    REQUIRE(telemetryMessages.size() == 1);
    CHECK(telemetryMessages.back()
            .value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("telemetry"))
            .toObject()
            .value(QStringLiteral("sequence"))
            .toInteger() == 1);

    fixture.scheduler.setNowWithoutRunningTasks(1'050);
    REQUIRE(fixture.scheduler.runNextAtCurrentTime());
    CHECK(fixture.gameplaySource.sampledSequences ==
          QVector<quint32>{ 1, 1, 2 });
    fixture.scheduler.advanceBy(199);
    CHECK(fixture.gameplaySource.sampledSequences.size() == 3);
    fixture.scheduler.advanceBy(1);
    CHECK(fixture.gameplaySource.sampledSequences.back() == 3);

    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectText(generation, standingsEvent(1, 100, 200));
    REQUIRE(fixture.session.liveStandings()->rowCount() == 2);
    CHECK(fixture.session.opponentTarget()->available());
    CHECK(fixture.session.opponentTarget()->memberId() ==
          QStringLiteral("member-2"));
    CHECK(fixture.session.opponentTarget()->exScore() == 200);
    fixture.transport.injectText(generation, standingsEvent(3, 300, 250));
    CHECK(fixture.session.liveStandings()->revision() == 3);
    CHECK(fixture.session.opponentTarget()->exScore() == 250);
    fixture.transport.injectText(generation, standingsEvent(2, 100, 999));
    CHECK(fixture.session.liveStandings()->revision() == 3);
    CHECK(fixture.session.opponentTarget()->exScore() == 250);
}

TEST_CASE("ArenaSessionCompetition: coalesces disconnected telemetry and "
          "flushes newest on resume",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.gameplaySource.sampleHandler = [](quint32 sequence) {
        return std::expected<arena::TelemetrySnapshot,
                             arena::ArenaGameplayCaptureFailure>{
            telemetryValue(sequence, sequence * 10)
        };
    };
    fixture.loadRound(runner.runner.get());
    fixture.startRound();
    fixture.scheduler.advanceBy(200);
    REQUIRE(messagesOfType(fixture.transport, u"round_telemetry").size() == 1);

    const auto oldGeneration = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectDisconnected(oldGeneration);
    REQUIRE(fixture.session.getState() ==
            arena::ArenaSession::State::Reconnecting);
    fixture.scheduler.advanceBy(600);
    CHECK(fixture.gameplaySource.sampledSequences ==
          QVector<quint32>{ 1, 2, 3, 4 });
    CHECK(messagesOfType(fixture.transport, u"round_telemetry").size() == 1);

    const auto resumedGeneration =
      fixture.reconnectWithPlayingRoom(false, standingsData(1, 10, 0, false));
    const auto messages = messagesOfType(fixture.transport, u"round_telemetry");
    REQUIRE(messages.size() == 2);
    const auto resumed =
      messages.back().value(QStringLiteral("data")).toObject();
    CHECK(resumed.value(QStringLiteral("connectionGeneration")).toInteger() ==
          4);
    CHECK(resumed.value(QStringLiteral("telemetry"))
            .toObject()
            .value(QStringLiteral("sequence"))
            .toInteger() == 4);
    CHECK(fixture.transport.textCalls.back().generation == resumedGeneration);
    CHECK(fixture.gameplaySource.attachCount == 1);

    fixture.scheduler.advanceBy(200);
    const auto continued =
      messagesOfType(fixture.transport, u"round_telemetry");
    REQUIRE(continued.size() == 3);
    CHECK(continued.back()
            .value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("telemetry"))
            .toObject()
            .value(QStringLiteral("sequence"))
            .toInteger() == 5);
}

TEST_CASE("ArenaSessionCompetition: retries one immutable terminal and "
          "finalizes presentation",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.gameplaySource.sampleHandler = [](quint32 sequence) {
        return std::expected<arena::TelemetrySnapshot,
                             arena::ArenaGameplayCaptureFailure>{
            telemetryValue(sequence, 50)
        };
    };
    fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
        return std::expected<arena::FinalResult,
                             arena::ArenaGameplayCaptureFailure>{ finalValue(
          100) };
    };
    fixture.loadRound(runner.runner.get());
    fixture.startRound();
    fixture.scheduler.advanceBy(200);
    const auto samplesAtFinish = fixture.gameplaySource.sampledSequences.size();
    const auto scores = runner.runner->finish();
    REQUIRE(scores.size() == 1);
    auto score = std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
    CHECK_FALSE(fixture.session.arenaGameplayActive());
    CHECK(fixture.gameplaySource.detachCount == 0);
    fixture.scheduler.advanceBy(600);
    CHECK(fixture.gameplaySource.sampledSequences.size() == samplesAtFinish);

    auto wrong = makeScore(QStringLiteral("wrong-guid"));
    CHECK_FALSE(fixture.session.submitLocalResult(wrong.get()));
    CHECK(fixture.gameplaySource.capturedScores.isEmpty());
    CHECK(messagesOfType(fixture.transport, u"round_result_submit").isEmpty());

    REQUIRE(fixture.session.submitLocalResult(score.get()));
    REQUIRE(fixture.gameplaySource.capturedScores.size() == 1);
    CHECK(fixture.gameplaySource.detachCount == 1);
    CHECK(fixture.gameplaySource.attachedRunner == nullptr);
    CHECK(fixture.session.arenaRunner() == nullptr);
    REQUIRE(fixture.session.resultPresentationActive());
    CHECK(fixture.session.presentedResult()->valid());
    CHECK_FALSE(fixture.session.presentedResult()->finalized());
    CHECK(fixture.session.presentedResult()->selectionTitle() ==
          QStringLiteral("Competition chart"));
    CHECK(fixture.session.presentedResult()->selectionOptionsSummary() ==
          fixture.session.arenaOptionsSummary());
    auto submissions =
      messagesOfType(fixture.transport, u"round_result_submit");
    REQUIRE(submissions.size() == 1);
    const auto original = submissions.front();
    const auto requestId =
      original.value(QStringLiteral("requestId")).toString();

    fixture.reconnectWithPlayingRoom(false, standingsData(1, 50, 0, false));
    submissions = messagesOfType(fixture.transport, u"round_result_submit");
    REQUIRE(submissions.size() == 2);
    auto retried = submissions.back();
    CHECK(retried.value(QStringLiteral("requestId")) ==
          original.value(QStringLiteral("requestId")));
    auto originalData = original.value(QStringLiteral("data")).toObject();
    auto retriedData = retried.value(QStringLiteral("data")).toObject();
    CHECK(originalData.take(QStringLiteral("connectionGeneration")) !=
          retriedData.take(QStringLiteral("connectionGeneration")));
    CHECK(originalData == retriedData);

    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectText(
      generation, terminalAccepted(requestId, QStringLiteral("finished")));
    REQUIRE(fixture.session.submitLocalResult(score.get()));
    fixture.session.abandonCurrentRound();
    CHECK(messagesOfType(fixture.transport, u"round_result_submit").size() ==
          2);
    CHECK(messagesOfType(fixture.transport, u"round_abandon").isEmpty());
    fixture.transport.injectText(generation, finalizedEvent());
    CHECK(fixture.session.getRoomPhase() == arena::RoomPhase::Selecting);
    CHECK(fixture.session.getCurrentRoundId().isEmpty());
    CHECK(fixture.session.lastResult()->finalized());
    CHECK(fixture.session.lastResult()->localWinner());
    CHECK(fixture.session.presentedResult()->finalized());
    CHECK(fixture.session.presentedResult()->localRank() == 1);
    CHECK(fixture.session.liveStandings()->rowCount() == 1);
    CHECK(fixture.session.arenaRunner() == nullptr);
    CHECK(fixture.gameplaySource.detachCount == 1);

    fixture.session.endResultPresentation(QStringLiteral("wrong"));
    CHECK(fixture.session.resultPresentationActive());
    fixture.session.endResultPresentation(QStringLiteral("round-1"));
    CHECK_FALSE(fixture.session.resultPresentationActive());
    CHECK_FALSE(fixture.session.presentedResult()->valid());
    CHECK(fixture.session.lastResult()->valid());
}

TEST_CASE("ArenaSessionCompetition: resume preserves a result presentation "
          "finalized while disconnected",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
        return std::expected<arena::FinalResult,
                             arena::ArenaGameplayCaptureFailure>{ finalValue(
          100) };
    };
    fixture.loadRound(runner.runner.get());
    fixture.startRound();
    const auto scores = runner.runner->finish();
    REQUIRE(scores.size() == 1);
    auto score = std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
    REQUIRE(fixture.session.submitLocalResult(score.get()));
    REQUIRE(fixture.session.resultPresentationActive());
    REQUIRE(fixture.session.presentedResult()->valid());
    CHECK_FALSE(fixture.session.presentedResult()->finalized());

    const auto oldGeneration = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectDisconnected(oldGeneration);
    REQUIRE(fixture.session.getState() ==
            arena::ArenaSession::State::Reconnecting);
    fixture.identityProvider.succeedTicket(
      fixture.identityProvider.ticketRequests.back(),
      QStringLiteral("resume-ticket"));
    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectConnected(generation);
    fixture.transport.injectText(
      generation,
      competitionHello(true,
                       roomData(QStringLiteral("selecting"),
                                std::nullopt,
                                false,
                                4,
                                QJsonValue::Null,
                                resultSnapshot())));

    REQUIRE(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.resultPresentationActive());
    REQUIRE(fixture.session.presentedResult()->valid());
    CHECK(fixture.session.presentedResult()->finalized());
    CHECK(fixture.session.presentedResult()->roundId() ==
          QStringLiteral("round-1"));
    CHECK(fixture.session.presentedResult()->localRank() == 1);
    CHECK(fixture.session.lastResult()->finalized());
    CHECK(fixture.session.getCurrentRoundId().isEmpty());
}

TEST_CASE("ArenaSessionCompetition: profile cleanup during final model reset "
          "cannot repopulate results",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
        return std::expected<arena::FinalResult,
                             arena::ArenaGameplayCaptureFailure>{ finalValue(
          100) };
    };
    fixture.loadRound(runner.runner.get());
    fixture.startRound();
    const auto scores = runner.runner->finish();
    REQUIRE(scores.size() == 1);
    auto score = std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
    REQUIRE(fixture.session.submitLocalResult(score.get()));

    auto changedProfile = false;
    QObject::connect(fixture.session.lastResult()->standings(),
                     &QAbstractItemModel::modelReset,
                     &fixture.session,
                     [&] {
                         if (!changedProfile) {
                             changedProfile = true;
                             fixture.identityProvider.replaceActiveProfile(
                               true,
                               arena::PublicIdentity{
                                 .userId = QStringLiteral("replacement-user"),
                                 .displayName = QStringLiteral("Replacement"),
                               });
                         }
                     });
    fixture.transport.injectText(
      fixture.transport.connectCalls.back().generation, finalizedEvent());

    REQUIRE(changedProfile);
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK_FALSE(fixture.session.lastResult()->valid());
    CHECK_FALSE(fixture.session.presentedResult()->valid());
    CHECK_FALSE(fixture.session.resultPresentationActive());
}

TEST_CASE(
  "ArenaSessionCompetition: submits capture failure and abort DNF exactly once",
  "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();

    SECTION("capture failure becomes result unavailable")
    {
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        Fixture fixture;
        fixture.enterRoom();
        fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*)
          -> std::expected<arena::FinalResult,
                           arena::ArenaGameplayCaptureFailure> {
            return std::unexpected(
              arena::ArenaGameplayCaptureFailure::InvalidResult);
        };
        fixture.loadRound(runner.runner.get());
        fixture.startRound();
        const auto scores = runner.runner->finish();
        REQUIRE(scores.size() == 1);
        auto score =
          std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
        REQUIRE(fixture.session.submitLocalResult(score.get()));
        CHECK(fixture.gameplaySource.detachCount == 1);
        CHECK(fixture.gameplaySource.attachedRunner == nullptr);
        CHECK(fixture.session.arenaRunner() == nullptr);
        const auto abandons =
          messagesOfType(fixture.transport, u"round_abandon");
        REQUIRE(abandons.size() == 1);
        CHECK(abandons.front()
                .value(QStringLiteral("data"))
                .toObject()
                .value(QStringLiteral("reason"))
                .toString() == QStringLiteral("result_unavailable"));
        CHECK(fixture.session.presentedResult()->localDnf());
        CHECK_FALSE(fixture.session.getErrorCode().contains(
          QStringLiteral("arena-score-guid")));
    }

    SECTION("local abort is idempotent and ignores its partial score")
    {
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        auto score = makeScore();
        Fixture fixture;
        fixture.enterRoom();
        fixture.loadRound(runner.runner.get());
        fixture.startRound();
        fixture.session.setGameplayChatOpen(true);
        REQUIRE(fixture.session.gameplayChatOpen());
        fixture.session.abandonCurrentRound();
        fixture.session.abandonCurrentRound();
        CHECK(fixture.gameplaySource.detachCount == 1);
        CHECK_FALSE(fixture.session.arenaGameplayActive());
        CHECK_FALSE(fixture.session.gameplayChatOpen());
        const auto abandons =
          messagesOfType(fixture.transport, u"round_abandon");
        REQUIRE(abandons.size() == 1);
        CHECK(abandons.front()
                .value(QStringLiteral("data"))
                .toObject()
                .value(QStringLiteral("reason"))
                .toString() == QStringLiteral("aborted"));
        fixture.reconnectWithPlayingRoom(false, standingsData(1, 0, 0, false));
        CHECK(messagesOfType(fixture.transport, u"round_abandon").size() == 2);
        const auto retriedAbandon =
          messagesOfType(fixture.transport, u"round_abandon").back();
        fixture.transport.injectText(
          fixture.transport.connectCalls.back().generation,
          terminalAccepted(
            retriedAbandon.value(QStringLiteral("requestId")).toString(),
            QStringLiteral("dnf")));
        fixture.session.abandonCurrentRound();
        CHECK(messagesOfType(fixture.transport, u"round_abandon").size() == 2);
        REQUIRE(fixture.session.submitLocalResult(score.get()));
        CHECK(fixture.gameplaySource.capturedScores.isEmpty());
        CHECK(fixture.session.presentedResult()->localDnf());

        auto wrong = makeScore(QStringLiteral("wrong-guid"));
        CHECK_FALSE(fixture.session.submitLocalResult(wrong.get()));
        CHECK(messagesOfType(fixture.transport, u"round_abandon").size() == 2);
    }
}

TEST_CASE("ArenaSessionCompetition: authoritative resume clears observed "
          "terminal and updates models",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom(true);
    fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
        return std::expected<arena::FinalResult,
                             arena::ArenaGameplayCaptureFailure>{ finalValue(
          100) };
    };
    fixture.loadRound(runner.runner.get(), true);
    fixture.startRound();
    const auto scores = runner.runner->finish();
    REQUIRE(scores.size() == 1);
    auto score = std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
    REQUIRE(fixture.session.submitLocalResult(score.get()));
    REQUIRE(messagesOfType(fixture.transport, u"round_result_submit").size() ==
            1);

    auto observed = standingsData(5, 100, 200);
    auto entries = observed.value(QStringLiteral("entries")).toArray();
    entries[0] = QJsonObject{
        { QStringLiteral("memberId"), QStringLiteral("member-1") },
        { QStringLiteral("connectionStatus"), QStringLiteral("connected") },
        { QStringLiteral("competitionState"), QStringLiteral("finished") },
        { QStringLiteral("rank"), 2 },
        { QStringLiteral("result"), finalResultJson(100) },
    };
    observed.insert(QStringLiteral("entries"), entries);
    fixture.reconnectWithPlayingRoom(true, observed);
    CHECK(messagesOfType(fixture.transport, u"round_result_submit").size() ==
          1);
    REQUIRE(fixture.session.submitLocalResult(score.get()));
    fixture.session.abandonCurrentRound();
    CHECK(messagesOfType(fixture.transport, u"round_result_submit").size() ==
          1);
    CHECK(messagesOfType(fixture.transport, u"round_abandon").isEmpty());
    CHECK(fixture.session.liveStandings()->revision() == 5);
    CHECK(fixture.session.liveStandings()->rowCount() == 2);
    CHECK(fixture.session.opponentTarget()->memberId() ==
          QStringLiteral("member-2"));
}

TEST_CASE("ArenaSessionCompetition: authoritative DNF prevents a conflicting "
          "local terminal",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    auto runner = makeReadyRunner();
    runner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();
    fixture.loadRound(runner.runner.get());
    fixture.startRound();

    auto observed = standingsData(2, 0, 0, false);
    observed.insert(QStringLiteral("entries"),
                    QJsonArray{ dnfStanding(QStringLiteral("member-1"),
                                            QStringLiteral("play_deadline")) });
    fixture.transport.injectText(
      fixture.transport.connectCalls.back().generation,
      compact({ { QStringLiteral("type"), QStringLiteral("round_standings") },
                { QStringLiteral("data"), std::move(observed) } }));

    fixture.session.abandonCurrentRound();
    CHECK(messagesOfType(fixture.transport, u"round_abandon").isEmpty());
    const auto scores = runner.runner->finish();
    REQUIRE(scores.size() == 1);
    auto score = std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
    REQUIRE(fixture.session.submitLocalResult(score.get()));
    CHECK(fixture.gameplaySource.capturedScores.isEmpty());
    CHECK(fixture.gameplaySource.detachCount == 1);
    CHECK(fixture.session.presentedResult()->localDnf());
    CHECK(messagesOfType(fixture.transport, u"round_result_submit").isEmpty());
}

TEST_CASE("ArenaSessionCompetition: stale terminal errors resume for an "
          "authoritative snapshot",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();
    const auto staleCodes =
      QStringList{ QStringLiteral("round_stale"),
                   QStringLiteral("launch_stage_stale"),
                   QStringLiteral("round_already_terminal") };
    for (const auto& code : staleCodes) {
        CAPTURE(code);
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        Fixture fixture;
        fixture.enterRoom();
        fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
            return std::expected<arena::FinalResult,
                                 arena::ArenaGameplayCaptureFailure>{
                finalValue(100)
            };
        };
        fixture.loadRound(runner.runner.get());
        fixture.startRound();
        const auto scores = runner.runner->finish();
        REQUIRE(scores.size() == 1);
        auto score =
          std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
        REQUIRE(fixture.session.submitLocalResult(score.get()));
        const auto submitted =
          messagesOfType(fixture.transport, u"round_result_submit").back();
        fixture.transport.injectText(
          fixture.transport.connectCalls.back().generation,
          commandError(submitted.value(QStringLiteral("requestId")).toString(),
                       code));
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::Reconnecting);

        fixture.identityProvider.succeedTicket(
          fixture.identityProvider.ticketRequests.back(),
          QStringLiteral("resume-ticket"));
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);
        fixture.transport.injectText(
          generation,
          competitionHello(true,
                           roomData(QStringLiteral("playing"),
                                    QStringLiteral("playing"),
                                    false,
                                    4,
                                    standingsData(1, 100, 0, false))));
        CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
        CHECK(
          messagesOfType(fixture.transport, u"round_result_submit").size() ==
          2);
    }
}

TEST_CASE("ArenaSessionCompetition: cleanup paths synchronously clear "
          "transient competition state",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();

    auto exercise = [](auto trigger) {
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        Fixture fixture;
        fixture.enterRoom(true);
        fixture.gameplaySource.sampleHandler = [](quint32 sequence) {
            return std::expected<arena::TelemetrySnapshot,
                                 arena::ArenaGameplayCaptureFailure>{
                telemetryValue(sequence, 10)
            };
        };
        fixture.loadRound(runner.runner.get(), true);
        fixture.startRound();
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectText(generation, standingsEvent(1));
        fixture.session.setGameplayChatOpen(true);
        const auto samples = fixture.gameplaySource.sampledSequences.size();

        trigger(fixture, generation);

        CHECK(fixture.session.arenaRunner() == nullptr);
        CHECK_FALSE(fixture.session.arenaGameplayActive());
        CHECK_FALSE(fixture.session.gameplayChatOpen());
        CHECK_FALSE(fixture.session.resultPresentationActive());
        CHECK_FALSE(fixture.session.presentedResult()->valid());
        CHECK(fixture.session.liveStandings()->rowCount() == 0);
        CHECK_FALSE(fixture.session.opponentTarget()->available());
        CHECK(fixture.gameplaySource.detachCount >= 1);
        fixture.scheduler.advanceBy(1'000);
        CHECK(fixture.gameplaySource.sampledSequences.size() == samples);
    };

    SECTION("active profile change")
    {
        exercise([](Fixture& fixture, auto) {
            fixture.identityProvider.replaceActiveProfile(
              true,
              arena::PublicIdentity{
                .userId = QStringLiteral("replacement-user"),
                .displayName = QStringLiteral("Replacement"),
              });
        });
    }
    SECTION("self kicked")
    {
        exercise([](Fixture& fixture, auto generation) {
            fixture.transport.injectText(
              generation,
              compact({
                { QStringLiteral("type"), QStringLiteral("room_member_left") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("roomId"), QStringLiteral("room-1") },
                    { QStringLiteral("roomGeneration"), 2 },
                    { QStringLiteral("memberId"), QStringLiteral("member-1") },
                    { QStringLiteral("reason"), QStringLiteral("kicked") },
                  } },
              }));
        });
    }
    SECTION("exit Arena")
    {
        exercise([](Fixture& fixture, auto) { fixture.session.exitArena(); });
    }

    SECTION("pending result presentation")
    {
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        Fixture fixture;
        fixture.enterRoom();
        fixture.gameplaySource.finalHandler = [](gameplay_logic::BmsScore*) {
            return std::expected<arena::FinalResult,
                                 arena::ArenaGameplayCaptureFailure>{
                finalValue(100)
            };
        };
        fixture.loadRound(runner.runner.get());
        fixture.startRound();
        const auto scores = runner.runner->finish();
        REQUIRE(scores.size() == 1);
        auto score =
          std::unique_ptr<gameplay_logic::BmsScore>{ scores.front() };
        REQUIRE(fixture.session.submitLocalResult(score.get()));
        REQUIRE(fixture.session.resultPresentationActive());

        fixture.identityProvider.replaceActiveProfile(
          true,
          arena::PublicIdentity{
            .userId = QStringLiteral("replacement-user"),
            .displayName = QStringLiteral("Replacement"),
          });

        CHECK_FALSE(fixture.session.resultPresentationActive());
        CHECK_FALSE(fixture.session.presentedResult()->valid());
    }
}

TEST_CASE("ArenaOverlayCustomization: Session gates only its active Arena "
          "runner and chat exits the gate",
          "[arena][ArenaOverlayCustomization]")
{
    ensureCoreApplication();

    auto ordinaryRunner = makeReadyRunner();
    ordinaryRunner.runner->start();
    REQUIRE(ordinaryRunner.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);

    auto arenaRunner = makeReadyRunner();
    arenaRunner.runner->holdStart();
    Fixture fixture;
    fixture.enterRoom();

    fixture.session.setOverlayCustomizationActive(true);
    CHECK_FALSE(fixture.session.overlayCustomizationActive());
    CHECK_FALSE(ordinaryRunner.runner->inputSuppressed());

    fixture.loadRound(arenaRunner.runner.get());
    fixture.startRound();
    REQUIRE(fixture.session.arenaRunner() == arenaRunner.runner.get());
    REQUIRE(arenaRunner.runner->getStatus() ==
            gameplay_logic::ChartRunner::Running);

    auto changes = 0;
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::overlayCustomizationActiveChanged,
                     [&changes] { ++changes; });

    fixture.session.setOverlayCustomizationActive(true);
    CHECK(fixture.session.overlayCustomizationActive());
    CHECK(arenaRunner.runner->inputSuppressed());
    CHECK_FALSE(ordinaryRunner.runner->inputSuppressed());
    CHECK(changes == 1);

    fixture.session.setOverlayCustomizationActive(true);
    CHECK(changes == 1);

    fixture.session.setGameplayChatOpen(true);
    CHECK(fixture.session.gameplayChatOpen());
    CHECK_FALSE(fixture.session.overlayCustomizationActive());
    CHECK_FALSE(arenaRunner.runner->inputSuppressed());
    CHECK(changes == 2);

    fixture.session.setOverlayCustomizationActive(false);
    CHECK(changes == 2);
}

TEST_CASE("ArenaOverlayCustomization: Session restores input on runner and "
          "lifecycle cleanup",
          "[arena][ArenaOverlayCustomization]")
{
    ensureCoreApplication();

    auto exercise = [](auto trigger) {
        auto runner = makeReadyRunner();
        runner.runner->holdStart();
        Fixture fixture;
        fixture.enterRoom();
        fixture.loadRound(runner.runner.get());
        fixture.startRound();
        fixture.session.setOverlayCustomizationActive(true);
        REQUIRE(fixture.session.overlayCustomizationActive());
        REQUIRE(runner.runner->inputSuppressed());

        trigger(fixture, runner);

        CHECK_FALSE(fixture.session.overlayCustomizationActive());
        if (runner.runner) {
            CHECK_FALSE(runner.runner->inputSuppressed());
        }
    };

    SECTION("round finish")
    {
        exercise([](Fixture&, RunnerFixture& runner) {
            const auto scores = runner.runner->finish();
            for (auto* score : scores) {
                delete score;
            }
        });
    }

    SECTION("active profile change")
    {
        exercise([](Fixture& fixture, RunnerFixture&) {
            fixture.identityProvider.replaceActiveProfile(
              true,
              arena::PublicIdentity{
                .userId = QStringLiteral("replacement-user"),
                .displayName = QStringLiteral("Replacement"),
              });
        });
    }

    SECTION("room leave acknowledged")
    {
        exercise([](Fixture& fixture, RunnerFixture&) {
            fixture.session.leaveRoom();
            fixture.transport.injectText(
              fixture.transport.connectCalls.back().generation,
              compact({
                { QStringLiteral("type"), QStringLiteral("room_member_left") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("roomId"), QStringLiteral("room-1") },
                    { QStringLiteral("roomGeneration"), 2 },
                    { QStringLiteral("memberId"), QStringLiteral("member-1") },
                    { QStringLiteral("reason"), QStringLiteral("left") },
                  } },
              }));
        });
    }

    SECTION("server round finalization")
    {
        exercise([](Fixture& fixture, RunnerFixture&) {
            fixture.transport.injectText(
              fixture.transport.connectCalls.back().generation,
              finalizedEvent());
            CHECK(fixture.session.getRoomPhase() ==
                  arena::RoomPhase::Selecting);
        });
    }

    SECTION("Arena exit")
    {
        exercise([](Fixture& fixture, RunnerFixture&) {
            fixture.session.exitArena();
        });
    }

    SECTION("runner destruction")
    {
        exercise([](Fixture& fixture, RunnerFixture& runner) {
            runner.runner.reset();
            QCoreApplication::processEvents();
            CHECK_FALSE(fixture.session.arenaGameplayActive());
            CHECK(fixture.gameplaySource.attachedRunner == nullptr);
        });
    }
}

TEST_CASE("ArenaSessionCompetition: capability downgrade rejects admission and "
          "resume locally",
          "[arena][ArenaSessionCompetition]")
{
    ensureCoreApplication();

    SECTION("minor one remains browse only")
    {
        Fixture fixture;
        fixture.session.connectForBrowsing();
        fixture.transport.injectConnected(1);
        auto hello = messageObject(competitionHello(false));
        auto data = hello.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("protocolMinor"), 1);
        data.insert(QStringLiteral("capabilities"),
                    QJsonArray{ QStringLiteral("rooms-v1"),
                                QStringLiteral("rounds-v1") });
        fixture.transport.injectText(
          1,
          compact({
            { QStringLiteral("type"), QStringLiteral("server_hello") },
            { QStringLiteral("data"), std::move(data) },
          }));
        CHECK(fixture.session.getRoundsAvailable());
        CHECK_FALSE(fixture.session.competitionAvailable());
        fixture.identityProvider.setLoggedIn(true);
        fixture.session.joinRoom(QStringLiteral("room-1"), QString{});
        CHECK(fixture.identityProvider.ticketRequests.isEmpty());
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("competition_capability_required"));
    }

    SECTION("resume downgrade returns to browser without sending commands")
    {
        Fixture fixture;
        fixture.enterRoom();
        const auto oldGeneration =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectDisconnected(oldGeneration);
        fixture.identityProvider.succeedTicket(
          fixture.identityProvider.ticketRequests.back(),
          QStringLiteral("resume-ticket"));
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);
        auto hello = messageObject(
          competitionHello(true, roomData(QStringLiteral("selecting"))));
        auto data = hello.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("protocolMinor"), 1);
        data.insert(QStringLiteral("capabilities"),
                    QJsonArray{ QStringLiteral("rooms-v1"),
                                QStringLiteral("rounds-v1") });
        fixture.transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"), QStringLiteral("server_hello") },
            { QStringLiteral("data"), std::move(data) },
          }));
        CHECK(fixture.session.getState() ==
              arena::ArenaSession::State::Browsing);
        CHECK(fixture.session.getRoomId().isEmpty());
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("competition_capability_required"));
        CHECK_FALSE(fixture.session.competitionAvailable());
    }
}
