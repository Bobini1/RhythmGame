#include "FakeArenaIdentityProvider.h"
#include "FakeArenaInventorySource.h"
#include "FakeArenaRoundLoader.h"
#include "FakeArenaScheduler.h"
#include "FakeArenaTransport.h"
#include "arena/ArenaBinaryProtocol.h"
#include "arena/ArenaSession.h"
#include "gameplay_logic/BmsGameReferee.h"
#include "gameplay_logic/BmsLiveScore.h"
#include "gameplay_logic/BmsNotes.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/NoteState.h"
#include "gameplay_logic/rules/HitRules.h"
#include "qml_components/Bga.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QElapsedTimer>
#include <QPromise>
#include <QThread>

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "RhythmGameArenaSessionTests";
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
      QStringLiteral("Arena runner"),
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
      QStringLiteral("arena-runner.bms"),
      0,
      QString(64, QChar(u'a')),
      QString(32, QChar(u'b')),
      gameplay_logic::ChartData::Keymode::K7,
      QList<QList<qint64>>{},
      QList<gameplay_logic::BpmChange>{},
      0);
}

auto
makeReadyChartRunner() -> std::unique_ptr<gameplay_logic::ChartRunner>
{
    using namespace std::chrono_literals;
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
        1'000,
        QString(64, QChar(u'a')),
        QString(32, QChar(u'b')),
        gameplay_logic::ChartData::Keymode::K7,
        0,
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
        1s,
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
    return runner;
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
serverHello(bool authenticated) -> QString
{
    auto data = QJsonObject{
        { QStringLiteral("protocolMajor"), 1 },
        { QStringLiteral("protocolMinor"), 1 },
        { QStringLiteral("capabilities"),
          QJsonArray{ QStringLiteral("rooms-v1"),
                      QStringLiteral("rounds-v1") } },
        { QStringLiteral("resume"),
          QJsonObject{
            { QStringLiteral("status"), QStringLiteral("not_requested") } } },
    };
    if (authenticated) {
        data.insert(
          QStringLiteral("identity"),
          QJsonObject{
            { QStringLiteral("userId"), QStringLiteral("user-1") },
            { QStringLiteral("displayName"), QStringLiteral("Alice") },
            { QStringLiteral("avatarUrl"), QJsonValue::Null },
          });
    }
    return QString::fromUtf8(QJsonDocument(QJsonObject{
                                             { QStringLiteral("type"),
                                               QStringLiteral("server_hello") },
                                             { QStringLiteral("data"), data },
                                           })
                               .toJson(QJsonDocument::Compact));
}

auto
legacyServerHello(bool authenticated) -> QString
{
    auto data = messageObject(serverHello(authenticated))
                  .value(QStringLiteral("data"))
                  .toObject();
    data.insert(QStringLiteral("protocolMinor"), 0);
    data.insert(QStringLiteral("capabilities"),
                QJsonArray{ QStringLiteral("rooms-v1") });
    return QString::fromUtf8(
      QJsonDocument(
        QJsonObject{ { QStringLiteral("type"), QStringLiteral("server_hello") },
                     { QStringLiteral("data"), data } })
        .toJson(QJsonDocument::Compact));
}

auto
phase2ServerHello(bool authenticated) -> QString
{
    auto data = messageObject(serverHello(authenticated))
                  .value(QStringLiteral("data"))
                  .toObject();
    data.insert(QStringLiteral("protocolMinor"), 1);
    data.insert(
      QStringLiteral("capabilities"),
      QJsonArray{ QStringLiteral("rooms-v1"), QStringLiteral("rounds-v1") });
    return QString::fromUtf8(
      QJsonDocument(
        QJsonObject{ { QStringLiteral("type"), QStringLiteral("server_hello") },
                     { QStringLiteral("data"), data } })
        .toJson(QJsonDocument::Compact));
}

auto
directorySnapshot(qint64 revision,
                  QString roomId = QStringLiteral("room-1"),
                  QString name = QStringLiteral("First room")) -> QString
{
    return QString::fromUtf8(
      QJsonDocument(
        QJsonObject{
          { QStringLiteral("type"), QStringLiteral("directory_snapshot") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("revision"), revision },
              { QStringLiteral("rooms"),
                QJsonArray{ QJsonObject{
                  { QStringLiteral("roomId"), roomId },
                  { QStringLiteral("name"), name },
                  { QStringLiteral("phase"), QStringLiteral("selecting") },
                  { QStringLiteral("hasPassword"), false },
                  { QStringLiteral("connectedCount"), 1 },
                  { QStringLiteral("reservedCount"), 0 },
                  { QStringLiteral("maxCount"), 16 },
                } } },
            } },
        })
        .toJson(QJsonDocument::Compact));
}

auto
compact(QJsonObject object) -> QString
{
    return QString::fromUtf8(
      QJsonDocument(std::move(object)).toJson(QJsonDocument::Compact));
}

auto
directoryDelta(qint64 revision, QJsonArray upserts, QJsonArray removed = {})
  -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("room_directory_updated") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("revision"), revision },
          { QStringLiteral("upserts"), std::move(upserts) },
          { QStringLiteral("removedRoomIds"), std::move(removed) } } },
    });
}

auto
roomSummary(QString roomId, QString name) -> QJsonObject
{
    return { { QStringLiteral("roomId"), std::move(roomId) },
             { QStringLiteral("name"), std::move(name) },
             { QStringLiteral("phase"), QStringLiteral("selecting") },
             { QStringLiteral("hasPassword"), false },
             { QStringLiteral("connectedCount"), 1 },
             { QStringLiteral("reservedCount"), 0 },
             { QStringLiteral("maxCount"), 16 } };
}

auto
member(QString memberId,
       QString displayName,
       QString status = QStringLiteral("connected"),
       qint64 wins = 0) -> QJsonObject
{
    return {
        { QStringLiteral("memberId"), std::move(memberId) },
        { QStringLiteral("identity"),
          QJsonObject{
            { QStringLiteral("userId"), QStringLiteral("user-1") },
            { QStringLiteral("displayName"), std::move(displayName) },
            { QStringLiteral("avatarUrl"), QJsonValue::Null } } },
        { QStringLiteral("status"), std::move(status) },
        { QStringLiteral("lobbyWins"), wins },
    };
}

auto
phase2Member(QString memberId,
             QString displayName,
             QString inventoryState = QStringLiteral("missing"),
             qint64 inventoryRevision = 0,
             qint64 availabilityAppliedRevision = 0) -> QJsonObject
{
    auto result = member(std::move(memberId), std::move(displayName));
    result.insert(QStringLiteral("ready"), false);
    result.insert(QStringLiteral("inventoryState"), std::move(inventoryState));
    result.insert(QStringLiteral("inventoryRevision"), inventoryRevision);
    result.insert(QStringLiteral("availabilityAppliedRevision"),
                  availabilityAppliedRevision);
    result.insert(QStringLiteral("roundState"), QStringLiteral("eligible"));
    return result;
}

auto
phase2Selection(QString sha256) -> QJsonObject
{
    return {
        { QStringLiteral("sha256"), std::move(sha256) },
        { QStringLiteral("title"), QStringLiteral("Arena chart") },
        { QStringLiteral("subtitle"), QString{} },
        { QStringLiteral("artist"), QStringLiteral("Composer") },
        { QStringLiteral("keyMode"), 7 },
        { QStringLiteral("randomSequence"), QJsonArray{} },
        { QStringLiteral("noteOrderP1"), QStringLiteral("normal") },
        { QStringLiteral("noteOrderP2"), QStringLiteral("mirror") },
        { QStringLiteral("dpMode"), QStringLiteral("off") },
        { QStringLiteral("laneSeed"), QStringLiteral("0123456789abcdef") },
        { QStringLiteral("randomizationVersion"), 1 },
    };
}

