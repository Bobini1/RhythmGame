#include "ArenaSession.h"

#include "ArenaProtocol.h"

#include <QByteArray>

#include <algorithm>
#include <limits>
#include <type_traits>
#include <utility>

namespace arena {
namespace {

template<class... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

auto
commandCode(CommandErrorCode code) -> QString
{
    switch (code) {
        case CommandErrorCode::AuthRequired:
            return QStringLiteral("auth_required");
        case CommandErrorCode::AlreadyInRoom:
            return QStringLiteral("already_in_room");
        case CommandErrorCode::NotInRoom:
            return QStringLiteral("not_in_room");
        case CommandErrorCode::RoomNotFound:
            return QStringLiteral("room_not_found");
        case CommandErrorCode::RoomPasswordInvalid:
            return QStringLiteral("room_password_invalid");
        case CommandErrorCode::RoomFull:
            return QStringLiteral("room_full");
        case CommandErrorCode::RoomBanned:
            return QStringLiteral("room_banned");
        case CommandErrorCode::RoomDuplicateIdentity:
            return QStringLiteral("room_duplicate_identity");
        case CommandErrorCode::RoomGenerationStale:
            return QStringLiteral("room_generation_stale");
        case CommandErrorCode::ConnectionGenerationStale:
            return QStringLiteral("connection_generation_stale");
        case CommandErrorCode::PermissionDenied:
            return QStringLiteral("permission_denied");
        case CommandErrorCode::TargetNotFound:
            return QStringLiteral("target_not_found");
        case CommandErrorCode::CannotKickSelf:
            return QStringLiteral("cannot_kick_self");
        case CommandErrorCode::ChatEmpty:
            return QStringLiteral("chat_empty");
        case CommandErrorCode::ChatTooLong:
            return QStringLiteral("chat_too_long");
        case CommandErrorCode::RateLimited:
            return QStringLiteral("rate_limited");
    }
    return {};
}

auto
fatalCode(FatalErrorCode code) -> QString
{
    switch (code) {
        case FatalErrorCode::MalformedMessage:
            return QStringLiteral("malformed_message");
        case FatalErrorCode::FrameTooLarge:
            return QStringLiteral("frame_too_large");
        case FatalErrorCode::UnexpectedBinary:
            return QStringLiteral("unexpected_binary");
        case FatalErrorCode::HelloRequired:
            return QStringLiteral("hello_required");
        case FatalErrorCode::HelloRepeated:
            return QStringLiteral("hello_repeated");
        case FatalErrorCode::ProtocolIncompatible:
            return QStringLiteral("protocol_incompatible");
        case FatalErrorCode::CapabilityRequired:
            return QStringLiteral("capability_required");
        case FatalErrorCode::InvalidTicket:
            return QStringLiteral("invalid_ticket");
        case FatalErrorCode::TicketReplayed:
            return QStringLiteral("ticket_replayed");
        case FatalErrorCode::ServerShuttingDown:
            return QStringLiteral("server_shutting_down");
    }
    return {};
}

auto
protocolCode(ProtocolFailureCode code) -> QString
{
    switch (code) {
        case ProtocolFailureCode::MalformedMessage:
            return QStringLiteral("malformed_message");
        case ProtocolFailureCode::FrameTooLarge:
            return QStringLiteral("frame_too_large");
        case ProtocolFailureCode::ProtocolIncompatible:
            return QStringLiteral("protocol_incompatible");
        case ProtocolFailureCode::CapabilityRequired:
            return QStringLiteral("capability_required");
    }
    return {};
}

auto
ticketFailureCode(ArenaIdentityProvider::TicketFailure failure)
  -> std::pair<QString, QString>
{
    switch (failure) {
        case ArenaIdentityProvider::TicketFailure::NotLoggedIn:
            return { QStringLiteral("ticket_not_logged_in"),
                     QStringLiteral("arena.error.authRequired") };
        case ArenaIdentityProvider::TicketFailure::Network:
            return { QStringLiteral("ticket_network"),
                     QStringLiteral("arena.error.ticketNetwork") };
        case ArenaIdentityProvider::TicketFailure::Rejected:
            return { QStringLiteral("ticket_rejected"),
                     QStringLiteral("arena.error.invalidTicket") };
        case ArenaIdentityProvider::TicketFailure::MalformedResponse:
            return { QStringLiteral("ticket_malformed_response"),
                     QStringLiteral("arena.error.ticketMalformedResponse") };
    }
    return {};
}

auto
transportFailureCode(ArenaTransport::Error error) -> std::pair<QString, QString>
{
    switch (error) {
        case ArenaTransport::Error::ConnectionFailed:
            return { QStringLiteral("transport_connection_failed"),
                     QStringLiteral("arena.error.connectionFailed") };
        case ArenaTransport::Error::RemoteClosed:
            return { QStringLiteral("transport_remote_closed"),
                     QStringLiteral("arena.error.remoteClosed") };
        case ArenaTransport::Error::TlsFailed:
            return { QStringLiteral("transport_tls_failed"),
                     QStringLiteral("arena.error.tlsFailed") };
        case ArenaTransport::Error::Other:
            return { QStringLiteral("transport_error"),
                     QStringLiteral("arena.error.transport") };
    }
    return {};
}

} // namespace

ArenaSession::ArenaSession(ArenaTransport* transport,
                           ArenaIdentityProvider* identityProvider,
                           ArenaScheduler* scheduler,
                           QUrl endpoint,
                           QString clientVersion,
                           QObject* parent)
  : QObject(parent)
  , m_transport(transport)
  , m_identityProvider(identityProvider)
  , m_scheduler(scheduler)
  , m_endpoint(std::move(endpoint))
  , m_clientVersion(std::move(clientVersion))
  , m_rooms(this)
  , m_members(this)
  , m_chat(this)
  , m_lastLoggedIn(identityProvider != nullptr && identityProvider->loggedIn())
{
    Q_ASSERT(m_transport != nullptr);
    Q_ASSERT(m_identityProvider != nullptr);
    Q_ASSERT(m_scheduler != nullptr);
    Q_ASSERT(m_endpoint.isValid());
    Q_ASSERT(m_endpoint.scheme() == QStringLiteral("ws") ||
             m_endpoint.scheme() == QStringLiteral("wss"));

    connect(m_transport,
            &ArenaTransport::connected,
            this,
            &ArenaSession::handleConnected);
    connect(m_transport,
            &ArenaTransport::disconnected,
            this,
            &ArenaSession::handleDisconnected);
    connect(m_transport,
            &ArenaTransport::transportError,
            this,
            &ArenaSession::handleTransportError);
    connect(m_transport,
            &ArenaTransport::textReceived,
            this,
            &ArenaSession::handleText);
    connect(m_transport,
            &ArenaTransport::binaryReceived,
            this,
            &ArenaSession::handleBinary);
    connect(m_identityProvider,
            &ArenaIdentityProvider::ticketReady,
            this,
            &ArenaSession::handleTicketReady);
    connect(m_identityProvider,
            &ArenaIdentityProvider::ticketFailed,
            this,
            &ArenaSession::handleTicketFailure);
    connect(m_identityProvider,
            &ArenaIdentityProvider::activeProfileChanged,
            this,
            &ArenaSession::handleActiveProfileChanged);
    connect(m_identityProvider,
            &ArenaIdentityProvider::loginStateChanged,
            this,
            &ArenaSession::handleLoginStateChanged);
}

ArenaSession::~ArenaSession()
{
    ++m_lifecycleGeneration;
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    invalidateTransport();
}

auto
ArenaSession::getState() const -> State
{
    return m_state;
}
auto
ArenaSession::getActive() const -> bool
{
    return m_active;
}
auto
ArenaSession::getAuthenticated() const -> bool
{
    return m_authenticated;
}
auto
ArenaSession::getLoginRequired() const -> bool
{
    return m_loginRequired;
}
auto
ArenaSession::getReconnecting() const -> bool
{
    return m_state == State::Reconnecting;
}
auto
ArenaSession::getDirectoryReady() const -> bool
{
    return m_directoryReady;
}
auto
ArenaSession::getAdmissionPending() const -> bool
{
    return !std::holds_alternative<std::monostate>(m_pendingAdmission);
}
auto
ArenaSession::getErrorCode() const -> QString
{
    return m_errorCode;
}
auto
ArenaSession::getErrorMessageKey() const -> QString
{
    return m_errorMessageKey;
}
auto
ArenaSession::getRoomId() const -> QString
{
    return m_roomId;
}
auto
ArenaSession::getRoomName() const -> QString
{
    return m_roomName;
}
auto
ArenaSession::getRoomGeneration() const -> qint64
{
    return m_roomGeneration;
}
auto
ArenaSession::getSelfMemberId() const -> QString
{
    return m_selfMemberId;
}
auto
ArenaSession::getOwnerMemberId() const -> QString
{
    return m_ownerMemberId.value_or(QString{});
}
auto
ArenaSession::getIsOwner() const -> bool
{
    return !m_selfMemberId.isEmpty() && m_ownerMemberId &&
           m_selfMemberId == *m_ownerMemberId;
}
auto
ArenaSession::getRooms() -> ArenaRoomListModel*
{
    return &m_rooms;
}
auto
ArenaSession::getMembers() -> ArenaMemberListModel*
{
    return &m_members;
}
auto
ArenaSession::getChat() -> ArenaChatModel*
{
    return &m_chat;
}

void
ArenaSession::setState(State state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged();
}

void
ArenaSession::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
}

