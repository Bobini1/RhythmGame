#include "FakeArenaIdentityProvider.h"
#include "FakeArenaScheduler.h"
#include "FakeArenaTransport.h"
#include "arena/ArenaSession.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QVector>

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "RhythmGameArenaSessionMatrixTests";
    static char* argv[]{ applicationName, nullptr };
    static const auto application =
      std::make_unique<QCoreApplication>(argc, argv);
}

auto
compact(QJsonObject object) -> QString
{
    return QString::fromUtf8(
      QJsonDocument(std::move(object)).toJson(QJsonDocument::Compact));
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
    return compact({ { QStringLiteral("type"), QStringLiteral("server_hello") },
                     { QStringLiteral("data"), data } });
}

auto
member(QString memberId, QString displayName) -> QJsonObject
{
    const auto userId = QStringLiteral("user-for-%1").arg(memberId);
    return {
        { QStringLiteral("memberId"), std::move(memberId) },
        { QStringLiteral("identity"),
          QJsonObject{
            { QStringLiteral("userId"), userId },
            { QStringLiteral("displayName"), std::move(displayName) },
            { QStringLiteral("avatarUrl"), QJsonValue::Null } } },
        { QStringLiteral("status"), QStringLiteral("connected") },
        { QStringLiteral("lobbyWins"), 0 },
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
                 QJsonArray members = {}) -> QJsonObject
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
        { QStringLiteral("chat"), QJsonArray{} },
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
commandError(QString requestId, QString code, QString key) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("command_error") },
      { QStringLiteral("requestId"), std::move(requestId) },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("code"), std::move(code) },
          { QStringLiteral("displayMessageKey"), std::move(key) },
        } },
    });
}

auto
memberLeft(QString memberId,
           QString reason = QStringLiteral("kicked"),
           qint64 roomGeneration = 3) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("room_member_left") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), roomGeneration },
          { QStringLiteral("memberId"), std::move(memberId) },
          { QStringLiteral("reason"), std::move(reason) },
        } },
    });
}

auto
memberJoined(qint64 roomGeneration) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("room_member_joined") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), roomGeneration },
          { QStringLiteral("member"),
            member(QStringLiteral("member-new"),
                   QStringLiteral("New player")) },
        } },
    });
}

auto
chatEvent(QString messageId, QString text) -> QString
{
    return compact({
      { QStringLiteral("type"), QStringLiteral("chat_message") },
      { QStringLiteral("data"),
        QJsonObject{
          { QStringLiteral("roomId"), QStringLiteral("room-1") },
          { QStringLiteral("roomGeneration"), 3 },
          { QStringLiteral("message"),
            chatMessage(std::move(messageId),
                        QStringLiteral("member-1"),
                        QStringLiteral("Alice"),
                        std::move(text)) },
        } },
    });
}

class NonCancellingArenaScheduler final : public arena::ArenaScheduler
{
  public:
    using ArenaScheduler::ArenaScheduler;

    [[nodiscard]] auto monotonicNowMs() const -> qint64 override
    {
        return m_nowMs;
    }

    [[nodiscard]] auto scheduleOnce(qint64 delayMs,
                                    QObject* context,
                                    std::function<void()> callback)
      -> TaskId override
    {
        if (delayMs < 0 || context == nullptr || !callback ||
            delayMs > (std::numeric_limits<qint64>::max)() - m_nowMs) {
            return InvalidTaskId;
        }
        const auto id = m_nextId++;
        m_tasks.push_back(
          { id, m_nowMs + delayMs, context, std::move(callback) });
        return id;
    }

    void cancel(TaskId) override
    {
        // Intentionally retain canceled callbacks to model a callback that was
        // already queued by an event loop when cancellation raced with it.
    }

    void advanceTo(qint64 targetMs)
    {
        REQUIRE(targetMs >= m_nowMs);
        while (true) {
            const auto next =
              std::min_element(m_tasks.begin(),
                               m_tasks.end(),
                               [](const Task& lhs, const Task& rhs) {
                                   return std::pair{ lhs.dueMs, lhs.id } <
                                          std::pair{ rhs.dueMs, rhs.id };
                               });
            if (next == m_tasks.end() || next->dueMs > targetMs) {
                break;
            }
            auto task = std::move(*next);
            m_tasks.erase(next);
            m_nowMs = task.dueMs;
            if (task.context) {
                task.callback();
            }
        }
        m_nowMs = targetMs;
    }

  private:
    struct Task
    {
        TaskId id;
        qint64 dueMs;
        QPointer<QObject> context;
        std::function<void()> callback;
    };

    qint64 m_nowMs{};
    TaskId m_nextId{ 1 };
    QVector<Task> m_tasks;
};