auto
phase2FrozenRound(QString sha256, QString stage = QStringLiteral("probing"))
  -> QJsonObject
{
    auto selection = phase2Selection(std::move(sha256));
    selection.insert(QStringLiteral("randomSequence"), QJsonArray{ 3, 1, 4 });
    selection.insert(QStringLiteral("noteOrderP1"),
                     QStringLiteral("s_random_plus"));
    selection.insert(QStringLiteral("noteOrderP2"),
                     QStringLiteral("lr2_random_ex"));
    selection.insert(QStringLiteral("dpMode"), QStringLiteral("lr2_flip"));
    return {
        { QStringLiteral("roundId"), QStringLiteral("round-1") },
        { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
        { QStringLiteral("selectionRevision"), 4 },
        { QStringLiteral("availabilityRevision"), 1 },
        { QStringLiteral("selection"), std::move(selection) },
        { QStringLiteral("participants"),
          QJsonArray{ QJsonObject{
            { QStringLiteral("memberId"), QStringLiteral("member-1") },
            { QStringLiteral("inventoryRevision"), 6 },
          } } },
        { QStringLiteral("stage"), std::move(stage) },
    };
}

auto
chatMessage(QString messageId,
            QString authorMemberId,
            QString displayName,
            QString text) -> QJsonObject
{
    return { { QStringLiteral("messageId"), std::move(messageId) },
             { QStringLiteral("authorMemberId"), std::move(authorMemberId) },
             { QStringLiteral("authorDisplayName"), std::move(displayName) },
             { QStringLiteral("sentAtMs"), 1000 },
             { QStringLiteral("text"), std::move(text) } };
}

auto
roomSnapshotData(QString token = QStringLiteral("seat-token-1"),
                 qint64 roomGeneration = 3,
                 qint64 connectionGeneration = 2,
                 QJsonArray members = {},
                 QJsonArray chat = {}) -> QJsonObject
{
    if (members.isEmpty()) {
        members.append(
          member(QStringLiteral("member-1"), QStringLiteral("Alice")));
    }
    return {
        { QStringLiteral("roomId"), QStringLiteral("room-1") },
        { QStringLiteral("roomGeneration"), roomGeneration },
        { QStringLiteral("name"), QStringLiteral("Arena room") },
        { QStringLiteral("phase"), QStringLiteral("selecting") },
        { QStringLiteral("hasPassword"), false },
        { QStringLiteral("maxCount"), 16 },
        { QStringLiteral("ownerMemberId"), QStringLiteral("member-1") },
        { QStringLiteral("self"),
          QJsonObject{
            { QStringLiteral("memberId"), QStringLiteral("member-1") },
            { QStringLiteral("connectionGeneration"), connectionGeneration },
            { QStringLiteral("resumeToken"), std::move(token) } } },
        { QStringLiteral("members"), std::move(members) },
        { QStringLiteral("chat"), std::move(chat) },
    };
}

auto
phase2RoomSnapshotData() -> QJsonObject
{
    auto result =
      roomSnapshotData(QStringLiteral("seat-token-1"),
                       3,
                       2,
                       QJsonArray{ phase2Member(QStringLiteral("member-1"),
                                                QStringLiteral("Alice")) });
    result.insert(QStringLiteral("selection"), QJsonValue::Null);
    result.insert(QStringLiteral("selectionRevision"), 0);
    result.insert(QStringLiteral("availabilityRevision"), 0);
    return result;
}

auto
roomSnapshot(QString requestId, QJsonObject data) -> QString
{
    return compact(
      { { QStringLiteral("type"), QStringLiteral("room_snapshot") },
        { QStringLiteral("requestId"), std::move(requestId) },
        { QStringLiteral("data"), std::move(data) } });
}

auto
resumeHello(QJsonObject room) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("server_hello") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("protocolMajor"), 1 },
          { QStringLiteral("protocolMinor"), 1 },
          { QStringLiteral("capabilities"),
            QJsonArray{ QStringLiteral("rooms-v1"),
                        QStringLiteral("rounds-v1") } },
          { QStringLiteral("identity"),
            QJsonObject{
              { QStringLiteral("userId"), QStringLiteral("user-1") },
              { QStringLiteral("displayName"), QStringLiteral("Alice") },
              { QStringLiteral("avatarUrl"), QJsonValue::Null },
            } },
          { QStringLiteral("resume"),
            QJsonObject{
              { QStringLiteral("status"), QStringLiteral("succeeded") },
              { QStringLiteral("room"), std::move(room) } } },
        } },
    });
}

auto
failedResumeHello() -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("server_hello") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("protocolMajor"), 1 },
          { QStringLiteral("protocolMinor"), 1 },
          { QStringLiteral("capabilities"),
            QJsonArray{ QStringLiteral("rooms-v1"),
                        QStringLiteral("rounds-v1") } },
          { QStringLiteral("identity"),
            QJsonObject{
              { QStringLiteral("userId"), QStringLiteral("user-1") },
              { QStringLiteral("displayName"), QStringLiteral("Alice") },
              { QStringLiteral("avatarUrl"), QJsonValue::Null },
            } },
          { QStringLiteral("resume"),
            QJsonObject{
              { QStringLiteral("status"), QStringLiteral("failed") },
              { QStringLiteral("code"), QStringLiteral("room_resume_failed") },
              { QStringLiteral("displayMessageKey"),
                QStringLiteral("arena.error.resumeFailed") },
            } },
        } },
    });
}

struct Fixture
{
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identity;
    arena::test::FakeArenaScheduler scheduler;
    arena::test::FakeArenaInventorySource inventory;
    arena::test::FakeArenaRoundLoader roundLoader;
    arena::ArenaSession session{ &transport,
                                 &identity,
                                 &scheduler,
                                 QUrl(QStringLiteral("ws://127.0.0.1:3001/ws")),
                                 QStringLiteral("2026.7.10"),
                                 &inventory,
                                 &roundLoader };

    void browse()
    {
        session.connectForBrowsing();
        transport.injectConnected(1);
        transport.injectText(1, serverHello(false));
    }

    auto authenticateAndCreate() -> QString
    {
        if (!session.getActive()) {
            browse();
        }
        identity.setLoggedIn(true);
        session.createRoom(QStringLiteral("Arena room"), QString{});
        REQUIRE_FALSE(identity.ticketRequests.isEmpty());
        identity.succeedTicket(identity.ticketRequests.back(),
                               QStringLiteral("short-lived-ticket"));
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        transport.injectText(generation, serverHello(true));
        const auto command = messageObject(transport.textCalls.back().message);
        REQUIRE(command.value(QStringLiteral("type")).toString() ==
                QStringLiteral("room_create"));
        return command.value(QStringLiteral("requestId")).toString();
    }

    void enterRoom()
    {
        const auto requestId = authenticateAndCreate();
        transport.injectText(transport.connectCalls.back().generation,
                             roomSnapshot(requestId, roomSnapshotData()));
        REQUIRE(session.getState() == arena::ArenaSession::State::InRoom);
    }

    void enterPhase2Room()
    {
        session.connectForBrowsing();
        transport.injectConnected(1);
        transport.injectText(1, phase2ServerHello(false));
        identity.setLoggedIn(true);
        session.createRoom(QStringLiteral("Arena room"), QString{});
        REQUIRE_FALSE(identity.ticketRequests.isEmpty());
        identity.succeedTicket(identity.ticketRequests.back(),
                               QStringLiteral("short-lived-ticket"));
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        transport.injectText(generation, phase2ServerHello(true));
        const auto command = messageObject(transport.textCalls.back().message);
        REQUIRE(command.value(QStringLiteral("type")).toString() ==
                QStringLiteral("room_create"));
        const auto requestId =
          command.value(QStringLiteral("requestId")).toString();
        transport.injectText(generation,
                             roomSnapshot(requestId, phase2RoomSnapshotData()));
        REQUIRE(session.getState() == arena::ArenaSession::State::InRoom);
    }

    void applyAvailabilityReset(qint64 revision,
                                const QByteArray& packed,
                                const QString& transferId)
    {
        const auto generation = transport.connectCalls.back().generation;
        const auto digest = QString::fromLatin1(
          QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
        transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"),
              QStringLiteral("availability_transfer_begin") },
            { QStringLiteral("data"),
              QJsonObject{
                { QStringLiteral("roomId"), QStringLiteral("room-1") },
                { QStringLiteral("roomGeneration"), 3 },
                { QStringLiteral("transferId"), transferId },
                { QStringLiteral("mode"), QStringLiteral("reset") },
                { QStringLiteral("targetRevision"), revision },
                { QStringLiteral("basis"),
                  QJsonArray{ QJsonObject{
                    { QStringLiteral("memberId"), QStringLiteral("member-1") },
                    { QStringLiteral("inventoryRevision"), 6 },
                  } } },
                { QStringLiteral("resetCount"),
                  packed.size() / arena::ArenaSha256Bytes },
                { QStringLiteral("resetChunkCount"), packed.isEmpty() ? 0 : 1 },
                { QStringLiteral("resetDigest"), digest },
              } },
          }));
        if (!packed.isEmpty()) {
            const auto rawTransferId = QByteArray::fromBase64(
              transferId.toLatin1(), QByteArray::Base64UrlEncoding);
            const auto encoded = arena::encodeArenaBinaryChunk({
              .kind = arena::ArenaBinaryKind::AvailabilityReset,
              .transferId = rawTransferId,
              .chunkIndex = 0,
              .packedHashes = packed,
            });
            REQUIRE(std::holds_alternative<QByteArray>(encoded));
            transport.injectBinary(generation, std::get<QByteArray>(encoded));
        }
        transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"),
              QStringLiteral("availability_transfer_commit") },
            { QStringLiteral("data"),
              QJsonObject{
                { QStringLiteral("roomId"), QStringLiteral("room-1") },
                { QStringLiteral("roomGeneration"), 3 },
                { QStringLiteral("transferId"), transferId },
                { QStringLiteral("targetRevision"), revision },
              } },
          }));
    }

    auto sendInventoryCommit(bool acknowledge) -> QString
    {
        REQUIRE_FALSE(inventory.requests.isEmpty());
        const auto packed = QByteArray(32, '\x55');
        const auto digest = QString::fromLatin1(
          QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
        inventory.succeed(inventory.requests.back(), packed);
        const auto begin = messageObject(transport.textCalls.back().message);
        REQUIRE(begin.value(QStringLiteral("type")).toString() ==
                QStringLiteral("inventory_upload_begin"));
        const auto beginRequestId =
          begin.value(QStringLiteral("requestId")).toString();
        const auto generation = transport.connectCalls.back().generation;
        transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"),
              QStringLiteral("inventory_upload_ready") },
            { QStringLiteral("requestId"), beginRequestId },
            { QStringLiteral("data"),
              QJsonObject{
                { QStringLiteral("roomId"), QStringLiteral("room-1") },
                { QStringLiteral("roomGeneration"), 3 },
                { QStringLiteral("connectionGeneration"), 2 },
                { QStringLiteral("uploadId"),
                  QStringLiteral("AAAAAAAAAAAAAAAAAAAAAA") },
                { QStringLiteral("libraryGeneration"),
                  inventory.currentGeneration },
                { QStringLiteral("hashCount"), 1 },
                { QStringLiteral("byteCount"), 32 },
                { QStringLiteral("chunkCount"), 1 },
                { QStringLiteral("vectorDigest"), digest },
                { QStringLiteral("deadlineMs"), 60'000 },
              } },
          }));
        const auto commit = messageObject(transport.textCalls.back().message);
        REQUIRE(commit.value(QStringLiteral("type")).toString() ==
                QStringLiteral("inventory_upload_commit"));
        const auto commitRequestId =
          commit.value(QStringLiteral("requestId")).toString();
        if (acknowledge) {
            transport.injectText(
              generation,
              compact({
                { QStringLiteral("type"),
                  QStringLiteral("inventory_committed") },
                { QStringLiteral("requestId"), commitRequestId },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("roomId"), QStringLiteral("room-1") },
                    { QStringLiteral("roomGeneration"), 3 },
                    { QStringLiteral("connectionGeneration"), 2 },
                    { QStringLiteral("libraryGeneration"),
                      inventory.currentGeneration },
                    { QStringLiteral("inventoryRevision"), 1 },
                    { QStringLiteral("inventoryState"),
                      QStringLiteral("ready") },
                  } },
              }));
        }
        return commitRequestId;
    }
};

} // namespace