void
ArenaSession::setAuthenticated(bool authenticated)
{
    if (m_authenticated == authenticated) {
        return;
    }
    m_authenticated = authenticated;
    emit authenticatedChanged();
}

void
ArenaSession::setLoginRequired(bool required)
{
    if (m_loginRequired == required) {
        return;
    }
    m_loginRequired = required;
    emit loginRequiredChanged();
}

void
ArenaSession::setDirectoryReady(bool ready)
{
    if (m_directoryReady == ready) {
        return;
    }
    m_directoryReady = ready;
    emit directoryReadyChanged();
}

void
ArenaSession::clearError()
{
    if (m_errorCode.isEmpty() && m_errorMessageKey.isEmpty()) {
        return;
    }
    m_errorCode.clear();
    m_errorMessageKey.clear();
    emit errorChanged();
}

void
ArenaSession::setError(QString code, QString messageKey)
{
    if (m_errorCode == code && m_errorMessageKey == messageKey) {
        return;
    }
    m_errorCode = std::move(code);
    m_errorMessageKey = std::move(messageKey);
    emit errorChanged();
}

void
ArenaSession::setPendingAdmission(PendingAdmission admission)
{
    const auto wasPending = getAdmissionPending();
    m_pendingAdmission = std::move(admission);
    if (wasPending != getAdmissionPending()) {
        emit admissionPendingChanged();
    }
}