template<typename Scheduler = arena::test::FakeArenaScheduler>
struct Fixture
{
    arena::test::FakeArenaTransport transport;
    arena::test::FakeArenaIdentityProvider identity;
    Scheduler scheduler;
    arena::ArenaSession session{ &transport,
                                 &identity,
                                 &scheduler,
                                 QUrl(QStringLiteral("ws://127.0.0.1:3001/ws")),
                                 QStringLiteral("2026.7.10") };

    void browse()
    {
        session.connectForBrowsing();
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        transport.injectText(generation, serverHello(false));
        REQUIRE(session.getState() == arena::ArenaSession::State::Browsing);
    }

    void enterRoom(QJsonArray members = {})
    {
        browse();
        identity.setLoggedIn(true);
        session.createRoom(QStringLiteral("Arena room"), QString{});
        REQUIRE_FALSE(identity.ticketRequests.isEmpty());
        identity.succeedTicket(identity.ticketRequests.back(),
                               QStringLiteral("admission-ticket"));
        const auto generation = transport.connectCalls.back().generation;
        transport.injectConnected(generation);
        transport.injectText(generation, serverHello(true));
        const auto admission =
          messageObject(transport.textCalls.back().message);
        REQUIRE(admission.value(QStringLiteral("type")).toString() ==
                QStringLiteral("room_create"));
        transport.injectText(
          generation,
          roomSnapshot(admission.value(QStringLiteral("requestId")).toString(),
                       roomSnapshotData(QStringLiteral("seat-token-1"),
                                        3,
                                        generation,
                                        std::move(members))));
        REQUIRE(session.getState() == arena::ArenaSession::State::InRoom);
    }
};

} // namespace

TEST_CASE("ArenaSession active profile replacement leaves the room and "
          "returns to anonymous browsing",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    const auto writesBeforeReplacement = fixture.transport.textCalls.size();

    fixture.identity.replaceActiveProfile(
      true,
      arena::PublicIdentity{ .userId = QStringLiteral("user-2"),
                             .displayName = QStringLiteral("Bob"),
                             .avatarUrl = std::nullopt });

    CHECK(fixture.session.getActive());
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    REQUIRE(fixture.transport.textCalls.size() == writesBeforeReplacement + 1);
    CHECK(messageObject(fixture.transport.textCalls.back().message)
            .value(QStringLiteral("type"))
            .toString() == QStringLiteral("room_leave"));
    REQUIRE(fixture.transport.connectCalls.back().generation == 3);

    fixture.transport.injectConnected(3);
    fixture.transport.injectText(3, serverHello(false));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK_FALSE(fixture.session.getAuthenticated());
}

TEST_CASE("ArenaSession restores anonymous browsing for every admission "
          "ticket failure",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    struct Case
    {
        arena::ArenaIdentityProvider::TicketFailure failure;
        QString code;
        QString key;
    };
    const auto cases = std::array{
        Case{ arena::ArenaIdentityProvider::TicketFailure::NotLoggedIn,
              QStringLiteral("ticket_not_logged_in"),
              QStringLiteral("arena.error.authRequired") },
        Case{ arena::ArenaIdentityProvider::TicketFailure::Network,
              QStringLiteral("ticket_network"),
              QStringLiteral("arena.error.ticketNetwork") },
        Case{ arena::ArenaIdentityProvider::TicketFailure::Rejected,
              QStringLiteral("ticket_rejected"),
              QStringLiteral("arena.error.invalidTicket") },
        Case{ arena::ArenaIdentityProvider::TicketFailure::MalformedResponse,
              QStringLiteral("ticket_malformed_response"),
              QStringLiteral("arena.error.ticketMalformedResponse") },
    };

    for (const auto& testCase : cases) {
        Fixture fixture;
        fixture.browse();
        fixture.identity.setLoggedIn(true);
        fixture.session.joinRoom(QStringLiteral("room-1"),
                                 QStringLiteral("secret"));
        REQUIRE(fixture.identity.ticketRequests.size() == 1);
        fixture.identity.failTicket(fixture.identity.ticketRequests.back(),
                                    testCase.failure);

        CHECK(fixture.session.getActive());
        CHECK_FALSE(fixture.session.getAuthenticated());
        CHECK_FALSE(fixture.session.getAdmissionPending());
        CHECK_FALSE(fixture.session.getLoginRequired());
        CHECK(fixture.session.getErrorCode() == testCase.code);
        CHECK(fixture.session.getErrorMessageKey() == testCase.key);
        CHECK(fixture.session.getState() ==
              arena::ArenaSession::State::Disconnected);
        REQUIRE(fixture.transport.connectCalls.back().generation == 2);
        fixture.transport.injectConnected(2);
        fixture.transport.injectText(2, serverHello(false));
        CHECK(fixture.session.getState() ==
              arena::ArenaSession::State::Browsing);
        CHECK_FALSE(fixture.session.getAuthenticated());
        CHECK(fixture.session.getErrorCode() == testCase.code);
    }
}