TEST_CASE("ArenaSession publishes browsing only after anonymous hello",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;

    CHECK_FALSE(fixture.session.getActive());
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    fixture.session.connectForBrowsing();
    REQUIRE(fixture.transport.connectCalls.size() == 1);
    CHECK(fixture.transport.connectCalls.front().generation == 1);
    CHECK(fixture.session.getActive());
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    CHECK_FALSE(fixture.session.getDirectoryReady());
    CHECK(fixture.transport.textCalls.isEmpty());

    fixture.transport.injectConnected(1);
    REQUIRE(fixture.transport.textCalls.size() == 1);
    const auto hello =
      messageObject(fixture.transport.textCalls.front().message);
    CHECK(hello.value(QStringLiteral("type")).toString() ==
          QStringLiteral("client_hello"));
    CHECK_FALSE(hello.value(QStringLiteral("data"))
                  .toObject()
                  .contains(QStringLiteral("ticket")));
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);

    fixture.transport.injectText(1, serverHello(false));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK_FALSE(fixture.session.getAuthenticated());
    REQUIRE(fixture.transport.textCalls.size() == 2);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("directory_subscribe"));

    fixture.transport.injectText(1, directorySnapshot(1));
    CHECK(fixture.session.getDirectoryReady());
    REQUIRE(fixture.session.getRooms()->rowCount() == 1);
    CHECK(fixture.session.getRooms()
            ->data(fixture.session.getRooms()->index(0, 0),
                   arena::ArenaRoomListModel::NameRole)
            .toString() == QStringLiteral("First room"));
}

TEST_CASE("ArenaSession uploads one packed inventory after Phase 2 admission",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();

    CHECK(fixture.session.getRoundsAvailable());
    REQUIRE(fixture.inventory.requests.size() == 1);
    const auto snapshotRequest = fixture.inventory.requests.front();
    const auto packed = QByteArray(32, '\x11');
    const auto digest = QString::fromLatin1(
      QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
    fixture.inventory.succeed(snapshotRequest, packed);

    const auto begin =
      messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(begin.value(QStringLiteral("type")).toString() ==
            QStringLiteral("inventory_upload_begin"));
    const auto beginRequestId =
      begin.value(QStringLiteral("requestId")).toString();
    const auto declaration = begin.value(QStringLiteral("data")).toObject();
    CHECK(declaration.value(QStringLiteral("libraryGeneration")).toInteger() ==
          fixture.inventory.currentGeneration);
    CHECK(declaration.value(QStringLiteral("hashCount")).toInteger() == 1);
    CHECK(declaration.value(QStringLiteral("byteCount")).toInteger() == 32);
    CHECK(declaration.value(QStringLiteral("chunkCount")).toInteger() == 1);
    CHECK(declaration.value(QStringLiteral("vectorDigest")).toString() ==
          digest);

    const auto uploadId = QStringLiteral("AAAAAAAAAAAAAAAAAAAAAA");
    const auto uploadReady = compact({
      { QStringLiteral("type"), QStringLiteral("inventory_upload_ready") },
      { QStringLiteral("requestId"), beginRequestId },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 3 },
          { QStringLiteral("connectionGeneration"), 2 },
          { QStringLiteral("uploadId"), uploadId },
          { QStringLiteral("libraryGeneration"),
            fixture.inventory.currentGeneration },
          { QStringLiteral("hashCount"), 1 },
          { QStringLiteral("byteCount"), 32 },
          { QStringLiteral("chunkCount"), 1 },
          { QStringLiteral("vectorDigest"), digest },
          { QStringLiteral("deadlineMs"), 60'000 },
        } },
    });
    fixture.transport.injectText(
      fixture.transport.connectCalls.back().generation, uploadReady);

    REQUIRE(fixture.transport.binaryCalls.size() == 1);
    const auto decoded = arena::decodeArenaBinaryChunk(
      fixture.transport.binaryCalls.front().bytes);
    REQUIRE(std::holds_alternative<arena::ArenaBinaryChunk>(decoded));
    const auto& chunk = std::get<arena::ArenaBinaryChunk>(decoded);
    CHECK(chunk.kind == arena::ArenaBinaryKind::InventoryUpload);
    CHECK(chunk.chunkIndex == 0);
    CHECK(chunk.packedHashes == packed);

    const auto commit =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(commit.value(QStringLiteral("type")).toString() ==
          QStringLiteral("inventory_upload_commit"));
    CHECK(commit.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("uploadId"))
            .toString() == uploadId);
    const auto writesAfterCommit = fixture.transport.textCalls.size();
    fixture.transport.injectText(
      fixture.transport.connectCalls.back().generation, uploadReady);
    CHECK(fixture.transport.binaryCalls.size() == 1);
    CHECK(fixture.transport.textCalls.size() == writesAfterCommit);
}

TEST_CASE("ArenaSession falls back once to anonymous legacy browse only",
          "[arena][session][protocol]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.session.connectForBrowsing();
    fixture.transport.injectConnected(1);
    auto hello = messageObject(fixture.transport.textCalls.back().message);
    CHECK(hello.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("protocolMinor"))
            .toInt() == 1);

    const auto incompatible = compact({
      { QStringLiteral("type"), QStringLiteral("fatal_error") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("code"), QStringLiteral("protocol_incompatible") },
          { QStringLiteral("displayMessageKey"),
            QStringLiteral("arena.error.protocolIncompatible") },
        } },
    });
    fixture.transport.injectText(1, incompatible);
    REQUIRE(fixture.transport.connectCalls.size() == 2);
    const auto fallbackGeneration =
      fixture.transport.connectCalls.back().generation;
    fixture.transport.injectConnected(fallbackGeneration);
    hello = messageObject(fixture.transport.textCalls.back().message);
    const auto fallbackData = hello.value(QStringLiteral("data")).toObject();
    CHECK(fallbackData.value(QStringLiteral("protocolMinor")).toInt() == 0);
    CHECK(fallbackData.value(QStringLiteral("capabilities")).toArray() ==
          QJsonArray{ QStringLiteral("rooms-v1") });

    fixture.transport.injectText(fallbackGeneration, legacyServerHello(false));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK_FALSE(fixture.session.getRoundsAvailable());
    fixture.identity.setLoggedIn(true);
    fixture.session.createRoom(QStringLiteral("Blocked"), QString{});
    CHECK(fixture.identity.ticketRequests.isEmpty());
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("rounds_capability_required"));

    const auto connections = fixture.transport.connectCalls.size();
    fixture.transport.injectText(fallbackGeneration, incompatible);
    CHECK(fixture.transport.connectCalls.size() == connections);
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Error);
}

TEST_CASE("ArenaSession never admits a seat without rounds-v1 negotiation",
          "[arena][session][protocol]")
{
    ensureCoreApplication();

    SECTION("rooms-only anonymous browse")
    {
        Fixture fixture;
        fixture.session.connectForBrowsing();
        fixture.transport.injectConnected(1);
        fixture.transport.injectText(1, legacyServerHello(false));
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::Browsing);
        fixture.identity.setLoggedIn(true);
        fixture.session.joinRoom(QStringLiteral("room-1"), QString{});
        CHECK(fixture.identity.ticketRequests.isEmpty());
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("rounds_capability_required"));
    }

    SECTION("authenticated connection downgrades capability")
    {
        Fixture fixture;
        fixture.browse();
        fixture.identity.setLoggedIn(true);
        fixture.session.createRoom(QStringLiteral("Arena room"), QString{});
        REQUIRE(fixture.identity.ticketRequests.size() == 1);
        fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                       QStringLiteral("ticket"));
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);
        const auto writesBeforeHello = fixture.transport.textCalls.size();
        fixture.transport.injectText(generation, legacyServerHello(true));
        REQUIRE(fixture.transport.textCalls.size() == writesBeforeHello + 1);
        CHECK(messageObject(fixture.transport.textCalls.back().message)
                .value(QStringLiteral("type"))
                .toString() == QStringLiteral("directory_subscribe"));
        CHECK_FALSE(fixture.session.getAdmissionPending());
        CHECK(fixture.session.getErrorCode() ==
              QStringLiteral("rounds_capability_required"));
    }
}