void
ArenaSession::clearPendingAdmission()
{
    setPendingAdmission(std::monostate{});
    m_pendingAdmissionRequestId.clear();
    setLoginRequired(false);
}

void
ArenaSession::restoreAnonymousAfterAdmissionFailure(QString code,
                                                    QString messageKey)
{
    invalidateAsyncWork();
    clearPendingAdmission();
    invalidateTransport();
    clearRoom();
    setAuthenticated(false);
    setError(std::move(code), std::move(messageKey));
    openAnonymousBrowsing();
}

void
ArenaSession::connectForBrowsing()
{
    if (m_active) {
        return;
    }
    setActive(true);
    clearError();
    openAnonymousBrowsing();
}

void
ArenaSession::startTransport(HandshakeKind kind)
{
    invalidateTransport();
    m_protocolReady = false;
    m_handshakeKind = kind;
    m_directoryRevision.reset();
    m_directoryResyncPending = false;
    setDirectoryReady(false);
    auto generation = m_nextTransportGeneration++;
    if (generation == ArenaTransport::InvalidGeneration) {
        generation = m_nextTransportGeneration++;
    }
    m_currentTransportGeneration = generation;
    m_transport->connectTo(generation, m_endpoint);
}

void
ArenaSession::invalidateTransport()
{
    const auto generation = std::exchange(m_currentTransportGeneration,
                                          ArenaTransport::InvalidGeneration);
    m_protocolReady = false;
    m_handshakeKind = HandshakeKind::None;
    if (generation != ArenaTransport::InvalidGeneration) {
        m_transport->close(generation);
    }
}

void
ArenaSession::openAnonymousBrowsing()
{
    if (!m_active) {
        return;
    }
    setAuthenticated(false);
    setState(State::Disconnected);
    startTransport(HandshakeKind::AnonymousBrowse);
}

auto
ArenaSession::sendMessage(const ClientMessage& message) -> bool
{
    if (m_currentTransportGeneration == ArenaTransport::InvalidGeneration) {
        return false;
    }
    const auto encoded = encodeClientMessage(message);
    if (const auto* failure = std::get_if<ProtocolFailure>(&encoded)) {
        failProtocol(failure->code);
        return false;
    }
    m_transport->sendText(m_currentTransportGeneration,
                          std::get<QString>(encoded));
    return true;
}

void
ArenaSession::sendDirectorySubscribe()
{
    if (m_protocolReady) {
        (void)sendMessage(DirectorySubscribe{});
    }
}

auto
ArenaSession::nextRequestId() -> QString
{
    return QStringLiteral("rg-%1").arg(m_nextRequestId++);
}

void
ArenaSession::handleConnected(ArenaTransport::Generation generation)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    if (m_handshakeKind == HandshakeKind::Resume && resumeDeadlineReached()) {
        failResumeAtDeadline();
        return;
    }
    ClientHello hello{ .clientVersion = m_clientVersion };
    if (m_handshakeKind == HandshakeKind::AuthenticatedAdmission ||
        m_handshakeKind == HandshakeKind::Resume) {
        if (m_pendingTicket.isEmpty()) {
            failProtocol(QStringLiteral("invalid_ticket"),
                         QStringLiteral("arena.error.invalidTicket"));
            return;
        }
        hello.ticket = m_pendingTicket;
    }
    if (m_handshakeKind == HandshakeKind::Resume) {
        if (m_roomId.isEmpty() || m_resumeToken.isEmpty()) {
            failResumeAtDeadline();
            return;
        }
        hello.resume =
          ResumeRequest{ .roomId = m_roomId, .seatToken = m_resumeToken };
    }
    (void)sendMessage(hello);
    m_pendingTicket.clear();
    m_pendingTicket.squeeze();
}

void
ArenaSession::handleText(ArenaTransport::Generation generation,
                         const QString& text)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    const auto decoded = decodeServerMessage(text);
    if (const auto* failure = std::get_if<ProtocolFailure>(&decoded)) {
        failProtocol(failure->code);
        return;
    }
    handleServerMessage(std::get<ServerMessage>(decoded));
}

void
ArenaSession::handleBinary(ArenaTransport::Generation generation,
                           const QByteArray&)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    failProtocol(QStringLiteral("unexpected_binary"),
                 QStringLiteral("arena.error.unexpectedBinary"));
}

