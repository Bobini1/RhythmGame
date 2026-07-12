#include "FakeArenaIdentityProvider.h"
#include "FakeArenaScheduler.h"
#include "FakeArenaTransport.h"
#include "arena/ArenaChatModel.h"
#include "arena/ArenaMemberListModel.h"
#include "arena/ArenaRoomListModel.h"

#include <catch2/catch_test_macros.hpp>

#include <QList>
#include <QUrl>

#include <memory>

namespace {

auto
room(QString id, QString name = QStringLiteral("Room")) -> arena::RoomSummary
{
    return { .roomId = std::move(id),
             .name = std::move(name),
             .phase = arena::RoomPhase::Selecting,
             .hasPassword = false,
             .connectedCount = 1,
             .reservedCount = 0,
             .maxCount = arena::RoomCapacity };
}

auto
member(QString id,
       QString displayName = QStringLiteral("Alice"),
       arena::MemberStatus status = arena::MemberStatus::Connected)
  -> arena::Member
{
    return { .memberId = std::move(id),
             .identity = { .userId = QStringLiteral("user-1"),
                           .displayName = std::move(displayName),
                           .avatarUrl = std::nullopt },
             .status = status,
             .lobbyWins = 0 };
}

auto
chat(QString id,
     QString author = QStringLiteral("member-1"),
     QString text = QStringLiteral("Hello")) -> arena::ChatMessage
{
    return { .messageId = std::move(id),
             .authorMemberId = std::move(author),
             .authorDisplayName = QStringLiteral("Alice"),
             .sentAtMs = 1000,
             .text = std::move(text) };
}

} // namespace

TEST_CASE("ArenaModels expose exact role contracts", "[arena][models]")
{
    using namespace arena;
    ArenaRoomListModel rooms;
    ArenaMemberListModel members;
    ArenaChatModel chats;

    CHECK(rooms.roleNames() ==
          QHash<int, QByteArray>{
            { ArenaRoomListModel::RoomIdRole, "roomId" },
            { ArenaRoomListModel::NameRole, "name" },
            { ArenaRoomListModel::PhaseRole, "phase" },
            { ArenaRoomListModel::PasswordProtectedRole, "passwordProtected" },
            { ArenaRoomListModel::ConnectedCountRole, "connectedCount" },
            { ArenaRoomListModel::ReservedCountRole, "reservedCount" },
            { ArenaRoomListModel::MaximumCountRole, "maximumCount" },
            { ArenaRoomListModel::MembersRole, "members" },
          });
    CHECK(
      members.roleNames() ==
      QHash<int, QByteArray>{
        { ArenaMemberListModel::MemberIdRole, "memberId" },
        { ArenaMemberListModel::DisplayNameRole, "displayName" },
        { ArenaMemberListModel::AvatarUrlRole, "avatarUrl" },
        { ArenaMemberListModel::ConnectedRole, "connected" },
        { ArenaMemberListModel::OwnerRole, "owner" },
        { ArenaMemberListModel::SelfRole, "self" },
        { ArenaMemberListModel::LobbyWinsRole, "lobbyWins" },
        { ArenaMemberListModel::ReadyRole, "ready" },
        { ArenaMemberListModel::InventoryStateRole, "inventoryState" },
        { ArenaMemberListModel::InventoryRevisionRole, "inventoryRevision" },
        { ArenaMemberListModel::AvailabilityAppliedRevisionRole,
          "availabilityAppliedRevision" },
        { ArenaMemberListModel::RoundStateRole, "roundState" },
      });
    CHECK(chats.roleNames() ==
          QHash<int, QByteArray>{
            { ArenaChatModel::MessageIdRole, "messageId" },
            { ArenaChatModel::MemberIdRole, "memberId" },
            { ArenaChatModel::DisplayNameRole, "displayName" },
            { ArenaChatModel::TextRole, "text" },
            { ArenaChatModel::TimestampRole, "timestamp" },
            { ArenaChatModel::SelfRole, "self" },
          });

    CHECK(rooms.rowCount(rooms.index(0, 0)) == 0);
    CHECK_FALSE(rooms.data({}, ArenaRoomListModel::NameRole).isValid());
    CHECK_FALSE(
      members.data({}, ArenaMemberListModel::DisplayNameRole).isValid());
    CHECK_FALSE(chats.data({}, ArenaChatModel::TextRole).isValid());
}

