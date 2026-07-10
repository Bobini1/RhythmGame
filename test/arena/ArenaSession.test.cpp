#include "FakeArenaIdentityProvider.h"
#include "FakeArenaScheduler.h"
#include "FakeArenaTransport.h"
#include "arena/ArenaSession.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>

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
        { QStringLiteral("protocolMinor"), 0 },
        { QStringLiteral("capabilities"),
          QJsonArray{ QStringLiteral("rooms-v1") } },
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
          { QStringLiteral("protocolMinor"), 0 },
          { QStringLiteral("capabilities"),
            QJsonArray{ QStringLiteral("rooms-v1") } },
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
          { QStringLiteral("protocolMinor"), 0 },
          { QStringLiteral("capabilities"),
            QJsonArray{ QStringLiteral("rooms-v1") } },
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
    arena::ArenaSession session{ &transport,
                                 &identity,
                                 &scheduler,
                                 QUrl(QStringLiteral("ws://127.0.0.1:3001/ws")),
                                 QStringLiteral("2026.7.10") };

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