void
ArenaSession::handleServerHello(const ServerHello& hello)
{
    if (m_protocolReady) {
        failProtocol(QStringLiteral("hello_repeated"),
                     QStringLiteral("arena.error.helloRepeated"));
        return;
    }
    const auto kind = m_handshakeKind;
    if (kind == HandshakeKind::Resume && resumeDeadlineReached()) {
        failResumeAtDeadline();
        return;
    }
    cancelTask(m_attemptTimeoutTask);
    if (kind == HandshakeKind::AnonymousBrowse) {
        if (hello.identity ||
            !std::holds_alternative<ResumeNotRequested>(hello.resume)) {
            failProtocol(ProtocolFailureCode::MalformedMessage);
            return;
        }
        m_protocolReady = true;
        m_handshakeKind = HandshakeKind::None;
        setAuthenticated(false);
        setState(State::Browsing);
        sendDirectorySubscribe();
        return;
    }
    if (kind == HandshakeKind::AuthenticatedAdmission) {
        if (!hello.identity ||
            !std::holds_alternative<ResumeNotRequested>(hello.resume)) {
            failProtocol(ProtocolFailureCode::MalformedMessage);
            return;
        }
        m_protocolReady = true;
        m_handshakeKind = HandshakeKind::None;
        setAuthenticated(true);
        setLoginRequired(false);
        setState(State::Browsing);
        sendDirectorySubscribe();
        sendPendingAdmission();
        return;
    }
    if (kind != HandshakeKind::Resume || !hello.identity) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    m_protocolReady = true;
    m_handshakeKind = HandshakeKind::None;
    setAuthenticated(true);
    if (const auto* succeeded = std::get_if<ResumeSucceeded>(&hello.resume)) {
        applyRoomSnapshot(succeeded->room);
        sendDirectorySubscribe();
        return;
    }
    if (std::holds_alternative<ResumeFailed>(hello.resume)) {
        cancelReconnectTasks();
        clearRoom();
        setError(QStringLiteral("resume_failed"),
                 QStringLiteral("arena.error.resumeFailed"));
        setState(State::Browsing);
        sendDirectorySubscribe();
        return;
    }
    failProtocol(ProtocolFailureCode::MalformedMessage);
}

void
ArenaSession::handleDirectorySnapshot(const DirectorySnapshot& snapshot)
{
    if (m_directoryRevision && snapshot.revision < *m_directoryRevision) {
        return;
    }
    if (!m_rooms.replace(snapshot.rooms)) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    m_directoryRevision = snapshot.revision;
    m_directoryResyncPending = false;
    setDirectoryReady(true);
}

void
ArenaSession::handleDirectoryDelta(const RoomDirectoryUpdated& delta)
{
    if (!m_directoryRevision || delta.revision > *m_directoryRevision + 1) {
        if (!m_directoryResyncPending) {
            m_directoryResyncPending = true;
            sendDirectorySubscribe();
        }
        return;
    }
    if (delta.revision <= *m_directoryRevision) {
        return;
    }
    if (!m_rooms.applyDelta(delta.upserts, delta.removedRoomIds)) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    m_directoryRevision = delta.revision;
}

void
ArenaSession::createRoom(const QString& name, const QString& password)
{
    if (!m_active || m_state != State::Browsing) {
        return;
    }
    clearError();
    if (getAdmissionPending() && !m_pendingAdmissionRequestId.isEmpty()) {
        setError(QStringLiteral("busy"), QStringLiteral("arena.error.busy"));
        return;
    }
    clearPendingAdmission();
    setPendingAdmission(PendingCreate{
      .name = name,
      .password =
        password.isEmpty() ? std::nullopt : std::optional<QString>{ password },
    });
    if (!m_identityProvider->loggedIn()) {
        setLoginRequired(true);
        return;
    }
    if (m_authenticated && m_protocolReady) {
        sendPendingAdmission();
    } else {
        beginAuthenticatedAdmission();
    }
}

void
ArenaSession::joinRoom(const QString& roomId, const QString& password)
{
    if (!m_active || m_state != State::Browsing) {
        return;
    }
    clearError();
    if (getAdmissionPending() && !m_pendingAdmissionRequestId.isEmpty()) {
        setError(QStringLiteral("busy"), QStringLiteral("arena.error.busy"));
        return;
    }
    clearPendingAdmission();
    setPendingAdmission(PendingJoin{
      .roomId = roomId,
      .password =
        password.isEmpty() ? std::nullopt : std::optional<QString>{ password },
    });
    if (!m_identityProvider->loggedIn()) {
        setLoginRequired(true);
        return;
    }
    if (m_authenticated && m_protocolReady) {
        sendPendingAdmission();
    } else {
        beginAuthenticatedAdmission();
    }
}

void
ArenaSession::beginAuthenticatedAdmission()
{
    if (!m_active || !getAdmissionPending() ||
        !m_identityProvider->loggedIn()) {
        return;
    }
    setLoginRequired(false);
    setAuthenticated(false);
    setState(State::ConnectingAuthenticated);
    invalidateTransport();
    requestTicket();
}

void
ArenaSession::requestTicket()
{
    m_currentTicketRequestId = m_nextTicketRequestId++;
    m_identityProvider->requestTicket(m_currentTicketRequestId);
}