TEST_CASE("ArenaRoomListModel exposes public member previews",
          "[arena][models][directory-members]")
{
    using namespace arena;
    ArenaRoomListModel rooms;
    auto row = room(QStringLiteral("room-1"));
    row.members = {
        RoomMemberPreview{ .displayName = QStringLiteral("Alice"),
                           .avatarUrl = std::nullopt,
                           .connected = true },
        RoomMemberPreview{ .displayName = QStringLiteral("Bobini"),
                           .avatarUrl = QUrl{ QStringLiteral(
                             "https://example.test/bobini.png") },
                           .connected = false },
    };
    REQUIRE(rooms.replace({ row }));

    const auto members =
      rooms.data(rooms.index(0, 0), ArenaRoomListModel::MembersRole).toList();
    REQUIRE(members.size() == 2);
    CHECK(members[0].toMap().value(QStringLiteral("displayName")).toString() ==
          QStringLiteral("Alice"));
    CHECK(members[0]
            .toMap()
            .value(QStringLiteral("avatarUrl"))
            .toString()
            .isEmpty());
    CHECK(members[0].toMap().value(QStringLiteral("connected")).toBool());
    CHECK(members[1].toMap().value(QStringLiteral("displayName")).toString() ==
          QStringLiteral("Bobini"));
    CHECK(members[1].toMap().value(QStringLiteral("avatarUrl")).toString() ==
          QStringLiteral("https://example.test/bobini.png"));
    CHECK_FALSE(members[1].toMap().value(QStringLiteral("connected")).toBool());
}