TEST_CASE(
  "ArenaSession applies common availability atomically and resyncs gaps",
  "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x22');
    const auto digest = QString::fromLatin1(
      QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
    const auto transferId = QStringLiteral("BBBBBBBBBBBBBBBBBBBBBB");

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"),
          QStringLiteral("availability_transfer_begin") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("transferId"), transferId },
            { QStringLiteral("mode"), QStringLiteral("reset") },
            { QStringLiteral("targetRevision"), 1 },
            { QStringLiteral("basis"),
              QJsonArray{ QJsonObject{
                { QStringLiteral("memberId"), QStringLiteral("member-1") },
                { QStringLiteral("inventoryRevision"), 1 },
              } } },
            { QStringLiteral("resetCount"), 1 },
            { QStringLiteral("resetChunkCount"), 1 },
            { QStringLiteral("resetDigest"), digest },
          } },
      }));
    CHECK(fixture.session.getAvailabilitySyncing());

    const auto rawTransferId = QByteArray::fromBase64(
      transferId.toLatin1(), QByteArray::Base64UrlEncoding);
    const auto encoded = arena::encodeArenaBinaryChunk({
      .kind = arena::ArenaBinaryKind::AvailabilityReset,
      .transferId = rawTransferId,
      .chunkIndex = 0,
      .packedHashes = packed,
    });
    REQUIRE(std::holds_alternative<QByteArray>(encoded));
    fixture.transport.injectBinary(generation, std::get<QByteArray>(encoded));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"),
          QStringLiteral("availability_transfer_commit") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("transferId"),
              QStringLiteral("DDDDDDDDDDDDDDDDDDDDDD") },
            { QStringLiteral("targetRevision"), 1 },
          } },
      }));
    CHECK(fixture.session.getAvailabilitySyncing());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"),
          QStringLiteral("availability_transfer_commit") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("transferId"), transferId },
            { QStringLiteral("targetRevision"), 1 },
          } },
      }));

    CHECK_FALSE(fixture.session.getAvailabilitySyncing());
    REQUIRE(fixture.session.getAvailability()->revision() == 1);
    CHECK(fixture.session.getAvailability()->availability(
            QString::fromLatin1(packed.toHex())) ==
          arena::ArenaAvailabilityIndex::Availability::AvailableToAll);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("availability_applied"));

    const auto emptyDigest = QString::fromLatin1(
      QCryptographicHash::hash(QByteArray{}, QCryptographicHash::Sha256)
        .toHex());
    const auto deltaTransferId = QStringLiteral("CCCCCCCCCCCCCCCCCCCCCC");
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"),
          QStringLiteral("availability_transfer_begin") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("transferId"), deltaTransferId },
            { QStringLiteral("mode"), QStringLiteral("delta") },
            { QStringLiteral("targetRevision"), 2 },
            { QStringLiteral("basis"),
              QJsonArray{ QJsonObject{
                { QStringLiteral("memberId"), QStringLiteral("member-1") },
                { QStringLiteral("inventoryRevision"), 1 },
              } } },
            { QStringLiteral("baseRevision"), 1 },
            { QStringLiteral("addedCount"), 1 },
            { QStringLiteral("addedChunkCount"), 1 },
            { QStringLiteral("addedDigest"), digest },
            { QStringLiteral("removedCount"), 0 },
            { QStringLiteral("removedChunkCount"), 0 },
            { QStringLiteral("removedDigest"), emptyDigest },
          } },
      }));
    const auto rawDeltaId = QByteArray::fromBase64(
      deltaTransferId.toLatin1(), QByteArray::Base64UrlEncoding);
    const auto outOfOrder = arena::encodeArenaBinaryChunk({
      .kind = arena::ArenaBinaryKind::AvailabilityAdd,
      .transferId = rawDeltaId,
      .chunkIndex = 1,
      .packedHashes = packed,
    });
    REQUIRE(std::holds_alternative<QByteArray>(outOfOrder));
    fixture.transport.injectBinary(generation,
                                   std::get<QByteArray>(outOfOrder));

    CHECK(fixture.session.getAvailabilitySyncing());
    CHECK(fixture.session.getAvailability()->revision() == 1);
    const auto resync =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(resync.value(QStringLiteral("type")).toString() ==
          QStringLiteral("availability_resync"));
    CHECK(resync.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("currentRevision"))
            .toInteger() == 1);
}

TEST_CASE("ArenaSession readies only against the exact common selection basis",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x33');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    REQUIRE(fixture.session.getAvailability()->applyReset(1, packed));

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("selection_changed") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("selectionRevision"), 4 },
            { QStringLiteral("availabilityRevision"), 1 },
            { QStringLiteral("selection"), phase2Selection(sha256) },
            { QStringLiteral("selectedByMemberId"),
              QStringLiteral("member-1") },
          } },
      }));

    CHECK(fixture.session.getCanSelect());
    CHECK(fixture.session.getCanReady());
    CHECK_FALSE(fixture.session.getReady());
    CHECK(fixture.session.getSelectedTitle() == QStringLiteral("Arena chart"));
    fixture.session.setReady(true);
    const auto ready =
      messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(ready.value(QStringLiteral("type")).toString() ==
            QStringLiteral("ready_set"));
    const auto data = ready.value(QStringLiteral("data")).toObject();
    CHECK(data.value(QStringLiteral("ready")).toBool());
    CHECK(data.value(QStringLiteral("selectionRevision")).toInteger() == 4);
    CHECK(data.value(QStringLiteral("availabilityRevision")).toInteger() == 1);
    CHECK(data.value(QStringLiteral("inventoryRevision")).toInteger() == 6);

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("syncing"),
                           6,
                           1) },
          } },
      }));
    CHECK_FALSE(fixture.session.getCanSelect());
    CHECK_FALSE(fixture.session.getCanReady());
}

TEST_CASE("ArenaSession selects only a locally common immutable snapshot",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x38');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    fixture.applyAvailabilityReset(
      1, packed, QStringLiteral("GGGGGGGGGGGGGGGGGGGGGG"));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    REQUIRE(fixture.session.getCanSelect());

    fixture.roundLoader.nextSelection = arena::SelectionSnapshot{
        .sha256 = sha256,
        .title = QStringLiteral("Immutable chart"),
        .subtitle = QString{},
        .artist = QStringLiteral("Composer"),
        .keyMode = 7,
        .randomSequence = { 2, 1 },
        .noteOrderP1 = arena::NoteOrder::SRandom,
        .noteOrderP2 = arena::NoteOrder::Mirror,
        .dpMode = arena::DpMode::Off,
        .laneSeed = QStringLiteral("0123456789abcdef"),
        .randomizationVersion = 1,
    };
    fixture.session.selectChart(
      reinterpret_cast<gameplay_logic::ChartData*>(quintptr{ 1 }));
    const auto selection =
      messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(selection.value(QStringLiteral("type")).toString() ==
            QStringLiteral("selection_set"));
    const auto data = selection.value(QStringLiteral("data")).toObject();
    CHECK(data.value(QStringLiteral("availabilityRevision")).toInteger() == 1);
    CHECK(data.value(QStringLiteral("inventoryRevision")).toInteger() == 6);
    CHECK(data.value(QStringLiteral("selection"))
            .toObject()
            .value(QStringLiteral("sha256"))
            .toString() == sha256);
}

TEST_CASE("ArenaSession probes and loads the exact frozen round",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x39');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    fixture.applyAvailabilityReset(
      1, packed, QStringLiteral("HHHHHHHHHHHHHHHHHHHHHH"));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), phase2FrozenRound(sha256) },
          } },
      }));
    REQUIRE(fixture.session.getRoomPhase() == arena::RoomPhase::Loading);
    REQUIRE(fixture.session.getCurrentRoundId() == QStringLiteral("round-1"));
    auto unexpectedRound = phase2FrozenRound(sha256);
    unexpectedRound.insert(QStringLiteral("roundId"),
                           QStringLiteral("round-2"));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), std::move(unexpectedRound) },
          } },
      }));
    CHECK(fixture.session.getCurrentRoundId() == QStringLiteral("round-1"));
    // A library rescan may publish a newer next-round inventory while this
    // round remains bound to its frozen revision.
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           7,
                           1) },
          } },
      }));

    const auto probeRequest = [&](qint64 inventoryRevision) {
        return compact({
          { QStringLiteral("type"), QStringLiteral("round_probe_requested") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("roomId"), QStringLiteral("room-1") },
              { QStringLiteral("roomGeneration"), 3 },
              { QStringLiteral("connectionGeneration"), 2 },
              { QStringLiteral("roundId"), QStringLiteral("round-1") },
              { QStringLiteral("launchAttemptId"),
                QStringLiteral("attempt-1") },
              { QStringLiteral("selectionRevision"), 4 },
              { QStringLiteral("availabilityRevision"), 1 },
              { QStringLiteral("inventoryRevision"), inventoryRevision },
              { QStringLiteral("nonce"), QStringLiteral("probe-nonce-1") },
              { QStringLiteral("sha256"), sha256 },
              { QStringLiteral("deadlineMs"), 15'000 },
            } },
        });
    };
    fixture.transport.injectText(generation, probeRequest(5));
    CHECK(fixture.roundLoader.probes.isEmpty());
    fixture.transport.injectText(generation, probeRequest(6));
    REQUIRE(fixture.roundLoader.probes.size() == 1);
    const auto probeId = fixture.roundLoader.probes.front().first;
    CHECK(fixture.roundLoader.probes.front().second == packed);

    emit fixture.roundLoader.probeFinished(
      probeId,
      arena::ArenaProbeResult{ .failure = arena::ArenaProbeFailure::None,
                               .observedSha256 = packed });
    auto response = messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(response.value(QStringLiteral("type")).toString() ==
            QStringLiteral("round_probe_result"));
    auto responseData = response.value(QStringLiteral("data")).toObject();
    CHECK(responseData.value(QStringLiteral("roundId")).toString() ==
          QStringLiteral("round-1"));
    CHECK(responseData.value(QStringLiteral("launchAttemptId")).toString() ==
          QStringLiteral("attempt-1"));
    CHECK(responseData.value(QStringLiteral("nonce")).toString() ==
          QStringLiteral("probe-nonce-1"));
    CHECK(responseData.value(QStringLiteral("ok")).toBool());
    CHECK(responseData.value(QStringLiteral("sha256")).toString() == sha256);
    CHECK_FALSE(responseData.contains(QStringLiteral("path")));

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_load_requested") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("connectionGeneration"), 2 },
            { QStringLiteral("round"),
              phase2FrozenRound(sha256, QStringLiteral("loading")) },
          } },
      }));
    REQUIRE(fixture.roundLoader.loads.size() == 1);
    const auto& [loadId, request] = fixture.roundLoader.loads.front();
    CHECK(request.sha256 == packed);
    CHECK(request.playConfig.randomSequence == QList<qint64>{ 3, 1, 4 });
    CHECK(request.playConfig.noteOrderP1 ==
          resource_managers::NoteOrderAlgorithm::SRandomPlus);
    CHECK(request.playConfig.noteOrderP2 ==
          resource_managers::NoteOrderAlgorithm::Lr2RandomEx);
    CHECK(request.playConfig.dpMode == resource_managers::DpOptions::Lr2Flip);
    CHECK(request.playConfig.laneSeed == 0x0123456789abcdefULL);
    CHECK(request.playConfig.randomizationVersion == 1);

    const auto cancellationsBeforeStaleProbe =
      fixture.roundLoader.cancellations.size();
    fixture.transport.injectText(generation, probeRequest(6));
    CHECK(fixture.roundLoader.probes.size() == 1);
    CHECK(fixture.roundLoader.loads.size() == 1);
    CHECK(fixture.roundLoader.cancellations.size() ==
          cancellationsBeforeStaleProbe);

    emit fixture.roundLoader.loadFailed(loadId,
                                        arena::ArenaLoadFailure::ParseFailed);
    response = messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(response.value(QStringLiteral("type")).toString() ==
            QStringLiteral("round_load_result"));
    responseData = response.value(QStringLiteral("data")).toObject();
    CHECK_FALSE(responseData.value(QStringLiteral("ok")).toBool());
    CHECK(responseData.value(QStringLiteral("reason")).toString() ==
          QStringLiteral("parse_failed"));
    CHECK(responseData.value(QStringLiteral("inventoryRevision")).toInteger() ==
          6);
    CHECK_FALSE(responseData.contains(QStringLiteral("path")));
}