void
ArenaSession::handleTicketReady(quint64 requestId, const QString& ticket)
{
    if (!m_active || requestId == 0 || requestId != m_currentTicketRequestId) {
        return;
    }
    m_currentTicketRequestId = 0;
    if (m_state == State::Reconnecting && resumeDeadlineReached()) {
        failResumeAtDeadline();
        return;
    }
    m_pendingTicket = ticket;
    if (m_state == State::ConnectingAuthenticated) {
        startTransport(HandshakeKind::AuthenticatedAdmission);
    } else if (m_state == State::Reconnecting) {
        startTransport(HandshakeKind::Resume);
    } else {
        m_pendingTicket.clear();
    }
}

void
ArenaSession::handleTicketFailure(quint64 requestId,
                                  ArenaIdentityProvider::TicketFailure failure)
{
    if (!m_active || requestId == 0 || requestId != m_currentTicketRequestId) {
        return;
    }
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    if (m_state == State::Reconnecting && resumeDeadlineReached()) {
        failResumeAtDeadline();
        return;
    }
    const auto [code, key] = ticketFailureCode(failure);
    if (m_state == State::Reconnecting &&
        failure == ArenaIdentityProvider::TicketFailure::Network) {
        scheduleReconnect();
        return;
    }
    if (m_state == State::Reconnecting) {
        cancelReconnectTasks();
        clearRoom();
    }
    clearPendingAdmission();
    setError(code, key);
    openAnonymousBrowsing();
}

void
ArenaSession::sendPendingAdmission()
{
    if (!m_protocolReady || !m_authenticated || !getAdmissionPending() ||
        !m_pendingAdmissionRequestId.isEmpty()) {
        return;
    }
    const auto requestId = nextRequestId();
    ClientMessage message =
      std::visit(Overloaded{
                   [](const std::monostate&) -> ClientMessage {
                       return DirectorySubscribe{};
                   },
                   [&](const PendingCreate& pending) -> ClientMessage {
                       return RoomCreate{ .requestId = requestId,
                                          .name = pending.name,
                                          .password = pending.password };
                   },
                   [&](const PendingJoin& pending) -> ClientMessage {
                       return RoomJoin{ .requestId = requestId,
                                        .roomId = pending.roomId,
                                        .password = pending.password };
                   },
                 },
                 m_pendingAdmission);
    if (!sendMessage(message)) {
        return;
    }
    m_pendingAdmissionRequestId = requestId;
    m_pendingCommands.insert(
      requestId,
      PendingCommand{ .kind = PendingCommandKind::Admission,
                      .lifecycleGeneration = m_lifecycleGeneration });
}

void
ArenaSession::handleServerMessage(const ServerMessage& message)
{
    if (const auto* hello = std::get_if<ServerHello>(&message)) {
        handleServerHello(*hello);
        return;
    }
    if (const auto* fatal = std::get_if<FatalError>(&message)) {
        failProtocol(fatalCode(fatal->code), fatal->displayMessageKey);
        return;
    }
    if (!m_protocolReady) {
        failProtocol(QStringLiteral("hello_required"),
                     QStringLiteral("arena.error.helloRequired"));
        return;
    }

    std::visit(
      Overloaded{
        [](const ServerHello&) {},
        [](const FatalError&) {},
        [this](const DirectorySnapshot& snapshot) {
            handleDirectorySnapshot(snapshot);
        },
        [this](const RoomDirectoryUpdated& delta) {
            handleDirectoryDelta(delta);
        },
        [this](const RoomSnapshotEvent& event) {
            const auto found = m_pendingCommands.constFind(event.requestId);
            if (found == m_pendingCommands.cend() ||
                found->kind != PendingCommandKind::Admission ||
                found->lifecycleGeneration != m_lifecycleGeneration ||
                event.requestId != m_pendingAdmissionRequestId) {
                return;
            }
            applyRoomSnapshot(event.room);
        },
        [this](const RoomMemberJoined& event) {
            if (acceptsRoomEvent(event.roomId, event.roomGeneration)) {
                m_members.upsert(event.member);
            }
        },
        [this](const RoomMemberUpdated& event) {
            if (acceptsRoomEvent(event.roomId, event.roomGeneration)) {
                m_members.upsert(event.member);
            }
        },
        [this](const RoomMemberLeft& event) {
            if (!acceptsRoomEvent(event.roomId, event.roomGeneration)) {
                return;
            }
            if (event.memberId == m_selfMemberId) {
                const auto kicked = event.reason == MemberLeftReason::Kicked;
                returnToAuthenticatedBrowser();
                if (kicked) {
                    setError(QStringLiteral("kicked"),
                             QStringLiteral("arena.error.kicked"));
                }
                return;
            }
            (void)m_members.remove(event.memberId);
            for (auto it = m_pendingCommands.begin();
                 it != m_pendingCommands.end();) {
                if (it->kind == PendingCommandKind::Kick &&
                    it->relatedMemberId == event.memberId) {
                    it = m_pendingCommands.erase(it);
                } else {
                    ++it;
                }
            }
        },
        [this](const RoomOwnerChanged& event) {
            if (!acceptsRoomEvent(event.roomId, event.roomGeneration)) {
                return;
            }
            const auto oldIsOwner = getIsOwner();
            const auto oldOwner = m_ownerMemberId;
            m_ownerMemberId = event.ownerMemberId;
            m_members.setOwnerMemberId(m_ownerMemberId);
            if (oldOwner != m_ownerMemberId || oldIsOwner != getIsOwner()) {
                emit ownerChanged();
            }
        },
        [this](const ChatMessageEvent& event) {
            if (!acceptsRoomEvent(event.roomId, event.roomGeneration)) {
                return;
            }
            m_chat.upsert(event.message);
            if (event.message.authorMemberId == m_selfMemberId &&
                !m_pendingChatCommandIds.isEmpty()) {
                const auto requestId = m_pendingChatCommandIds.takeFirst();
                m_pendingCommands.remove(requestId);
            }
        },
        [this](const ServerHeartbeat& heartbeat) {
            (void)sendMessage(HeartbeatReply{ .nonce = heartbeat.nonce });
        },
        [this](const ServerGoingAway&) {
            cancelReconnectTasks();
            invalidateTransport();
            clearPendingAdmission();
            clearRoom();
            setAuthenticated(false);
            setError(QStringLiteral("server_going_away"),
                     QStringLiteral("arena.serverGoingAway"));
            setState(State::Error);
        },
        [this](const CommandError& error) { handleCommandError(error); },
        // Phase 2 messages are decoded here before the round-state handlers
        // are connected in the ArenaSession integration task.
        [](const auto&) {},
      },
      message);
}