TEST_CASE("ArenaModels replace snapshots by value and preserve order",
          "[arena][models]")
{
    using namespace arena;
    ArenaRoomListModel rooms;
    ArenaMemberListModel members;
    ArenaChatModel chats;
    int roomResets = 0;
    int memberResets = 0;
    int chatResets = 0;
    int countChanges = 0;
    QObject::connect(
      &rooms, &QAbstractItemModel::modelReset, [&] { ++roomResets; });
    QObject::connect(
      &members, &QAbstractItemModel::modelReset, [&] { ++memberResets; });
    QObject::connect(
      &chats, &QAbstractItemModel::modelReset, [&] { ++chatResets; });
    QObject::connect(
      &rooms, &ArenaRoomListModel::countChanged, [&] { ++countChanges; });

    QVector<RoomSummary> roomRows{
        room(QStringLiteral("room-2"), QStringLiteral("Second")),
        room(QStringLiteral("room-1"), QStringLiteral("First"))
    };
    QVector<Member> memberRows{ member(QStringLiteral("member-2"),
                                       QStringLiteral("Bob")),
                                member(QStringLiteral("member-1")) };
    QVector<ChatMessage> chatRows{ chat(QStringLiteral("message-2")),
                                   chat(QStringLiteral("message-1")) };
    REQUIRE(rooms.replace(roomRows));
    REQUIRE(members.replace(
      memberRows, QStringLiteral("member-2"), QStringLiteral("member-1")));
    REQUIRE(chats.replace(chatRows, QStringLiteral("member-1")));
    roomRows[0].name = QStringLiteral("Mutated source");
    memberRows.clear();
    chatRows.clear();

    CHECK(roomResets == 1);
    CHECK(memberResets == 1);
    CHECK(chatResets == 1);
    CHECK(countChanges == 1);
    CHECK(
      rooms.data(rooms.index(0, 0), ArenaRoomListModel::NameRole).toString() ==
      QStringLiteral("Second"));
    CHECK(rooms.data(rooms.index(1, 0), ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("room-1"));
    CHECK(members.data(members.index(0, 0), ArenaMemberListModel::OwnerRole)
            .toBool());
    CHECK(members.data(members.index(1, 0), ArenaMemberListModel::SelfRole)
            .toBool());
    CHECK(
      chats.data(chats.index(1, 0), ArenaChatModel::MessageIdRole).toString() ==
      QStringLiteral("message-1"));
}

TEST_CASE("ArenaModels reject duplicate replacement keys without mutation",
          "[arena][models]")
{
    using namespace arena;
    ArenaRoomListModel rooms;
    ArenaMemberListModel members;
    ArenaChatModel chats;
    REQUIRE(rooms.replace({ room(QStringLiteral("keep")) }));
    REQUIRE(members.replace(
      { member(QStringLiteral("keep")) }, std::nullopt, QString{}));
    REQUIRE(chats.replace({ chat(QStringLiteral("keep")) }, QString{}));

    CHECK_FALSE(rooms.replace({ room(QStringLiteral("duplicate")),
                                room(QStringLiteral("duplicate")) }));
    CHECK_FALSE(members.replace({ member(QStringLiteral("duplicate")),
                                  member(QStringLiteral("duplicate")) },
                                std::nullopt,
                                QString{}));
    CHECK_FALSE(chats.replace(
      { chat(QStringLiteral("duplicate")), chat(QStringLiteral("duplicate")) },
      QString{}));
    CHECK(rooms.rowCount() == 1);
    CHECK(members.rowCount() == 1);
    CHECK(chats.rowCount() == 1);
    CHECK(rooms.data(rooms.index(0, 0), ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("keep"));
}

TEST_CASE("ArenaModels apply room deltas by opaque ID", "[arena][models]")
{
    using namespace arena;
    ArenaRoomListModel model;
    REQUIRE(model.replace({ room(QStringLiteral("a"), QStringLiteral("A")),
                            room(QStringLiteral("b"), QStringLiteral("B")) }));
    QList<int> changedRoles;
    QObject::connect(&model,
                     &QAbstractItemModel::dataChanged,
                     [&](const QModelIndex&,
                         const QModelIndex&,
                         const QList<int>& roles) { changedRoles = roles; });
    auto updated = room(QStringLiteral("b"), QStringLiteral("B2"));
    updated.connectedCount = 2;
    REQUIRE(model.applyDelta({ updated, room(QStringLiteral("c")) },
                             { QStringLiteral("a") }));
    CHECK(model.rowCount() == 2);
    CHECK(model.data(model.index(0, 0), ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("b"));
    CHECK(model.data(model.index(1, 0), ArenaRoomListModel::RoomIdRole)
            .toString() == QStringLiteral("c"));
    CHECK(changedRoles == QList<int>{ ArenaRoomListModel::NameRole,
                                      ArenaRoomListModel::ConnectedCountRole });

    CHECK_FALSE(model.applyDelta({ room(QStringLiteral("same")) },
                                 { QStringLiteral("same") }));
    CHECK_FALSE(
      model.applyDelta({}, { QStringLiteral("x"), QStringLiteral("x") }));
    CHECK(model.rowCount() == 2);
    CHECK_FALSE(model.remove(QStringLiteral("unknown")));
}

TEST_CASE("ArenaModels update member owner self and connectivity roles tightly",
          "[arena][models]")
{
    using namespace arena;
    ArenaMemberListModel model;
    REQUIRE(
      model.replace({ member(QStringLiteral("one")),
                      member(QStringLiteral("two"), QStringLiteral("Bob")) },
                    QStringLiteral("one"),
                    QStringLiteral("one")));
    QVector<QList<int>> emissions;
    QObject::connect(
      &model,
      &QAbstractItemModel::dataChanged,
      [&](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
          emissions.push_back(roles);
      });

    model.setOwnerMemberId(QStringLiteral("two"));
    REQUIRE(emissions.size() == 2);
    CHECK(emissions[0] == QList<int>{ ArenaMemberListModel::OwnerRole });
    CHECK(emissions[1] == QList<int>{ ArenaMemberListModel::OwnerRole });
    emissions.clear();
    model.setSelfMemberId(QStringLiteral("two"));
    REQUIRE(emissions.size() == 2);
    CHECK(emissions[0] == QList<int>{ ArenaMemberListModel::SelfRole });

    emissions.clear();
    auto changed = member(
      QStringLiteral("two"), QStringLiteral("Bobby"), MemberStatus::Reserved);
    changed.lobbyWins = 3;
    model.upsert(changed);
    REQUIRE(emissions.size() == 1);
    CHECK(emissions.front() ==
          QList<int>{ ArenaMemberListModel::DisplayNameRole,
                      ArenaMemberListModel::ConnectedRole,
                      ArenaMemberListModel::LobbyWinsRole });
    CHECK_FALSE(
      model.data(model.index(1, 0), ArenaMemberListModel::ConnectedRole)
        .toBool());
    CHECK(model.remove(QStringLiteral("one")));
    model.clear();
    CHECK(model.rowCount() == 0);
}

TEST_CASE("ArenaModels expose Phase 2 room and member state as exact roles",
          "[arena][models]")
{
    using namespace arena;
    ArenaRoomListModel rooms;
    ArenaMemberListModel members;

    auto loadingRoom = room(QStringLiteral("room"));
    loadingRoom.phase = RoomPhase::Loading;
    REQUIRE(rooms.replace({ loadingRoom }));
    CHECK(
      rooms.data(rooms.index(0, 0), ArenaRoomListModel::PhaseRole).toString() ==
      QStringLiteral("loading"));

    auto row = member(QStringLiteral("member"));
    REQUIRE(members.replace({ row }, std::nullopt, QString{}));
    CHECK_FALSE(
      members.data(members.index(0, 0), ArenaMemberListModel::ReadyRole)
        .toBool());
    CHECK(members
            .data(members.index(0, 0), ArenaMemberListModel::InventoryStateRole)
            .toString() == QStringLiteral("missing"));
    CHECK(
      members
        .data(members.index(0, 0), ArenaMemberListModel::InventoryRevisionRole)
        .toLongLong() == 0);
    CHECK(members
            .data(members.index(0, 0),
                  ArenaMemberListModel::AvailabilityAppliedRevisionRole)
            .toLongLong() == 0);
    CHECK(
      members.data(members.index(0, 0), ArenaMemberListModel::RoundStateRole)
        .toString() == QStringLiteral("eligible"));

    QVector<QList<int>> memberChanges;
    QObject::connect(
      &members,
      &QAbstractItemModel::dataChanged,
      [&](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
          memberChanges.push_back(roles);
      });
    row.ready = true;
    row.inventoryState = InventoryState::Ready;
    row.inventoryRevision = 7;
    row.availabilityAppliedRevision = 9;
    row.roundState = MemberRoundState::Loading;
    members.upsert(row);
    REQUIRE(memberChanges.size() == 1);
    CHECK(memberChanges.front() ==
          QList<int>{ ArenaMemberListModel::ReadyRole,
                      ArenaMemberListModel::InventoryStateRole,
                      ArenaMemberListModel::InventoryRevisionRole,
                      ArenaMemberListModel::AvailabilityAppliedRevisionRole,
                      ArenaMemberListModel::RoundStateRole });

    QList<int> roomChanges;
    QObject::connect(&rooms,
                     &QAbstractItemModel::dataChanged,
                     [&](const QModelIndex&,
                         const QModelIndex&,
                         const QList<int>& roles) { roomChanges = roles; });
    loadingRoom.phase = RoomPhase::Playing;
    rooms.upsert(loadingRoom);
    CHECK(roomChanges == QList<int>{ ArenaRoomListModel::PhaseRole });
    CHECK(
      rooms.data(rooms.index(0, 0), ArenaRoomListModel::PhaseRole).toString() ==
      QStringLiteral("playing"));
}

TEST_CASE("ArenaModels keep chat ordered bounded and self-aware",
          "[arena][models]")
{
    using namespace arena;
    ArenaChatModel model;
    QVector<ChatMessage> initial;
    initial.reserve(MaxWireChatBacklog);
    for (int i = 0; i < MaxWireChatBacklog; ++i) {
        initial.push_back(chat(QStringLiteral("message-%1").arg(i)));
    }
    REQUIRE(model.replace(initial, QStringLiteral("member-1")));
    model.upsert(
      chat(QStringLiteral("new-message"), QStringLiteral("member-2")));
    CHECK(model.rowCount() == MaxWireChatBacklog);
    CHECK(
      model.data(model.index(0, 0), ArenaChatModel::MessageIdRole).toString() ==
      QStringLiteral("message-1"));
    CHECK(model
            .data(model.index(MaxWireChatBacklog - 1, 0),
                  ArenaChatModel::MessageIdRole)
            .toString() == QStringLiteral("new-message"));

    auto changed = chat(QStringLiteral("new-message"),
                        QStringLiteral("member-1"),
                        QStringLiteral("Changed"));
    model.upsert(changed);
    CHECK(
      model
        .data(model.index(MaxWireChatBacklog - 1, 0), ArenaChatModel::TextRole)
        .toString() == QStringLiteral("Changed"));
    CHECK(
      model
        .data(model.index(MaxWireChatBacklog - 1, 0), ArenaChatModel::SelfRole)
        .toBool());
    model.setSelfMemberId(QStringLiteral("member-2"));
    CHECK_FALSE(
      model
        .data(model.index(MaxWireChatBacklog - 1, 0), ArenaChatModel::SelfRole)
        .toBool());
    CHECK(model.remove(QStringLiteral("new-message")));
    model.clear();
    CHECK(model.rowCount() == 0);
}

TEST_CASE("ArenaModels fake transport records writes and injects events",
          "[arena][models]")
{
    using namespace arena;
    using namespace arena::test;
    FakeArenaTransport transport;
    ArenaTransport::Generation connectedGeneration = 0;
    ArenaTransport::Generation textGeneration = 0;
    QString received;
    QObject::connect(
      &transport, &ArenaTransport::connected, [&](auto generation) {
          connectedGeneration = generation;
      });
    QObject::connect(&transport,
                     &ArenaTransport::textReceived,
                     [&](auto generation, const QString& text) {
                         textGeneration = generation;
                         received = text;
                     });
    transport.connectTo(3, QUrl(QStringLiteral("ws://127.0.0.1:3001/ws")));
    transport.sendText(3, QStringLiteral("hello"));
    transport.sendBinary(3, QByteArray("binary"));
    transport.close(3);
    transport.injectConnected(2);
    transport.injectText(1, QStringLiteral("late"));

    REQUIRE(transport.connectCalls.size() == 1);
    CHECK(transport.connectCalls.front().generation == 3);
    CHECK(transport.textCalls.front().generation == 3);
    CHECK(transport.binaryCalls.front().generation == 3);
    CHECK(transport.closeCalls == QVector<ArenaTransport::Generation>{ 3 });
    CHECK(connectedGeneration == 2);
    CHECK(textGeneration == 1);
    CHECK(received == QStringLiteral("late"));
}

TEST_CASE("ArenaModels fake identity correlates late ticket completions",
          "[arena][models]")
{
    using namespace arena;
    using namespace arena::test;
    FakeArenaIdentityProvider identity;
    QVector<quint64> successes;
    QVector<quint64> failures;
    int profileChanges = 0;
    QObject::connect(
      &identity,
      &ArenaIdentityProvider::ticketReady,
      [&](quint64 id, const QString&) { successes.push_back(id); });
    QObject::connect(&identity,
                     &ArenaIdentityProvider::ticketFailed,
                     [&](quint64 id, auto) { failures.push_back(id); });
    QObject::connect(&identity,
                     &ArenaIdentityProvider::activeProfileChanged,
                     [&] { ++profileChanges; });
    identity.requestTicket(1);
    identity.requestTicket(2);
    identity.succeedTicket(1, QStringLiteral("secret-ticket"));
    identity.failTicket(2, ArenaIdentityProvider::TicketFailure::Network);
    identity.replaceActiveProfile(
      true,
      PublicIdentity{ .userId = QStringLiteral("u"),
                      .displayName = QStringLiteral("Alice") });
    CHECK(identity.ticketRequests == QVector<quint64>{ 1, 2 });
    CHECK(successes == QVector<quint64>{ 1 });
    CHECK(failures == QVector<quint64>{ 2 });
    CHECK(profileChanges == 1);
}

TEST_CASE("ArenaModels fake scheduler is deterministic and cancellable",
          "[arena][models]")
{
    using namespace arena;
    using namespace arena::test;
    FakeArenaScheduler scheduler;
    QObject context;
    QVector<int> order;
    const auto cancelled =
      scheduler.scheduleOnce(5, &context, [&] { order.push_back(99); });
    scheduler.cancel(cancelled);
    REQUIRE(scheduler.scheduleOnce(10, &context, [&] { order.push_back(3); }) !=
            ArenaScheduler::InvalidTaskId);
    REQUIRE(scheduler.scheduleOnce(5, &context, [&] {
        order.push_back(1);
        CHECK(scheduler.scheduleOnce(0, &context, [&] {
            order.push_back(2);
        }) != ArenaScheduler::InvalidTaskId);
    }) != ArenaScheduler::InvalidTaskId);
    auto destroyed = std::make_unique<QObject>();
    REQUIRE(scheduler.scheduleOnce(2, destroyed.get(), [&] {
        order.push_back(98);
    }) != ArenaScheduler::InvalidTaskId);
    destroyed.reset();
    CHECK(scheduler.scheduleOnce(-1, &context, [] {}) ==
          ArenaScheduler::InvalidTaskId);
    CHECK(scheduler.scheduleOnce(1, nullptr, [] {}) ==
          ArenaScheduler::InvalidTaskId);
    scheduler.advanceTo(10);
    CHECK(order == QVector<int>{ 1, 2, 3 });
    CHECK(scheduler.monotonicNowMs() == 10);
    CHECK(scheduler.pendingCount() == 0);
    scheduler.advanceTo(5);
    CHECK(scheduler.monotonicNowMs() == 10);
}