TEST_CASE("ArenaSession anchors and preserves a held synchronized runner",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    auto runner = makeReadyChartRunner();
    Fixture fixture;
    fixture.enterPhase2Room();
    auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x3d');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), phase2FrozenRound(sha256) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_load_requested") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("connectionGeneration"), 2 },
            { QStringLiteral("round"),
              phase2FrozenRound(sha256, QStringLiteral("loading")) },
          } },
    }));
    REQUIRE(fixture.roundLoader.loads.size() == 1);
    const auto loadId = fixture.roundLoader.loads.front().first;
    runner->holdStart();
    emit fixture.roundLoader.loadFinished(loadId, runner.get());
    REQUIRE(messageObject(fixture.transport.textCalls.back().message)
              .value(QStringLiteral("type"))
              .toString() == QStringLiteral("round_load_result"));

    QVector<gameplay_logic::ChartRunner*> prepared;
    int runnerStarted = 0;
    int launchCancelled = 0;
    QObject::connect(
      &fixture.session,
      &arena::ArenaSession::preparedGameplayChanged,
      [&](gameplay_logic::ChartRunner* value) {
          prepared.push_back(value);
          if (value != nullptr) {
              // Simulate expensive synchronous QML creation. The release
              // deadline must remain anchored to message receipt.
              fixture.scheduler.advanceBy(750);
          }
      });
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::roundRunnerStarted,
                     [&](const QString& roundId,
                         gameplay_logic::ChartRunner* value) {
                         CHECK(roundId == QStringLiteral("round-1"));
                         CHECK(value == runner.get());
                         ++runnerStarted;
                     });
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::roundLaunchCancelled,
                     [&] {
                         CHECK(fixture.session.getRoomPhase() ==
                               arena::RoomPhase::Selecting);
                         CHECK(fixture.session.getCurrentRoundId().isEmpty());
                         ++launchCancelled;
                     });
    const auto schedule = [&](qint64 connectionGeneration,
                              qint64 startAfterMs) {
        fixture.transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"),
              QStringLiteral("round_start_scheduled") },
            { QStringLiteral("data"),
              QJsonObject{
                { QStringLiteral("roomId"), QStringLiteral("room-1") },
                { QStringLiteral("roomGeneration"), 3 },
                { QStringLiteral("connectionGeneration"),
                  connectionGeneration },
                { QStringLiteral("roundId"), QStringLiteral("round-1") },
                { QStringLiteral("launchAttemptId"),
                  QStringLiteral("attempt-1") },
                { QStringLiteral("startAtServerMs"), 10'000 },
                { QStringLiteral("startAfterMs"), startAfterMs },
              } },
          }));
    };
    schedule(2, 1'000);
    REQUIRE(prepared == QVector<gameplay_logic::ChartRunner*>{ runner.get() });
    CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Ready);

    SECTION("release stays anchored across prepared UI work")
    {
        fixture.scheduler.advanceBy(249);
        CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Ready);
        fixture.scheduler.advanceBy(1);
        CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Running);
        CHECK(runnerStarted == 1);
    }

    SECTION("scheduled resume retains and rearms the same runner")
    {
        const auto cancellations = fixture.roundLoader.cancellations.size();
        fixture.transport.injectDisconnected(generation);
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::Reconnecting);
        CHECK(fixture.roundLoader.cancellations.size() == cancellations);
        REQUIRE_FALSE(fixture.identity.ticketRequests.isEmpty());
        fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                       QStringLiteral("resume-ticket"));
        generation = fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);

        auto resumedRound =
          phase2FrozenRound(sha256, QStringLiteral("scheduled"));
        auto resumed = phase2RoomSnapshotData();
        resumed.insert(QStringLiteral("phase"), QStringLiteral("loading"));
        resumed.insert(QStringLiteral("selection"),
                       resumedRound.value(QStringLiteral("selection")));
        resumed.insert(QStringLiteral("selectionRevision"), 4);
        resumed.insert(QStringLiteral("availabilityRevision"), 1);
        resumed.insert(QStringLiteral("round"), resumedRound);
        resumed.insert(
          QStringLiteral("members"),
          QJsonArray{ phase2Member(QStringLiteral("member-1"),
                                   QStringLiteral("Alice"),
                                   QStringLiteral("ready"),
                                   6,
                                   1) });
        auto self = resumed.value(QStringLiteral("self")).toObject();
        self.insert(QStringLiteral("connectionGeneration"), 3);
        self.insert(QStringLiteral("resumeToken"),
                    QStringLiteral("rotated-seat-token"));
        resumed.insert(QStringLiteral("self"), self);
        fixture.transport.injectText(generation,
                                     resumeHello(std::move(resumed)));
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::InRoom);
        CHECK(prepared.size() == 1);
        schedule(3, 500);
        CHECK(prepared.size() == 1);
        fixture.scheduler.advanceBy(499);
        CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Ready);
        fixture.scheduler.advanceBy(1);
        CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Running);
        CHECK(runnerStarted == 1);
        CHECK(fixture.roundLoader.cancellations.size() == cancellations);
    }

    SECTION("pre-start cancellation pops before destroying the runner")
    {
        fixture.transport.injectText(
          generation,
          compact({
            { QStringLiteral("type"),
              QStringLiteral("round_launch_cancelled") },
            { QStringLiteral("data"),
              QJsonObject{
                { QStringLiteral("roomId"), QStringLiteral("room-1") },
                { QStringLiteral("roomGeneration"), 3 },
                { QStringLiteral("roundId"), QStringLiteral("round-1") },
                { QStringLiteral("launchAttemptId"),
                  QStringLiteral("attempt-1") },
                { QStringLiteral("reason"), QStringLiteral("cancelled") },
                { QStringLiteral("selection"), QJsonValue::Null },
                { QStringLiteral("selectionRevision"), 5 },
                { QStringLiteral("availabilityRevision"), 1 },
              } },
          }));
        REQUIRE(prepared.size() == 2);
        CHECK(prepared.back() == nullptr);
        CHECK(launchCancelled == 1);
        CHECK(fixture.roundLoader.cancellations ==
              QVector<quint64>{ loadId });
        fixture.scheduler.advanceBy(1'000);
        CHECK(runner->getStatus() == gameplay_logic::ChartRunner::Ready);
        CHECK(runnerStarted == 0);
    }
}

TEST_CASE("ArenaSession invalidates frozen callbacks before reconnect",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x3a');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), phase2FrozenRound(sha256) },
          } },
      }));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_probe_requested") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("connectionGeneration"), 2 },
            { QStringLiteral("roundId"), QStringLiteral("round-1") },
            { QStringLiteral("launchAttemptId"), QStringLiteral("attempt-1") },
            { QStringLiteral("selectionRevision"), 4 },
            { QStringLiteral("availabilityRevision"), 1 },
            { QStringLiteral("inventoryRevision"), 6 },
            { QStringLiteral("nonce"), QStringLiteral("probe-nonce-2") },
            { QStringLiteral("sha256"), sha256 },
            { QStringLiteral("deadlineMs"), 15'000 },
          } },
      }));
    REQUIRE(fixture.roundLoader.probes.size() == 1);
    const auto oldProbeId = fixture.roundLoader.probes.front().first;

    fixture.transport.injectDisconnected(generation);
    REQUIRE(fixture.session.getState() ==
            arena::ArenaSession::State::Reconnecting);
    CHECK(fixture.roundLoader.cancellations == QVector<quint64>{ oldProbeId });
    const auto writesAfterDisconnect = fixture.transport.textCalls.size();
    emit fixture.roundLoader.probeFinished(
      oldProbeId,
      arena::ArenaProbeResult{ .failure = arena::ArenaProbeFailure::None,
                               .observedSha256 = packed });
    CHECK(fixture.transport.textCalls.size() == writesAfterDisconnect);
}