void
ArenaSession::handleCommandError(const CommandError& error)
{
    const auto found = m_pendingCommands.find(error.requestId);
    if (found == m_pendingCommands.end() ||
        found->lifecycleGeneration != m_lifecycleGeneration) {
        return;
    }
    const auto kind = found->kind;
    m_pendingCommands.erase(found);
    m_pendingChatCommandIds.removeAll(error.requestId);
    setError(commandCode(error.code), error.displayMessageKey);
    if (kind == PendingCommandKind::Admission) {
        clearPendingAdmission();
        setState(State::Browsing);
    }
    if ((error.code == CommandErrorCode::RoomGenerationStale ||
         error.code == CommandErrorCode::ConnectionGenerationStale) &&
        !m_roomId.isEmpty()) {
        beginReconnect();
    }
}

void
ArenaSession::applyRoomSnapshot(const RoomSnapshot& snapshot)
{
    if (!m_members.replace(
          snapshot.members, snapshot.ownerMemberId, snapshot.self.memberId) ||
        !m_chat.replace(snapshot.chat, snapshot.self.memberId)) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    cancelReconnectTasks();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
    clearPendingAdmission();

    const auto oldOwner = m_ownerMemberId;
    const auto oldIsOwner = getIsOwner();
    m_roomId = snapshot.roomId;
    m_roomName = snapshot.name;
    m_roomGeneration = snapshot.roomGeneration;
    m_connectionGeneration = snapshot.self.connectionGeneration;
    m_selfMemberId = snapshot.self.memberId;
    m_ownerMemberId = snapshot.ownerMemberId;
    m_resumeToken = snapshot.self.resumeToken;
    emit roomChanged();
    if (oldOwner != m_ownerMemberId || oldIsOwner != getIsOwner()) {
        emit ownerChanged();
    }
    setAuthenticated(true);
    clearError();
    setState(State::InRoom);
}

void
ArenaSession::clearRoom()
{
    const auto hadRoom = !m_roomId.isEmpty() || !m_roomName.isEmpty() ||
                         m_roomGeneration != 0 || !m_selfMemberId.isEmpty();
    const auto hadOwner = m_ownerMemberId.has_value();
    m_roomId.clear();
    m_roomName.clear();
    m_roomGeneration = 0;
    m_connectionGeneration = 0;
    m_selfMemberId.clear();
    m_ownerMemberId.reset();
    m_resumeToken.clear();
    m_resumeToken.squeeze();
    m_members.clear();
    m_chat.clear();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
    if (hadRoom) {
        emit roomChanged();
    }
    if (hadOwner) {
        emit ownerChanged();
    }
}

auto
ArenaSession::acceptsRoomEvent(QStringView roomId, qint64 generation) -> bool
{
    if (m_roomId.isEmpty() || roomId != m_roomId) {
        return false;
    }
    if (generation == m_roomGeneration) {
        return true;
    }
    if (generation > m_roomGeneration) {
        beginReconnect();
    }
    return false;
}

void
ArenaSession::returnToAuthenticatedBrowser()
{
    clearRoom();
    clearPendingAdmission();
    clearError();
    setState(State::Browsing);
}