TEST_CASE("ArenaSession ignores canceled retry and timeout callbacks after "
          "successful resume",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture<NonCancellingArenaScheduler> fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    REQUIRE(fixture.identity.ticketRequests.size() == 2);
    fixture.identity.failTicket(
      fixture.identity.ticketRequests.back(),
      arena::ArenaIdentityProvider::TicketFailure::Network);

    fixture.session.retry();
    REQUIRE(fixture.identity.ticketRequests.size() == 3);
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("resume-ticket"));
    REQUIRE(fixture.transport.connectCalls.back().generation == 3);
    fixture.transport.injectConnected(3);
    fixture.transport.injectText(
      3, resumeHello(roomSnapshotData(QStringLiteral("rotated-token"), 3, 3)));
    REQUIRE(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    const auto ticketRequests = fixture.identity.ticketRequests.size();
    const auto transportConnections = fixture.transport.connectCalls.size();

    fixture.scheduler.advanceTo(60'000);

    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getRoomId() == QStringLiteral("room-1"));
    CHECK(fixture.identity.ticketRequests.size() == ticketRequests);
    CHECK(fixture.transport.connectCalls.size() == transportConnections);
}

TEST_CASE("ArenaSession reconnect backoff reaches eight seconds and manual "
          "retry bypasses it",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom();
    fixture.transport.injectDisconnected(2);
    REQUIRE(fixture.identity.ticketRequests.size() == 2);

    constexpr auto Delays =
      std::array<qint64, 6>{ 500, 1'000, 2'000, 4'000, 8'000, 8'000 };
    for (const auto delay : Delays) {
        const auto requestsBefore = fixture.identity.ticketRequests.size();
        fixture.identity.failTicket(
          fixture.identity.ticketRequests.back(),
          arena::ArenaIdentityProvider::TicketFailure::Network);
        fixture.scheduler.advanceBy(delay - 1);
        CHECK(fixture.identity.ticketRequests.size() == requestsBefore);
        fixture.scheduler.advanceBy(1);
        CHECK(fixture.identity.ticketRequests.size() == requestsBefore + 1);
    }

    fixture.identity.failTicket(
      fixture.identity.ticketRequests.back(),
      arena::ArenaIdentityProvider::TicketFailure::Network);
    const auto requestsBeforeManualRetry =
      fixture.identity.ticketRequests.size();
    fixture.scheduler.advanceBy(1'000);
    CHECK(fixture.identity.ticketRequests.size() == requestsBeforeManualRetry);
    fixture.session.retry();
    CHECK(fixture.identity.ticketRequests.size() ==
          requestsBeforeManualRetry + 1);

    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("manual-retry-ticket"));
    const auto resumeGeneration =
      fixture.transport.connectCalls.back().generation;
    fixture.transport.injectConnected(resumeGeneration);
    fixture.transport.injectText(
      resumeGeneration,
      resumeHello(
        roomSnapshotData(QStringLiteral("manual-token"), 3, resumeGeneration)));
    REQUIRE(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    const auto requestsAfterResume = fixture.identity.ticketRequests.size();
    fixture.scheduler.advanceBy(8'000);
    CHECK(fixture.identity.ticketRequests.size() == requestsAfterResume);
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
}

TEST_CASE("ArenaSession starts snapshot resynchronization for a higher room "
          "generation",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture fixture;
    auto members = QJsonArray{
        member(QStringLiteral("member-1"), QStringLiteral("Alice")),
        member(QStringLiteral("member-2"), QStringLiteral("Bob")),
    };
    fixture.enterRoom(members);
    REQUIRE(fixture.session.getMembers()->rowCount() == 2);
    const auto ticketRequests = fixture.identity.ticketRequests.size();

    fixture.transport.injectText(2, memberJoined(4));

    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Reconnecting);
    CHECK(fixture.session.getRoomId() == QStringLiteral("room-1"));
    CHECK(fixture.session.getRoomGeneration() == 3);
    CHECK(fixture.session.getMembers()->rowCount() == 2);
    REQUIRE(fixture.identity.ticketRequests.size() == ticketRequests + 1);

    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("resync-ticket"));
    const auto generation = fixture.transport.connectCalls.back().generation;
    fixture.transport.injectConnected(generation);
    fixture.transport.injectText(
      generation,
      resumeHello(roomSnapshotData(
        QStringLiteral("resynced-token"), 4, generation, members)));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
    CHECK(fixture.session.getRoomGeneration() == 4);
    CHECK(fixture.session.getMembers()->rowCount() == 2);
}

TEST_CASE("ArenaSession correlates chat and kick successes and errors",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.enterRoom({
      member(QStringLiteral("member-1"), QStringLiteral("Alice")),
      member(QStringLiteral("member-2"), QStringLiteral("Bob")),
    });

    fixture.session.sendChat(QStringLiteral("Ready"));
    auto command = messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(command.value(QStringLiteral("type")).toString() ==
            QStringLiteral("chat_send"));
    const auto successfulChatRequest =
      command.value(QStringLiteral("requestId")).toString();
    CHECK(command.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("text"))
            .toString() == QStringLiteral("Ready"));
    fixture.transport.injectText(
      2, chatEvent(QStringLiteral("message-1"), QStringLiteral("Ready")));
    REQUIRE(fixture.session.getChat()->rowCount() == 1);
    fixture.transport.injectText(
      2,
      commandError(successfulChatRequest,
                   QStringLiteral("chat_too_long"),
                   QStringLiteral("arena.error.chatTooLong")));
    CHECK(fixture.session.getErrorCode().isEmpty());

    fixture.session.sendChat(QStringLiteral("Too long"));
    command = messageObject(fixture.transport.textCalls.back().message);
    const auto failedChatRequest =
      command.value(QStringLiteral("requestId")).toString();
    fixture.transport.injectText(
      2,
      commandError(failedChatRequest,
                   QStringLiteral("chat_too_long"),
                   QStringLiteral("arena.error.chatTooLong")));
    CHECK(fixture.session.getErrorCode() == QStringLiteral("chat_too_long"));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);

    fixture.session.kickMember(QStringLiteral("member-2"));
    command = messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(command.value(QStringLiteral("type")).toString() ==
            QStringLiteral("room_kick"));
    const auto successfulKickRequest =
      command.value(QStringLiteral("requestId")).toString();
    CHECK(command.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("targetMemberId"))
            .toString() == QStringLiteral("member-2"));
    fixture.transport.injectText(2, memberLeft(QStringLiteral("member-2")));
    REQUIRE(fixture.session.getMembers()->rowCount() == 1);
    fixture.transport.injectText(
      2,
      commandError(successfulKickRequest,
                   QStringLiteral("target_not_found"),
                   QStringLiteral("arena.error.targetNotFound")));
    CHECK(fixture.session.getErrorCode().isEmpty());

    fixture.session.kickMember(QStringLiteral("missing-member"));
    command = messageObject(fixture.transport.textCalls.back().message);
    const auto failedKickRequest =
      command.value(QStringLiteral("requestId")).toString();
    fixture.transport.injectText(
      2,
      commandError(failedKickRequest,
                   QStringLiteral("target_not_found"),
                   QStringLiteral("arena.error.targetNotFound")));
    CHECK(fixture.session.getErrorCode() == QStringLiteral("target_not_found"));
    CHECK(fixture.session.getErrorMessageKey() ==
          QStringLiteral("arena.error.targetNotFound"));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::InRoom);
}