TEST_CASE("ArenaSession publishes authoritative cancellation state atomically",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto sha256 = QString::fromLatin1(QByteArray(32, '\x3b').toHex());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), phase2FrozenRound(sha256) },
          } },
      }));
    int selectionChanges = 0;
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::selectionChanged,
                     [&] { ++selectionChanges; });
    bool observedSelectingState = false;
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::roundChanged,
                     [&] {
                         if (fixture.session.getRoomPhase() ==
                             arena::RoomPhase::Selecting) {
                             observedSelectingState = true;
                             CHECK(fixture.session.getCurrentRoundId().isEmpty());
                         }
                     });

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_launch_cancelled") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("roundId"), QStringLiteral("round-1") },
            { QStringLiteral("launchAttemptId"),
              QStringLiteral("attempt-1") },
            { QStringLiteral("reason"), QStringLiteral("cancelled") },
            { QStringLiteral("selection"), QJsonValue::Null },
            { QStringLiteral("selectionRevision"), 0 },
            { QStringLiteral("availabilityRevision"), 0 },
          } },
      }));

    CHECK(observedSelectingState);
    CHECK(selectionChanges >= 1);
}

TEST_CASE("ArenaSession advances waiting spectators on room-wide round start",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto sha256 = QString::fromLatin1(QByteArray(32, '\x3c').toHex());
    auto round = phase2FrozenRound(sha256);
    round.insert(
      QStringLiteral("participants"),
      QJsonArray{ QJsonObject{
        { QStringLiteral("memberId"), QStringLiteral("member-2") },
        { QStringLiteral("inventoryRevision"), 4 },
      } });
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_loading_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("round"), std::move(round) },
          } },
      }));
    REQUIRE(fixture.session.getRoomPhase() == arena::RoomPhase::Loading);

    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("round_started") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("roundId"), QStringLiteral("round-1") },
            { QStringLiteral("launchAttemptId"),
              QStringLiteral("attempt-1") },
          } },
      }));
    CHECK(fixture.session.getRoomPhase() == arena::RoomPhase::Playing);
}

TEST_CASE("ArenaSession publishes only the newest queued library generation",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    REQUIRE(fixture.inventory.requests.size() == 1);
    const auto staleRequest = fixture.inventory.requests.front();
    const auto writesBeforeMutation = fixture.transport.textCalls.size();

    fixture.inventory.advanceGeneration();
    REQUIRE(fixture.inventory.cancellations ==
            QVector<quint64>{ staleRequest });
    REQUIRE(fixture.inventory.requests.size() == 2);
    const auto currentRequest = fixture.inventory.requests.back();
    CHECK(currentRequest != staleRequest);

    fixture.inventory.succeed(staleRequest, QByteArray(32, '\x10'));
    CHECK(fixture.transport.textCalls.size() == writesBeforeMutation);
    fixture.inventory.succeed(currentRequest, QByteArray(32, '\x20'));
    REQUIRE(fixture.transport.textCalls.size() == writesBeforeMutation + 1);
    const auto begin =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(begin.value(QStringLiteral("type")).toString() ==
          QStringLiteral("inventory_upload_begin"));
    CHECK(begin.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("libraryGeneration"))
            .toInteger() == 2);
}

TEST_CASE("ArenaSession advances readiness when availability retains selection",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    const auto generation = fixture.transport.connectCalls.back().generation;
    const auto packed = QByteArray(32, '\x44');
    const auto sha256 = QString::fromLatin1(packed.toHex());
    fixture.applyAvailabilityReset(
      1, packed, QStringLiteral("EEEEEEEEEEEEEEEEEEEEEE"));
    auto selectionNotifications = 0;
    QObject::connect(&fixture.session,
                     &arena::ArenaSession::selectionChanged,
                     [&] { ++selectionNotifications; });
    CHECK_FALSE(fixture.session.getCanSelect());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           1) },
          } },
      }));
    CHECK(selectionNotifications > 0);
    CHECK(fixture.session.getCanSelect());
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("selection_changed") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("selectionRevision"), 4 },
            { QStringLiteral("availabilityRevision"), 1 },
            { QStringLiteral("selection"), phase2Selection(sha256) },
            { QStringLiteral("selectedByMemberId"),
              QStringLiteral("member-1") },
          } },
      }));
    REQUIRE(fixture.session.getCanReady());

    fixture.applyAvailabilityReset(
      2, packed, QStringLiteral("FFFFFFFFFFFFFFFFFFFFFF"));
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("room_member_updated") },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 3 },
            { QStringLiteral("member"),
              phase2Member(QStringLiteral("member-1"),
                           QStringLiteral("Alice"),
                           QStringLiteral("ready"),
                           6,
                           2) },
          } },
      }));
    REQUIRE(fixture.session.getCanReady());
    fixture.session.setReady(true);
    const auto ready = messageObject(fixture.transport.textCalls.back().message)
                         .value(QStringLiteral("data"))
                         .toObject();
    CHECK(ready.value(QStringLiteral("selectionRevision")).toInteger() == 4);
    CHECK(ready.value(QStringLiteral("availabilityRevision")).toInteger() == 2);
    CHECK(ready.value(QStringLiteral("inventoryRevision")).toInteger() == 6);
}

TEST_CASE("ArenaSession reconciles inventory generation across resume",
          "[arena][session][rounds]")
{
    ensureCoreApplication();

    const auto resumeWithRevision = [](Fixture& fixture,
                                       qint64 inventoryRevision) {
        const auto oldGeneration =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectDisconnected(oldGeneration);
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::Reconnecting);
        REQUIRE_FALSE(fixture.identity.ticketRequests.isEmpty());
        fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                       QStringLiteral("resume-ticket"));
        const auto generation =
          fixture.transport.connectCalls.back().generation;
        fixture.transport.injectConnected(generation);
        auto room = phase2RoomSnapshotData();
        room.insert(QStringLiteral("members"),
                    QJsonArray{ phase2Member(QStringLiteral("member-1"),
                                             QStringLiteral("Alice"),
                                             inventoryRevision > 0
                                               ? QStringLiteral("ready")
                                               : QStringLiteral("missing"),
                                             inventoryRevision,
                                             0) });
        fixture.transport.injectText(generation, resumeHello(std::move(room)));
        REQUIRE(fixture.session.getState() ==
                arena::ArenaSession::State::InRoom);
    };

    SECTION("acknowledged commit")
    {
        Fixture fixture;
        fixture.enterPhase2Room();
        fixture.sendInventoryCommit(true);
        REQUIRE(fixture.inventory.requests.size() == 1);
        resumeWithRevision(fixture, 1);
        CHECK(fixture.inventory.requests.size() == 1);
    }

    SECTION("commit acknowledgement lost")
    {
        Fixture fixture;
        fixture.enterPhase2Room();
        fixture.sendInventoryCommit(false);
        REQUIRE(fixture.inventory.requests.size() == 1);
        resumeWithRevision(fixture, 1);
        CHECK(fixture.inventory.requests.size() == 1);
    }

    SECTION("commit did not land")
    {
        Fixture fixture;
        fixture.enterPhase2Room();
        fixture.sendInventoryCommit(false);
        REQUIRE(fixture.inventory.requests.size() == 1);
        resumeWithRevision(fixture, 0);
        CHECK(fixture.inventory.requests.size() == 2);
    }
}

TEST_CASE("ArenaSession recovers from higher-generation inventory responses",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    REQUIRE(fixture.inventory.requests.size() == 1);
    const auto packed = QByteArray(32, '\x66');
    const auto digest = QString::fromLatin1(
      QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
    fixture.inventory.succeed(fixture.inventory.requests.back(), packed);
    const auto begin =
      messageObject(fixture.transport.textCalls.back().message);
    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectText(
      generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("inventory_upload_ready") },
        { QStringLiteral("requestId"),
          begin.value(QStringLiteral("requestId")).toString() },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("roomId"), QStringLiteral("room-1") },
            { QStringLiteral("roomGeneration"), 4 },
            { QStringLiteral("connectionGeneration"), 2 },
            { QStringLiteral("uploadId"),
              QStringLiteral("AAAAAAAAAAAAAAAAAAAAAA") },
            { QStringLiteral("libraryGeneration"), 1 },
            { QStringLiteral("hashCount"), 1 },
            { QStringLiteral("byteCount"), 32 },
            { QStringLiteral("chunkCount"), 1 },
            { QStringLiteral("vectorDigest"), digest },
            { QStringLiteral("deadlineMs"), 60'000 },
          } },
      }));
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Reconnecting);
    CHECK(fixture.session.getErrorCode().isEmpty());
}

TEST_CASE("ArenaSession resumes newest generation after upload rejection",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    REQUIRE(fixture.inventory.requests.size() == 1);
    fixture.inventory.succeed(fixture.inventory.requests.back(),
                              QByteArray(32, '\x77'));
    const auto begin =
      messageObject(fixture.transport.textCalls.back().message);
    const auto beginRequestId =
      begin.value(QStringLiteral("requestId")).toString();
    fixture.inventory.advanceGeneration();
    CHECK(fixture.inventory.requests.size() == 1);

    fixture.transport.injectText(
      fixture.transport.connectCalls.back().generation,
      compact({
        { QStringLiteral("type"), QStringLiteral("command_error") },
        { QStringLiteral("requestId"), beginRequestId },
        { QStringLiteral("data"),
          QJsonObject{
            { QStringLiteral("code"), QStringLiteral("inventory_busy") },
            { QStringLiteral("displayMessageKey"),
              QStringLiteral("arena.error.inventoryBusy") },
          } },
      }));
    REQUIRE(fixture.inventory.requests.size() == 2);
    CHECK(fixture.inventory.currentGeneration == 2);
}

TEST_CASE("ArenaSession retries a failed current library generation",
          "[arena][session][rounds]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterPhase2Room();
    REQUIRE(fixture.inventory.requests.size() == 1);
    fixture.inventory.fail(fixture.inventory.requests.back(),
                           arena::ArenaInventoryFailure::DatabaseError);
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("inventory_build_failed"));
    fixture.session.retry();
    REQUIRE(fixture.inventory.requests.size() == 2);
    CHECK(fixture.inventory.currentGeneration == 1);
    CHECK(fixture.session.getErrorCode().isEmpty());
}