void
ArenaSession::leaveRoom()
{
    clearError();
    if (m_state == State::Reconnecting) {
        ++m_lifecycleGeneration;
        cancelReconnectTasks();
        m_currentTicketRequestId = 0;
        invalidateTransport();
        clearRoom();
        openAnonymousBrowsing();
        return;
    }
    if (m_state != State::InRoom || !m_protocolReady) {
        return;
    }
    const auto requestId = nextRequestId();
    if (sendMessage(
          RoomLeave{ .requestId = requestId,
                     .roomId = m_roomId,
                     .roomGeneration = m_roomGeneration,
                     .connectionGeneration = m_connectionGeneration })) {
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::Leave,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::kickMember(const QString& memberId)
{
    clearError();
    if (m_state != State::InRoom || !m_protocolReady) {
        return;
    }
    const auto requestId = nextRequestId();
    if (sendMessage(RoomKick{ .requestId = requestId,
                              .roomId = m_roomId,
                              .roomGeneration = m_roomGeneration,
                              .connectionGeneration = m_connectionGeneration,
                              .targetMemberId = memberId })) {
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::Kick,
                          .lifecycleGeneration = m_lifecycleGeneration,
                          .relatedMemberId = memberId });
    }
}

void
ArenaSession::sendChat(const QString& text)
{
    clearError();
    if (m_state != State::InRoom || !m_protocolReady) {
        return;
    }
    const auto requestId = nextRequestId();
    if (sendMessage(ChatSend{ .requestId = requestId,
                              .roomId = m_roomId,
                              .roomGeneration = m_roomGeneration,
                              .connectionGeneration = m_connectionGeneration,
                              .text = text })) {
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::Chat,
                          .lifecycleGeneration = m_lifecycleGeneration });
        m_pendingChatCommandIds.push_back(requestId);
    }
}

void
ArenaSession::handleDisconnected(ArenaTransport::Generation generation)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    m_currentTransportGeneration = ArenaTransport::InvalidGeneration;
    m_protocolReady = false;
    m_handshakeKind = HandshakeKind::None;
    if (m_state == State::InRoom) {
        beginReconnect();
        return;
    }
    if (m_state == State::Reconnecting) {
        scheduleReconnect();
        return;
    }
    if (m_state == State::ConnectingAuthenticated || getAdmissionPending()) {
        restoreAnonymousAfterAdmissionFailure(
          QStringLiteral("transport_remote_closed"),
          QStringLiteral("arena.error.remoteClosed"));
        return;
    }
    setAuthenticated(false);
    clearPendingAdmission();
    setError(QStringLiteral("transport_remote_closed"),
             QStringLiteral("arena.error.remoteClosed"));
    setState(State::Error);
}

void
ArenaSession::handleTransportError(ArenaTransport::Generation generation,
                                   ArenaTransport::Error error)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    if (m_state == State::InRoom) {
        beginReconnect();
        return;
    }
    if (m_state == State::Reconnecting) {
        scheduleReconnect();
        return;
    }
    const auto [code, key] = transportFailureCode(error);
    if (m_state == State::ConnectingAuthenticated || getAdmissionPending()) {
        restoreAnonymousAfterAdmissionFailure(code, key);
        return;
    }
    invalidateTransport();
    setAuthenticated(false);
    clearPendingAdmission();
    setError(code, key);
    setState(State::Error);
}

void
ArenaSession::beginReconnect()
{
    if (!m_active || m_roomId.isEmpty() || m_resumeToken.isEmpty()) {
        return;
    }
    ++m_lifecycleGeneration;
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
    clearPendingAdmission();
    invalidateTransport();
    setAuthenticated(false);
    setState(State::Reconnecting);
    m_nextBackoffMs = InitialBackoffMs;
    const auto now = m_scheduler->monotonicNowMs();
    m_reconnectDeadlineMs =
      now <= (std::numeric_limits<qint64>::max)() - ReconnectGraceMs
        ? now + ReconnectGraceMs
        : (std::numeric_limits<qint64>::max)();
    const auto lifecycle = m_lifecycleGeneration;
    m_graceTask = m_scheduler->scheduleOnce(
      m_reconnectDeadlineMs - now, this, [this, lifecycle] {
          if (m_active && lifecycle == m_lifecycleGeneration &&
              m_state == State::Reconnecting) {
              failResumeAtDeadline();
          }
      });
    startResumeAttempt();
}

void
ArenaSession::startResumeAttempt()
{
    if (!m_active || m_state != State::Reconnecting) {
        return;
    }
    const auto now = m_scheduler->monotonicNowMs();
    if (now >= m_reconnectDeadlineMs) {
        failResumeAtDeadline();
        return;
    }
    cancelTask(m_retryTask);
    cancelTask(m_attemptTimeoutTask);
    invalidateTransport();
    m_currentTicketRequestId = m_nextTicketRequestId++;
    const auto lifecycle = m_lifecycleGeneration;
    const auto timeout =
      (std::min)(AttemptTimeoutMs, m_reconnectDeadlineMs - now);
    m_attemptTimeoutTask =
      m_scheduler->scheduleOnce(timeout, this, [this, lifecycle] {
          if (m_active && lifecycle == m_lifecycleGeneration &&
              m_state == State::Reconnecting) {
              scheduleReconnect();
          }
      });
    m_identityProvider->requestTicket(m_currentTicketRequestId);
}