TEST_CASE("ArenaSession clears authenticated admission correlations before "
          "restoring anonymous browsing",
          "[arena][session][matrix]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.browse();
    fixture.identity.setLoggedIn(true);
    fixture.session.joinRoom(QStringLiteral("room-1"),
                             QStringLiteral("secret"));
    fixture.identity.succeedTicket(fixture.identity.ticketRequests.back(),
                                   QStringLiteral("admission-ticket"));
    fixture.transport.injectConnected(2);
    fixture.transport.injectText(2, serverHello(true));
    const auto admission =
      messageObject(fixture.transport.textCalls.back().message);
    REQUIRE(admission.value(QStringLiteral("type")).toString() ==
            QStringLiteral("room_join"));
    const auto requestId =
      admission.value(QStringLiteral("requestId")).toString();

    fixture.transport.injectDisconnected(2);

    CHECK(fixture.session.getActive());
    CHECK_FALSE(fixture.session.getAuthenticated());
    CHECK_FALSE(fixture.session.getAdmissionPending());
    CHECK(fixture.session.getState() ==
          arena::ArenaSession::State::Disconnected);
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("transport_remote_closed"));
    REQUIRE(fixture.transport.connectCalls.back().generation == 3);
    fixture.transport.injectConnected(3);
    fixture.transport.injectText(3, serverHello(false));
    REQUIRE(fixture.session.getState() == arena::ArenaSession::State::Browsing);

    fixture.transport.injectText(
      3,
      commandError(requestId,
                   QStringLiteral("room_password_invalid"),
                   QStringLiteral("arena.error.roomPasswordInvalid")));
    fixture.transport.injectText(3,
                                 roomSnapshot(requestId, roomSnapshotData()));
    CHECK(fixture.session.getState() == arena::ArenaSession::State::Browsing);
    CHECK(fixture.session.getRoomId().isEmpty());
    CHECK(fixture.session.getMembers()->rowCount() == 0);
    CHECK(fixture.session.getErrorCode() ==
          QStringLiteral("transport_remote_closed"));
}