TEST_CASE("ArenaSession gates create on inline login and authenticates first",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.transport.injectText(1, directorySnapshot(1));

    fixture.session.createRoom(QStringLiteral("Private room"),
                               QStringLiteral("secret-password"));
    CHECK(fixture.session.getLoginRequired());
    CHECK(fixture.session.getAdmissionPending());
    CHECK(fixture.identity.ticketRequests.isEmpty());
    CHECK(fixture.transport.connectCalls.size() == 1);

    fixture.identity.setLoggedIn(true);
    REQUIRE(fixture.identity.ticketRequests.size() == 1);
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.front(),
                                   QStringLiteral("short-lived-ticket"));
    REQUIRE(fixture.transport.connectCalls.size() == 2);
    CHECK(fixture.transport.connectCalls.back().generation == 2);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::ConnectingAuthenticated);

    fixture.transport.injectConnected(2);
    REQUIRE(fixture.transport.textCalls.size() == 3);
    const auto ticketHello =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(ticketHello.value(QStringLiteral("type")).toString() ==
          QStringLiteral("client_hello"));
    CHECK(ticketHello.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("ticket"))
            .toString() == QStringLiteral("short-lived-ticket"));

    fixture.transport.injectText(2, serverHello(true));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getAuthenticated());
    CHECK_FALSE(fixture.session.getLoginRequired());
    REQUIRE(fixture.transport.textCalls.size() == 5);
    const auto command =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(command.value(QStringLiteral("type")).toString() ==
          QStringLiteral("room_create"));
    CHECK(command.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("name"))
            .toString() == QStringLiteral("Private room"));
}

TEST_CASE(
  "ArenaSession repairs a directory revision gap with one snapshot request",
  "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.transport.injectText(1, directorySnapshot(4));
    const auto writesAfterSnapshot = fixture.transport.textCalls.size();

    fixture.transport.injectText(
      1,
      directoryDelta(
        4, { roomSummary(QStringLiteral("stale"), QStringLiteral("Stale")) }));
    CHECK(fixture.session.getRooms()->rowCount() == 1);
    fixture.transport.injectText(
      1,
      directoryDelta(
        5,
        { roomSummary(QStringLiteral("room-2"), QStringLiteral("Second")) },
        { QStringLiteral("room-1") }));
    REQUIRE(fixture.session.getRooms()->rowCount() == 1);
    CHECK(fixture.session.getRooms()
            ->data(fixture.session.getRooms()->index(0, 0),
                   arena::ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("room-2"));

    fixture.transport.injectText(
      1,
      directoryDelta(
        7, { roomSummary(QStringLiteral("gap"), QStringLiteral("Gap")) }));
    REQUIRE(fixture.transport.textCalls.size() == writesAfterSnapshot + 1);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("directory_subscribe"));
    fixture.transport.injectText(
      1,
      directoryDelta(
        8, { roomSummary(QStringLiteral("gap-2"), QStringLiteral("Gap 2")) }));
    CHECK(fixture.transport.textCalls.size() == writesAfterSnapshot + 1);
    fixture.transport.injectText(
      1,
      directorySnapshot(8, QStringLiteral("fresh"), QStringLiteral("Fresh")));
    REQUIRE(fixture.session.getRooms()->rowCount() == 1);
    CHECK(fixture.session.getRooms()
            ->data(fixture.session.getRooms()->index(0, 0),
                   arena::ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("fresh"));
}

TEST_CASE("ArenaSession rejects stale transport generations",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.identity.setLoggedIn(true);
    fixture.session.joinRoom(QStringLiteral("room-1"), QString{});
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("ticket"));
    REQUIRE(fixture.transport.connectCalls.size() == 2);
    const auto writes = fixture.transport.textCalls.size();

    fixture.transport.injectConnected(1);
    fixture.transport.injectText(1, serverHello(false));
    fixture.transport.injectBinary(1, QByteArrayLiteral("old"));
    fixture.transport.injectError(
      1, arena::ArenaTransport::Error::ConnectionFailed);
    fixture.transport.injectDisconnected(1);
    CHECK(fixture.transport.textCalls.size() == writes);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::ConnectingAuthenticated);
    CHECK(fixture.scheduler.pendingCount() == 0);

    fixture.transport.injectConnected(2);
    fixture.transport.injectText(2, serverHello(true));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getAuthenticated());
}

TEST_CASE("ArenaSession correlates admission snapshots and preserves wrong "
          "password Browser state",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.transport.injectText(1, directorySnapshot(1));
    fixture.identity.setLoggedIn(true);
    fixture.session.joinRoom(QStringLiteral("room-1"),
                             QStringLiteral("wrong-password"));
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("ticket"));
    fixture.transport.injectConnected(2);
    fixture.transport.injectText(2, serverHello(true));
    const auto command =
      messageObject(fixture.transport.textCalls.back().message);
    const auto requestId =
      command.value(QStringLiteral("requestId")).toString();

    fixture.transport.injectText(
      2, roomSnapshot(QStringLiteral("unknown"), roomSnapshotData()));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    fixture.transport.injectText(
      2,
      compact({ { QStringLiteral("type"), QStringLiteral("command_error") },
                { QStringLiteral("requestId"), requestId },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("code"),
                      QStringLiteral("room_password_invalid") },
                    { QStringLiteral("displayMessageKey"),
                      QStringLiteral("arena.error.roomPasswordInvalid") },
                  } } }));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getAuthenticated());
    CHECK(fixture.session.getRooms()->rowCount() == 1);
    CHECK_FALSE(fixture.session.getAdmissionPending());
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("room_password_invalid"));
    CHECK(fixture.session.getErrorMessageKey() ==
          QStringLiteral("arena.error.roomPasswordInvalid"));
}

TEST_CASE(
  "ArenaSession fully replaces snapshots and applies only current room events",
  "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    const auto requestId = fixture.authenticateAndCreate();
    fixture.transport.injectText(
      2, roomSnapshot(QStringLiteral("wrong"), roomSnapshotData()));
    CHECK(fixture.session.getMembers()->rowCount() == 0);

    auto firstMembers = QJsonArray{
        member(QStringLiteral("member-1"), QStringLiteral("Alice")),
        member(QStringLiteral("member-2"), QStringLiteral("Bob")),
    };
    auto firstChat = QJsonArray{ chatMessage(QStringLiteral("message-1"),
                                             QStringLiteral("member-2"),
                                             QStringLiteral("Bob"),
                                             QStringLiteral("Hello")) };
    fixture.transport.injectText(
      2,
      roomSnapshot(
        requestId,
        roomSnapshotData(
          QStringLiteral("token-1"), 3, 2, firstMembers, firstChat)));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getRoomId() == QStringLiteral("room-1"));
    CHECK(fixture.session.getRoomGeneration() == 3);
    CHECK(fixture.session.getSelfMemberId() == QStringLiteral("member-1"));
    CHECK(fixture.session.getIsOwner());
    CHECK(fixture.session.getMembers()->rowCount() == 2);
    CHECK(fixture.session.getChat()->rowCount() == 1);

    fixture.transport.injectText(
      2,
      compact(
        { { QStringLiteral("type"), QStringLiteral("room_member_joined") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("roomId"), QStringLiteral("other-room") },
              { QStringLiteral("roomGeneration"), 3 },
              { QStringLiteral("member"),
                member(QStringLiteral("ignored"), QStringLiteral("Ignored")) },
            } } }));
    CHECK(fixture.session.getMembers()->rowCount() == 2);

    fixture.transport.injectText(
      2,
      compact(
        { { QStringLiteral("type"), QStringLiteral("room_member_updated") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("roomId"), QStringLiteral("room-1") },
              { QStringLiteral("roomGeneration"), 3 },
              { QStringLiteral("member"),
                member(QStringLiteral("member-2"),
                       QStringLiteral("Bobby"),
                       QStringLiteral("reserved"),
                       2) },
            } } }));
    CHECK(fixture.session.getMembers()
            ->data(fixture.session.getMembers()->index(1, 0),
                   arena::ArenaMemberListModel::DisplayNameRole)
            .toString() == QStringLiteral("Bobby"));
    CHECK_FALSE(fixture.session.getMembers()
                  ->data(fixture.session.getMembers()->index(1, 0),
                         arena::ArenaMemberListModel::ConnectedRole)
                  .toBool());

    fixture.transport.injectText(
      2,
      compact(
        { { QStringLiteral("type"), QStringLiteral("room_owner_changed") },
          { QStringLiteral("data"),
            QJsonObject{
              { QStringLiteral("roomId"), QStringLiteral("room-1") },
              { QStringLiteral("roomGeneration"), 3 },
              { QStringLiteral("ownerMemberId"), QStringLiteral("member-2") },
            } } }));
    CHECK(fixture.session.getOwnerMemberId() == QStringLiteral("member-2"));
    CHECK_FALSE(fixture.session.getIsOwner());

    fixture.transport.injectText(
      2,
      compact({ { QStringLiteral("type"), QStringLiteral("chat_message") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("roomId"), QStringLiteral("room-1") },
                    { QStringLiteral("roomGeneration"), 3 },
                    { QStringLiteral("message"),
                      chatMessage(QStringLiteral("message-2"),
                                  QStringLiteral("member-1"),
                                  QStringLiteral("Alice"),
                                  QStringLiteral("Ready")) },
                  } } }));
    CHECK(fixture.session.getChat()->rowCount() == 2);
}