void
ArenaSession::scheduleReconnect()
{
    if (!m_active || m_state != State::Reconnecting) {
        return;
    }
    cancelTask(m_attemptTimeoutTask);
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    invalidateTransport();
    if (m_retryTask != ArenaScheduler::InvalidTaskId) {
        return;
    }
    const auto now = m_scheduler->monotonicNowMs();
    if (now >= m_reconnectDeadlineMs) {
        failResumeAtDeadline();
        return;
    }
    if (m_nextBackoffMs >= m_reconnectDeadlineMs - now) {
        return;
    }
    const auto delay = m_nextBackoffMs;
    m_nextBackoffMs = (std::min)(MaximumBackoffMs, m_nextBackoffMs * 2);
    const auto lifecycle = m_lifecycleGeneration;
    m_retryTask = m_scheduler->scheduleOnce(delay, this, [this, lifecycle] {
        m_retryTask = ArenaScheduler::InvalidTaskId;
        if (m_active && lifecycle == m_lifecycleGeneration &&
            m_state == State::Reconnecting) {
            startResumeAttempt();
        }
    });
}

auto
ArenaSession::resumeDeadlineReached() const -> bool
{
    return m_state == State::Reconnecting && m_reconnectDeadlineMs > 0 &&
           m_scheduler->monotonicNowMs() >= m_reconnectDeadlineMs;
}

void
ArenaSession::failResumeAtDeadline()
{
    if (!m_active) {
        return;
    }
    ++m_lifecycleGeneration;
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    invalidateTransport();
    clearPendingAdmission();
    clearRoom();
    setAuthenticated(false);
    setError(QStringLiteral("resume_failed"),
             QStringLiteral("arena.error.resumeFailed"));
    openAnonymousBrowsing();
}

void
ArenaSession::cancelTask(ArenaScheduler::TaskId& taskId)
{
    if (taskId == ArenaScheduler::InvalidTaskId) {
        return;
    }
    const auto old = std::exchange(taskId, ArenaScheduler::InvalidTaskId);
    m_scheduler->cancel(old);
}

void
ArenaSession::cancelReconnectTasks()
{
    cancelTask(m_graceTask);
    cancelTask(m_attemptTimeoutTask);
    cancelTask(m_retryTask);
    m_reconnectDeadlineMs = 0;
}

void
ArenaSession::retry()
{
    if (!m_active) {
        return;
    }
    if (m_state == State::Reconnecting) {
        clearError();
        cancelTask(m_retryTask);
        startResumeAttempt();
        return;
    }
    if (m_state == State::Error) {
        clearError();
        invalidateTransport();
        openAnonymousBrowsing();
    }
}

void
ArenaSession::bestEffortLeave()
{
    if (m_state != State::InRoom || !m_protocolReady ||
        m_currentTransportGeneration == ArenaTransport::InvalidGeneration ||
        m_roomId.isEmpty()) {
        return;
    }
    (void)sendMessage(RoomLeave{
      .requestId = nextRequestId(),
      .roomId = m_roomId,
      .roomGeneration = m_roomGeneration,
      .connectionGeneration = m_connectionGeneration,
    });
}

void
ArenaSession::invalidateAsyncWork()
{
    ++m_lifecycleGeneration;
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    m_pendingTicket.squeeze();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
}

void
ArenaSession::cleanupForIdentityChange()
{
    if (!m_active) {
        return;
    }
    const auto keepAnonymous = m_state == State::Browsing && !m_authenticated &&
                               m_roomId.isEmpty() && m_protocolReady;
    bestEffortLeave();
    invalidateAsyncWork();
    clearPendingAdmission();
    clearRoom();
    setAuthenticated(false);
    setLoginRequired(false);
    if (keepAnonymous) {
        clearError();
        return;
    }
    invalidateTransport();
    openAnonymousBrowsing();
}

void
ArenaSession::handleActiveProfileChanged()
{
    m_lastLoggedIn = m_identityProvider->loggedIn();
    cleanupForIdentityChange();
}

void
ArenaSession::handleLoginStateChanged()
{
    const auto loggedIn = m_identityProvider->loggedIn();
    const auto wasLoggedIn = std::exchange(m_lastLoggedIn, loggedIn);
    if (!m_active) {
        return;
    }
    if (!wasLoggedIn && loggedIn && getAdmissionPending()) {
        beginAuthenticatedAdmission();
        return;
    }
    if (wasLoggedIn && !loggedIn) {
        cleanupForIdentityChange();
    }
}

void
ArenaSession::exitArena()
{
    if (!m_active) {
        return;
    }
    bestEffortLeave();
    invalidateAsyncWork();
    clearPendingAdmission();
    invalidateTransport();
    clearRoom();
    m_rooms.clear();
    m_directoryRevision.reset();
    m_directoryResyncPending = false;
    setDirectoryReady(false);
    setAuthenticated(false);
    setLoginRequired(false);
    clearError();
    setActive(false);
    setState(State::Disconnected);
}

void
ArenaSession::failProtocol(ProtocolFailureCode code)
{
    failProtocol(protocolCode(code), displayMessageKey(code));
}

void
ArenaSession::failProtocol(QString code, QString messageKey)
{
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    clearPendingAdmission();
    invalidateTransport();
    clearRoom();
    setAuthenticated(false);
    setError(std::move(code), std::move(messageKey));
    setState(State::Error);
}

} // namespace arena