TEST_CASE(
  "ArenaSession replies to heartbeat and observes leave and kick events",
  "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    const auto writes = fixture.transport.textCalls.size();
    fixture.transport.injectText(
      2,
      compact({ { QStringLiteral("type"), QStringLiteral("server_heartbeat") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("nonce"), QStringLiteral("heartbeat-1") },
                    { QStringLiteral("sentAtMs"), 1000 },
                  } } }));
    REQUIRE(fixture.transport.textCalls.size() == writes + 1);
    const auto reply =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(reply.value(QStringLiteral("type")).toString() ==
          QStringLiteral("heartbeat_reply"));
    CHECK(reply.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("nonce"))
            .toString() == QStringLiteral("heartbeat-1"));

    fixture.session.leaveRoom();
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("room_leave"));
    fixture.transport.injectText(
      2,
      compact({ { QStringLiteral("type"), QStringLiteral("room_member_left") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("roomId"), QStringLiteral("room-1") },
                    { QStringLiteral("roomGeneration"), 3 },
                    { QStringLiteral("memberId"), QStringLiteral("member-1") },
                    { QStringLiteral("reason"), QStringLiteral("left") },
                  } } }));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getAuthenticated());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
}

TEST_CASE("ArenaSession resumes immediately with a fresh ticket and rotates "
          "the seat token",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Reconnecting);
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK(fixture.session.getMembers()->rowCount() == 1);
    REQUIRE(fixture.identity.ticketRequests.size() == 2);

    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("resume-ticket"));
    REQUIRE(fixture.transport.connectCalls.size() == 3);
    fixture.transport.injectConnected(3);
    const auto hello =
      messageObject(fixture.transport.textCalls.back().message);
    const auto resume = hello.value(QStringLiteral("data"))
                          .toObject()
                          .value(QStringLiteral("resume"))
                          .toObject();
    CHECK(resume.value(QStringLiteral("roomId")).toString() ==
          QStringLiteral("room-1"));
    CHECK(resume.value(QStringLiteral("seatToken")).toString() ==
          QStringLiteral("seat-token-1"));

    fixture.transport.injectText(
      3, resumeHello(roomSnapshotData(QStringLiteral("seat-token-2"), 3, 3)));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getAuthenticated());
    CHECK(fixture.session.getRoomGeneration() == 3);

    fixture.transport.injectDisconnected(3);
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("another-ticket"));
    fixture.transport.injectConnected(4);
    const auto rotatedHello =
      messageObject(fixture.transport.textCalls.back().message);
    CHECK(rotatedHello.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("resume"))
            .toObject()
            .value(QStringLiteral("seatToken"))
            .toString() == QStringLiteral("seat-token-2"));
}

TEST_CASE("ArenaSession rejects a resume hello at the exact grace deadline",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("resume-ticket"));
    fixture.transport.injectConnected(3);

    fixture.scheduler.setNowWithoutRunningTasks(60'000);
    fixture.transport.injectText(
      3, resumeHello(roomSnapshotData(QStringLiteral("too-late"), 3, 3)));

    CHECK(fixture.session.getState() != arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.session.getErrorCode() == QStringLiteral("resume_failed"));
    CHECK(fixture.transport.connectCalls.back().generation == 4);
}

TEST_CASE(
  "ArenaSession reconnect backoff is monotonic bounded and expires at grace",
  "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    REQUIRE(fixture.identity.ticketRequests.size() == 2);
    fixture.identity.failTicket(
      fixture.identity.ticketRequests.back(),
      arena::ArenaIdentityProvider::TicketFailure::Network);
    CHECK(fixture.scheduler.pendingCount() == 2);
    fixture.scheduler.advanceBy(499);
    CHECK(fixture.identity.ticketRequests.size() == 2);
    fixture.scheduler.advanceBy(1);
    REQUIRE(fixture.identity.ticketRequests.size() == 3);
    fixture.identity.failTicket(
      fixture.identity.ticketRequests.back(),
      arena::ArenaIdentityProvider::TicketFailure::Network);
    fixture.scheduler.advanceBy(999);
    CHECK(fixture.identity.ticketRequests.size() == 3);
    fixture.scheduler.advanceBy(1);
    CHECK(fixture.identity.ticketRequests.size() == 4);

    fixture.scheduler.advanceTo(60'000);
    CHECK(fixture.session.getState() !=
          arena::ArenaSession::State::Reconnecting);
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getErrorCode() == QStringLiteral("resume_failed"));
    CHECK(fixture.transport.connectCalls.back().generation > 2);
}

TEST_CASE(
  "ArenaSession handles authoritative resume failure without enumeration",
  "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("resume-ticket"));
    fixture.transport.injectConnected(3);
    fixture.transport.injectText(3, failedResumeHello());
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getAuthenticated());
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.session.getErrorCode() == QStringLiteral("resume_failed"));
    CHECK(fixture.session.getErrorMessageKey() ==
          QStringLiteral("arena.error.resumeFailed"));
    CHECK(fixture.scheduler.pendingCount() == 0);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("directory_subscribe"));
}

TEST_CASE("ArenaSession cleans up on logout and exitArena is terminal",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    const auto writesBeforeLogout = fixture.transport.textCalls.size();
    fixture.identity.setLoggedIn(false);
    CHECK(fixture.session.getActive());
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.transport.textCalls.size() == writesBeforeLogout + 1);
    CHECK(messageObject(fixture.transport.textCalls[writesBeforeLogout].message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("room_leave"));

    const auto closeCount = fixture.transport.closeCalls.size();
    fixture.session.exitArena();
    CHECK_FALSE(fixture.session.getActive());
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    CHECK_FALSE(fixture.session.getDirectoryReady());
    CHECK(fixture.session.getRooms()->rowCount() == 0);
    fixture.session.exitArena();
    CHECK(fixture.transport.closeCalls.size() <= closeCount + 1);

    const auto connectCount = fixture.transport.connectCalls.size();
    fixture.transport.injectConnected(99);
    fixture.transport.injectText(99, serverHello(false));
    CHECK(fixture.transport.connectCalls.size() == connectCount);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
}

TEST_CASE("ArenaSession handles binary and server shutdown failures safely",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectBinary(2, QByteArrayLiteral("untrusted-binary"));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Error);
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("unexpected_binary"));
    CHECK(fixture.session.getMembers()->rowCount() == 0);

    Fixture shutdown;
    shutdown.enterRoom();
    shutdown.transport.injectText(
      2,
      compact({ { QStringLiteral("type"), QStringLiteral("server_going_away") },
                { QStringLiteral("data"),
                  QJsonObject{
                    { QStringLiteral("displayMessageKey"),
                      QStringLiteral("arena.serverGoingAway") },
                    { QStringLiteral("retryAfterMs"), 1000 },
                  } } }));
    CHECK(shutdown.session.getState() == arena::ArenaSession::State::Error);
    CHECK(shutdown.session.getRoomId().isEmpty());
    CHECK(shutdown.session.getErrorCode() ==
          QStringLiteral("server_going_away"));
    CHECK(shutdown.session.getErrorMessageKey() ==
          QStringLiteral("arena.serverGoingAway"));
}

TEST_CASE("ArenaSession ignores stale ticket completions after exit",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.session.createRoom(QStringLiteral("Room"), QString{});
    fixture.identity.setLoggedIn(true);
    REQUIRE(fixture.identity.ticketRequests.size() == 1);
    const auto requestId = fixture.identity.ticketRequests.front();
    fixture.session.exitArena();
    const auto connectCount = fixture.transport.connectCalls.size();
    fixture.identity.succeedTicket(requestId, QStringLiteral("stale-ticket"));
    CHECK(fixture.transport.connectCalls.size() == connectCount);
    CHECK_FALSE(fixture.session.getActive());
}

TEST_CASE("ArenaSession restores anonymous browsing after admission transport "
          "failure",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.identity.setLoggedIn(true);
    fixture.session.createRoom(QStringLiteral("Room"),
                               QStringLiteral("private-secret"));
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("short-lived-ticket"));
    REQUIRE(fixture.transport.connectCalls.back().generation == 2);

    fixture.transport.injectError(
      2, arena::ArenaTransport::Error::ConnectionFailed);

    CHECK(fixture.session.getActive());
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK_FALSE(fixture.session.getAdmissionPending());
    CHECK_FALSE(fixture.session.getLoginRequired());
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("transport_connection_failed"));
    REQUIRE(fixture.transport.connectCalls.back().generation == 3);
    fixture.transport.injectConnected(3);
    fixture.transport.injectText(3, serverHello(false));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK_FALSE(fixture.session.getAuthenticated());
}

TEST_CASE("ArenaSession retry is inert outside reconnecting and retryable "
          "error states",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture inRoom;
    inRoom.enterRoom();
    const auto roomConnects = inRoom.transport.connectCalls.size();
    const auto roomCloses = inRoom.transport.closeCalls.size();
    inRoom.session.retry();
    CHECK(inRoom.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(inRoom.session.getRoomId() == QStringLiteral("room-1"));
    CHECK(inRoom.transport.connectCalls.size() == roomConnects);
    CHECK(inRoom.transport.closeCalls.size() == roomCloses);

    Fixture admission;
    admission.browse();
    admission.identity.setLoggedIn(true);
    admission.session.joinRoom(QStringLiteral("room-1"),
                               QStringLiteral("secret"));
    const auto admissionConnects = admission.transport.connectCalls.size();
    admission.session.retry();
    CHECK(admission.session.getState() ==
          arena::ArenaSession::State::ConnectingAuthenticated);
    CHECK(admission.session.getAdmissionPending());
    CHECK(admission.transport.connectCalls.size() == admissionConnects);
}

TEST_CASE("ArenaSession leave while reconnecting abandons the seat locally",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    REQUIRE(fixture.session.getState() ==
            arena::ArenaSession::State::Reconnecting);
    fixture.session.leaveRoom();
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.scheduler.pendingCount() == 0);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    CHECK(fixture.transport.connectCalls.back().generation == 3);
}
