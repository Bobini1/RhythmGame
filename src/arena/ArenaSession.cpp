#include "ArenaSession.h"

#include "ArenaBinaryProtocol.h"
#include "ArenaProtocol.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartRunner.h"

#include <QByteArray>
#include <QCryptographicHash>

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
        case CommandErrorCode::RoundsCapabilityRequired:
            return QStringLiteral("rounds_capability_required");
        case CommandErrorCode::CompetitionCapabilityRequired:
            return QStringLiteral("competition_capability_required");
        case CommandErrorCode::InventoryBusy:
            return QStringLiteral("inventory_busy");
        case CommandErrorCode::InventoryInvalid:
            return QStringLiteral("inventory_invalid");
        case CommandErrorCode::InventoryStale:
            return QStringLiteral("inventory_stale");
        case CommandErrorCode::InventoryCapacityExceeded:
            return QStringLiteral("inventory_capacity_exceeded");
        case CommandErrorCode::AvailabilityStale:
            return QStringLiteral("availability_stale");
        case CommandErrorCode::SelectionNotCommon:
            return QStringLiteral("selection_not_common");
        case CommandErrorCode::SelectionStale:
            return QStringLiteral("selection_stale");
        case CommandErrorCode::ReadyNotAllowed:
            return QStringLiteral("ready_not_allowed");
        case CommandErrorCode::RoundStale:
            return QStringLiteral("round_stale");
        case CommandErrorCode::LaunchStageStale:
            return QStringLiteral("launch_stage_stale");
        case CommandErrorCode::ResultInvalid:
            return QStringLiteral("result_invalid");
        case CommandErrorCode::RoundAlreadyTerminal:
            return QStringLiteral("round_already_terminal");
        case CommandErrorCode::ServerCapacity:
            return QStringLiteral("server_capacity");
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
        case FatalErrorCode::MalformedInventory:
            return QStringLiteral("malformed_inventory");
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

auto
packedDigest(const QByteArray& packed) -> QString
{
    return QString::fromLatin1(
      QCryptographicHash::hash(packed, QCryptographicHash::Sha256).toHex());
}

auto
decodeTransferId(QStringView encoded) -> QByteArray
{
    const auto result = QByteArray::fromBase64(
      encoded.toLatin1(),
      QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
    return result.size() == ArenaTransferIdBytes ? result : QByteArray{};
}

auto
playNoteOrder(NoteOrder order) -> resource_managers::NoteOrderAlgorithm
{
    using Target = resource_managers::NoteOrderAlgorithm;
    switch (order) {
        case NoteOrder::Normal:
            return Target::Normal;
        case NoteOrder::Mirror:
            return Target::Mirror;
        case NoteOrder::Random:
            return Target::Random;
        case NoteOrder::SRandom:
            return Target::SRandom;
        case NoteOrder::RRandom:
            return Target::RRandom;
        case NoteOrder::RandomPlus:
            return Target::RandomPlus;
        case NoteOrder::SRandomPlus:
            return Target::SRandomPlus;
        case NoteOrder::BeatorajaRandom:
            return Target::BeatorajaRandom;
        case NoteOrder::BeatorajaRandomEx:
            return Target::BeatorajaRandomEx;
        case NoteOrder::Lr2Random:
            return Target::Lr2Random;
        case NoteOrder::Lr2RandomEx:
            return Target::Lr2RandomEx;
    }
    return Target::Normal;
}

auto
playDpMode(DpMode mode) -> resource_managers::DpOptions
{
    using Target = resource_managers::DpOptions;
    switch (mode) {
        case DpMode::Off:
            return Target::Off;
        case DpMode::Flip:
            return Target::Flip;
        case DpMode::Lr2Flip:
            return Target::Lr2Flip;
        case DpMode::Battle:
            return Target::Battle;
    }
    return Target::Off;
}

auto
probeFailureReason(ArenaProbeFailure failure)
  -> std::optional<RoundProbeFailureReason>
{
    switch (failure) {
        case ArenaProbeFailure::None:
            return std::nullopt;
        case ArenaProbeFailure::MissingFile:
            return RoundProbeFailureReason::MissingFile;
        case ArenaProbeFailure::HashMismatch:
            return RoundProbeFailureReason::HashMismatch;
        case ArenaProbeFailure::ReadFailed:
            return RoundProbeFailureReason::ReadFailed;
        case ArenaProbeFailure::Cancelled:
            return RoundProbeFailureReason::Cancelled;
    }
    return RoundProbeFailureReason::Cancelled;
}

auto
loadFailureReason(ArenaLoadFailure failure) -> RoundLoadFailureReason
{
    switch (failure) {
        case ArenaLoadFailure::MissingFile:
            return RoundLoadFailureReason::MissingFile;
        case ArenaLoadFailure::HashMismatch:
            return RoundLoadFailureReason::HashMismatch;
        case ArenaLoadFailure::ParseFailed:
            return RoundLoadFailureReason::ParseFailed;
        case ArenaLoadFailure::UnsupportedConfig:
            return RoundLoadFailureReason::UnsupportedConfig;
        case ArenaLoadFailure::ResourceFailed:
            return RoundLoadFailureReason::ResourceFailed;
        case ArenaLoadFailure::Cancelled:
            return RoundLoadFailureReason::Cancelled;
    }
    return RoundLoadFailureReason::Cancelled;
}

auto
roundLaunchCancellationStatusKey(RoundLaunchCancellationReason reason)
  -> QString
{
    switch (reason) {
        case RoundLaunchCancellationReason::MissingFile:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.missingFile");
        case RoundLaunchCancellationReason::HashMismatch:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.hashMismatch");
        case RoundLaunchCancellationReason::ReadFailed:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.readFailed");
        case RoundLaunchCancellationReason::ParseFailed:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.parseFailed");
        case RoundLaunchCancellationReason::UnsupportedConfig:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.unsupportedConfig");
        case RoundLaunchCancellationReason::ResourceFailed:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.resourceFailed");
        case RoundLaunchCancellationReason::ProbeTimeout:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.probeTimeout");
        case RoundLaunchCancellationReason::LoadTimeout:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.loadTimeout");
        case RoundLaunchCancellationReason::ParticipantLeft:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.participantLeft");
        case RoundLaunchCancellationReason::ParticipantKicked:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.participantKicked");
        case RoundLaunchCancellationReason::ChartLengthMismatch:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.chartLengthMismatch");
        case RoundLaunchCancellationReason::ServerShutdown:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.serverShutdown");
        case RoundLaunchCancellationReason::Cancelled:
            return QStringLiteral(
              "arena.status.roundLaunchCancelled.cancelled");
    }
    return {};
}

auto
sameFrozenRound(const FrozenRound& left, const FrozenRound& right) -> bool
{
    return left.roundId == right.roundId &&
           left.launchAttemptId == right.launchAttemptId &&
           left.selectionRevision == right.selectionRevision &&
           left.availabilityRevision == right.availabilityRevision &&
           left.selection == right.selection &&
           left.participants == right.participants;
}

auto
hasCompetitionRoundShape(const FrozenRound& round) -> bool
{
    return std::ranges::all_of(round.participants,
                               [](const FrozenParticipant& participant) {
                                   return participant.identity.has_value();
                               });
}

auto
frozenInventoryRevision(const FrozenRound& round, QStringView memberId)
  -> std::optional<qint64>
{
    const auto participant = std::ranges::find_if(
      round.participants, [memberId](const FrozenParticipant& candidate) {
          return candidate.memberId == memberId;
      });
    return participant == round.participants.cend()
             ? std::nullopt
             : std::optional<qint64>{ participant->inventoryRevision };
}

auto
noteOrderName(NoteOrder value) -> QString
{
    switch (value) {
        case NoteOrder::Normal:
            return QStringLiteral("Normal");
        case NoteOrder::Mirror:
            return QStringLiteral("Mirror");
        case NoteOrder::Random:
            return QStringLiteral("Random");
        case NoteOrder::SRandom:
            return QStringLiteral("S-Random");
        case NoteOrder::RRandom:
            return QStringLiteral("R-Random");
        case NoteOrder::RandomPlus:
            return QStringLiteral("Random+");
        case NoteOrder::SRandomPlus:
            return QStringLiteral("S-Random+");
        case NoteOrder::BeatorajaRandom:
            return QStringLiteral("Beatoraja Random");
        case NoteOrder::BeatorajaRandomEx:
            return QStringLiteral("Beatoraja Random EX");
        case NoteOrder::Lr2Random:
            return QStringLiteral("LR2 Random");
        case NoteOrder::Lr2RandomEx:
            return QStringLiteral("LR2 Random EX");
    }
    return {};
}

auto
dpModeName(DpMode value) -> QString
{
    switch (value) {
        case DpMode::Off:
            return QStringLiteral("Off");
        case DpMode::Flip:
            return QStringLiteral("Flip");
        case DpMode::Lr2Flip:
            return QStringLiteral("LR2 Flip");
        case DpMode::Battle:
            return QStringLiteral("Battle");
    }
    return {};
}

auto
optionsSummary(const SelectionSnapshot& selection) -> QString
{
    const auto nativeDouble =
      selection.keyMode == 10 || selection.keyMode == 14;
    const auto battle = selection.dpMode == DpMode::Battle && !nativeDouble &&
                        (selection.keyMode == 5 || selection.keyMode == 7);
    const auto showP2 = nativeDouble || battle;
    const auto showDp =
      (nativeDouble && (selection.dpMode == DpMode::Flip ||
                        selection.dpMode == DpMode::Lr2Flip)) ||
      battle;
    if (!showP2 && !showDp) {
        return noteOrderName(selection.noteOrderP1);
    }

    auto parts = QStringList{ QStringLiteral("P1 %1").arg(
      noteOrderName(selection.noteOrderP1)) };
    if (showP2) {
        parts.append(
          QStringLiteral("P2 %1").arg(noteOrderName(selection.noteOrderP2)));
    }
    if (showDp) {
        parts.append(QStringLiteral("DP %1").arg(dpModeName(selection.dpMode)));
    }
    return parts.join(QStringLiteral(" | "));
}

auto
terminalRequestId(const std::variant<RoundResultSubmit, RoundAbandon>& message)
  -> QString
{
    return std::visit([](const auto& value) { return value.requestId; },
                      message);
}

auto
terminalKind(const std::variant<RoundResultSubmit, RoundAbandon>& message)
  -> TerminalKind
{
    return std::holds_alternative<RoundResultSubmit>(message)
             ? TerminalKind::Finished
             : TerminalKind::Dnf;
}

auto
standingIsTerminal(const LiveStandingEntry& entry) -> bool
{
    return std::holds_alternative<LiveFinishedStanding>(entry.state) ||
           std::holds_alternative<LiveDnfStanding>(entry.state);
}

} // namespace

ArenaSession::ArenaSession(ArenaTransport* transport,
                           ArenaIdentityProvider* identityProvider,
                           ArenaScheduler* scheduler,
                           QUrl endpoint,
                           QString clientVersion,
                           ArenaInventorySource* inventorySource,
                           ArenaRoundLoader* roundLoader,
                           ArenaGameplaySource* gameplaySource,
                           QObject* parent)
  : QObject(parent)
  , m_transport(transport)
  , m_identityProvider(identityProvider)
  , m_scheduler(scheduler)
  , m_inventorySource(inventorySource)
  , m_roundLoader(roundLoader)
  , m_gameplaySource(gameplaySource)
  , m_endpoint(std::move(endpoint))
  , m_clientVersion(std::move(clientVersion))
  , m_rooms(this)
  , m_members(this)
  , m_chat(this)
  , m_availability(this)
  , m_liveStandings(this)
  , m_lastResult(this)
  , m_presentedResult(this)
  , m_opponentTarget(this)
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
    connect(&m_availability, &ArenaAvailabilityIndex::changed, this, [this] {
        emit availabilityChanged();
        emit selectionChanged();
        emit readyChanged();
    });
    connect(this,
            &ArenaSession::selectionChanged,
            this,
            &ArenaSession::selectedByDisplayNameChanged);
    const auto notifySelectedByDisplayName = [this] {
        if (!m_selectedByMemberId.isEmpty()) {
            emit selectedByDisplayNameChanged();
        }
    };
    connect(&m_members,
            &QAbstractItemModel::modelReset,
            this,
            notifySelectedByDisplayName);
    connect(&m_members,
            &QAbstractItemModel::rowsInserted,
            this,
            notifySelectedByDisplayName);
    connect(&m_members,
            &QAbstractItemModel::rowsRemoved,
            this,
            notifySelectedByDisplayName);
    connect(
      &m_members,
      &QAbstractItemModel::dataChanged,
      this,
      [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles) {
          if (!m_selectedByMemberId.isEmpty() &&
              (roles.isEmpty() ||
               roles.contains(ArenaMemberListModel::DisplayNameRole))) {
              emit selectedByDisplayNameChanged();
          }
      });
    if (m_inventorySource != nullptr) {
        connect(m_inventorySource,
                &ArenaInventorySource::generationChanged,
                this,
                &ArenaSession::handleInventoryGenerationChanged);
        connect(m_inventorySource,
                &ArenaInventorySource::snapshotReady,
                this,
                &ArenaSession::handleInventorySnapshotReady);
        connect(m_inventorySource,
                &ArenaInventorySource::snapshotFailed,
                this,
                &ArenaSession::handleInventorySnapshotFailed);
    }
    if (m_roundLoader != nullptr) {
        connect(m_roundLoader,
                &ArenaRoundLoader::probeFinished,
                this,
                &ArenaSession::handleProbeFinished);
        connect(m_roundLoader,
                &ArenaRoundLoader::loadFinished,
                this,
                &ArenaSession::handleLoadFinished);
        connect(m_roundLoader,
                &ArenaRoundLoader::loadFailed,
                this,
                &ArenaSession::handleLoadFailed);
    }
}

ArenaSession::~ArenaSession()
{
    ++m_lifecycleGeneration;
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_round.reset();
    clearRoundTransfers();
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
ArenaSession::getRoundLaunchCancellationStatusKey() const -> QString
{
    return m_roundLaunchCancellationStatusKey;
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
ArenaSession::getRoundsAvailable() const -> bool
{
    return m_roundsAvailable;
}
auto
ArenaSession::competitionAvailable() const -> bool
{
    return m_competitionAvailable;
}
auto
ArenaSession::getAvailabilitySyncing() const -> bool
{
    return m_availability.state() == ArenaAvailabilityIndex::State::Syncing;
}
auto
ArenaSession::getAvailability() -> ArenaAvailabilityIndex*
{
    return &m_availability;
}
auto
ArenaSession::getRoomPhase() const -> RoomPhase
{
    return m_roomPhase;
}
auto
ArenaSession::getCanSelect() const -> bool
{
    return m_state == State::InRoom && m_roundsAvailable &&
           m_roomPhase == RoomPhase::Selecting && !m_round &&
           !m_pendingInventoryUpload &&
           m_selfInventoryState == InventoryState::Ready &&
           m_selfInventoryRevision > 0 && m_roomAvailabilityRevision > 0 &&
           m_availability.state() == ArenaAvailabilityIndex::State::Ready &&
           m_availability.revision() == m_roomAvailabilityRevision &&
           m_selfAvailabilityAppliedRevision == m_roomAvailabilityRevision;
}
auto
ArenaSession::getCanReady() const -> bool
{
    return getCanSelect() && m_selection.has_value() && !m_selfReady &&
           m_selfInventoryState == InventoryState::Ready &&
           m_selfInventoryRevision > 0 && m_roomAvailabilityRevision > 0 &&
           m_availability.revision() == m_roomAvailabilityRevision &&
           m_selfAvailabilityAppliedRevision == m_roomAvailabilityRevision;
}
auto
ArenaSession::getReady() const -> bool
{
    return m_selfReady;
}
auto
ArenaSession::getSelectedTitle() const -> QString
{
    return m_selection ? m_selection->title : QString{};
}
auto
ArenaSession::getSelectedSubtitle() const -> QString
{
    return m_selection ? m_selection->subtitle : QString{};
}
auto
ArenaSession::getSelectedMd5() const -> QString
{
    return m_selection && m_selection->md5 ? *m_selection->md5 : QString{};
}
auto
ArenaSession::getSelectedByMemberId() const -> QString
{
    return m_selectedByMemberId;
}
auto
ArenaSession::getSelectedByDisplayName() const -> QString
{
    return m_members.displayNameForMemberId(m_selectedByMemberId);
}
auto
ArenaSession::getSelectionRevision() const -> qint64
{
    return m_selectionRevision;
}
auto
ArenaSession::getCurrentRoundId() const -> QString
{
    return m_round ? m_round->roundId : QString{};
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
auto
ArenaSession::liveStandings() -> ArenaStandingsModel*
{
    return &m_liveStandings;
}
auto
ArenaSession::lastResult() -> ArenaResultModel*
{
    return &m_lastResult;
}
auto
ArenaSession::presentedResult() -> ArenaResultModel*
{
    return &m_presentedResult;
}
auto
ArenaSession::opponentTarget() -> ArenaOpponentTarget*
{
    return &m_opponentTarget;
}
auto
ArenaSession::arenaRunner() const -> gameplay_logic::ChartRunner*
{
    return m_arenaRunner.data();
}
auto
ArenaSession::arenaGameplayActive() const -> bool
{
    return m_arenaGameplayActive;
}
auto
ArenaSession::resultPresentationActive() const -> bool
{
    return m_resultPresentationActive;
}
auto
ArenaSession::gameplayChatOpen() const -> bool
{
    return m_gameplayChatOpen;
}

auto
ArenaSession::overlayCustomizationActive() const -> bool
{
    return m_overlayCustomizationActive;
}
auto
ArenaSession::arenaOptionsSummary() const -> QString
{
    return m_arenaOptionsSummary;
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
ArenaSession::setRoundsAvailable(bool available)
{
    if (m_roundsAvailable == available) {
        return;
    }
    m_roundsAvailable = available;
    emit capabilitiesChanged();
    emit selectionChanged();
    emit readyChanged();
}

void
ArenaSession::setCompetitionAvailable(bool available)
{
    if (m_competitionAvailable == available) {
        return;
    }
    m_competitionAvailable = available;
    emit capabilitiesChanged();
    emit selectionChanged();
    emit readyChanged();
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
ArenaSession::setRoundLaunchCancellationStatusKey(QString statusKey)
{
    if (m_roundLaunchCancellationStatusKey == statusKey) {
        return;
    }
    m_roundLaunchCancellationStatusKey = std::move(statusKey);
    emit roundLaunchCancellationStatusKeyChanged();
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
    m_legacyFallbackAvailable = true;
    m_legacyBrowseOnly = false;
    clearError();
    openAnonymousBrowsing();
}

void
ArenaSession::startTransport(HandshakeKind kind)
{
    invalidateTransport();
    setRoundsAvailable(false);
    setCompetitionAvailable(false);
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
    setRoundsAvailable(false);
    setCompetitionAvailable(false);
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
    if (m_legacyBrowseOnly &&
        m_handshakeKind == HandshakeKind::AnonymousBrowse) {
        hello.capabilities = { QString::fromLatin1(RoomsCapability) };
    }
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
                           const QByteArray& bytes)
{
    if (!m_active || generation != m_currentTransportGeneration) {
        return;
    }
    if (!m_pendingAvailabilityTransfer) {
        if (!m_availabilityResyncRequestId.isEmpty()) {
            return;
        }
        failProtocol(QStringLiteral("unexpected_binary"),
                     QStringLiteral("arena.error.unexpectedBinary"));
        return;
    }
    const auto decoded = decodeArenaBinaryChunk(bytes);
    const auto* chunk = std::get_if<ArenaBinaryChunk>(&decoded);
    if (chunk == nullptr) {
        requestAvailabilityResync();
        return;
    }
    auto& transfer = *m_pendingAvailabilityTransfer;
    if (chunk->transferId != transfer.transferId) {
        return;
    }
    auto append = [&](QByteArray& destination,
                      quint32& receivedChunks,
                      qint64 declaredCount) {
        if (chunk->chunkIndex != receivedChunks ||
            destination.size() + chunk->packedHashes.size() >
              declaredCount * ArenaSha256Bytes) {
            return false;
        }
        destination.append(chunk->packedHashes);
        ++receivedChunks;
        return true;
    };
    const auto& declaration = transfer.declaration;
    const auto accepted = [&] {
        switch (chunk->kind) {
            case ArenaBinaryKind::InventoryUpload:
                return false;
            case ArenaBinaryKind::AvailabilityReset:
                return declaration.mode == AvailabilityTransferMode::Reset &&
                       append(transfer.reset,
                              transfer.resetChunks,
                              declaration.resetCount);
            case ArenaBinaryKind::AvailabilityAdd:
                return declaration.mode == AvailabilityTransferMode::Delta &&
                       append(transfer.added,
                              transfer.addedChunks,
                              declaration.addedCount);
            case ArenaBinaryKind::AvailabilityRemove:
                return declaration.mode == AvailabilityTransferMode::Delta &&
                       append(transfer.removed,
                              transfer.removedChunks,
                              declaration.removedCount);
        }
        return false;
    }();
    if (!accepted) {
        requestAvailabilityResync();
    }
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
    setRoundsAvailable(
      hello.capabilities.contains(QString::fromLatin1(RoundsCapability)));
    setCompetitionAvailable(
      hello.capabilities.contains(QString::fromLatin1(CompetitionCapability)));
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
        m_legacyFallbackAvailable = false;
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
        if (!m_competitionAvailable) {
            clearPendingAdmission();
            setError(QStringLiteral("competition_capability_required"),
                     QStringLiteral("arena.error.updateRequired"));
            return;
        }
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
    if (!m_competitionAvailable) {
        cancelReconnectTasks();
        clearRoom();
        setError(QStringLiteral("competition_capability_required"),
                 QStringLiteral("arena.error.updateRequired"));
        setState(State::Browsing);
        sendDirectorySubscribe();
        return;
    }
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
    if (!m_competitionAvailable) {
        setError(QStringLiteral("competition_capability_required"),
                 QStringLiteral("arena.error.updateRequired"));
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
    if (!m_competitionAvailable) {
        setError(QStringLiteral("competition_capability_required"),
                 QStringLiteral("arena.error.updateRequired"));
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
        !m_pendingAdmissionRequestId.isEmpty() || !m_competitionAvailable) {
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
        if (!m_protocolReady &&
            m_handshakeKind == HandshakeKind::AnonymousBrowse &&
            m_legacyFallbackAvailable && !m_legacyBrowseOnly &&
            fatal->code == FatalErrorCode::ProtocolIncompatible) {
            m_legacyFallbackAvailable = false;
            m_legacyBrowseOnly = true;
            startTransport(HandshakeKind::AnonymousBrowse);
            return;
        }
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
                if (event.member.memberId == m_selfMemberId) {
                    applySelfMember(event.member);
                }
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
        [this](const InventoryUploadReady& ready) {
            handleInventoryUploadReady(ready);
        },
        [this](const InventoryCommitted& committed) {
            handleInventoryCommitted(committed);
        },
        [this](const AvailabilityTransferBegin& begin) {
            handleAvailabilityTransferBegin(begin);
        },
        [this](const AvailabilityTransferCommit& commit) {
            handleAvailabilityTransferCommit(commit);
        },
        [this](const SelectionChanged& changed) {
            if (!acceptsRoomEvent(changed.roomId, changed.roomGeneration) ||
                changed.selectionRevision < m_selectionRevision) {
                return;
            }
            applySelection(changed.selection,
                           changed.selectionRevision,
                           changed.availabilityRevision,
                           changed.selectedByMemberId);
            if (!m_selectionRequestId.isEmpty() && m_requestedSelection &&
                changed.selection == m_requestedSelection) {
                m_pendingCommands.remove(m_selectionRequestId);
                m_selectionRequestId.clear();
                m_requestedSelection.reset();
            }
        },
        [this](const SelectionRejected& rejected) {
            if (rejected.requestId != m_selectionRequestId) {
                return;
            }
            m_pendingCommands.remove(m_selectionRequestId);
            m_selectionRequestId.clear();
            m_requestedSelection.reset();
            setError(rejected.reason == SelectionRejectionReason::NotCommon
                       ? QStringLiteral("selection_not_common")
                       : QStringLiteral("selection_stale"),
                     rejected.reason == SelectionRejectionReason::NotCommon
                       ? QStringLiteral("arena.error.notCommon")
                       : QStringLiteral("arena.error.selectionStale"));
        },
        [this](const RoundLoadingStarted& started) {
            if (!acceptsRoomEvent(started.roomId, started.roomGeneration)) {
                return;
            }
            if (m_competitionAvailable &&
                !hasCompetitionRoundShape(started.round)) {
                failProtocol(ProtocolFailureCode::MalformedMessage);
                return;
            }
            if (started.round.stage != FrozenRoundStage::Probing) {
                return;
            }
            if (m_round && m_round->roundId == started.round.roundId &&
                m_round->launchAttemptId == started.round.launchAttemptId) {
                return;
            }
            if (m_roomPhase != RoomPhase::Selecting || m_round ||
                started.round.selectionRevision < m_selectionRevision ||
                started.round.availabilityRevision <
                  m_roomAvailabilityRevision ||
                (started.round.selectionRevision == m_selectionRevision &&
                 (!m_selection || started.round.selection != *m_selection))) {
                return;
            }
            const auto lifecycle = m_lifecycleGeneration;
            m_roomPhase = RoomPhase::Loading;
            m_selection = started.round.selection;
            m_selectionRevision = started.round.selectionRevision;
            m_roomAvailabilityRevision = started.round.availabilityRevision;
            m_selectedByMemberId.clear();
            if (!m_selectionRequestId.isEmpty()) {
                m_pendingCommands.remove(m_selectionRequestId);
                m_selectionRequestId.clear();
                m_requestedSelection.reset();
            }
            if (!m_readyRequestId.isEmpty()) {
                m_pendingCommands.remove(m_readyRequestId);
                m_readyRequestId.clear();
                m_requestedReady.reset();
            }
            m_round = started.round;
            setRoundLaunchCancellationStatusKey({});
            if (lifecycle != m_lifecycleGeneration || !m_round ||
                m_round->roundId != started.round.roundId ||
                m_round->launchAttemptId != started.round.launchAttemptId) {
                return;
            }
            beginCompetitionRound(started.round);
            // Any impossible stale local operation is discarded only after
            // the authoritative new state is visible to synchronous UI code.
            cancelPreparedRound(false);
            if (lifecycle != m_lifecycleGeneration || !m_round ||
                m_round->roundId != started.round.roundId ||
                m_round->launchAttemptId != started.round.launchAttemptId) {
                return;
            }
            emit roundChanged();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            emit selectionChanged();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            emit readyChanged();
        },
        [this](const RoundProbeRequested& requested) {
            handleProbeRequested(requested);
        },
        [this](const RoundLoadRequested& requested) {
            handleLoadRequested(requested);
        },
        [this](const RoundStartScheduled& scheduled) {
            handleRoundStartScheduled(scheduled);
        },
        [this](const RoundStarted& started) {
            if (!acceptsRoomEvent(started.roomId, started.roomGeneration)) {
                return;
            }
            if (m_competitionAvailable &&
                !started.playDeadlineAtServerMs.has_value()) {
                failProtocol(ProtocolFailureCode::MalformedMessage);
                return;
            }
            if (m_roomPhase != RoomPhase::Loading || !m_round ||
                m_round->roundId != started.roundId ||
                m_round->launchAttemptId != started.launchAttemptId) {
                return;
            }
            const auto isFrozenParticipant =
              frozenInventoryRevision(*m_round, m_selfMemberId).has_value();
            if ((isFrozenParticipant &&
                 m_round->stage != FrozenRoundStage::Scheduled) ||
                (!isFrozenParticipant &&
                 m_round->stage == FrozenRoundStage::Playing)) {
                return;
            }
            m_roomPhase = RoomPhase::Playing;
            m_round->stage = FrozenRoundStage::Playing;
            m_round->playDeadlineAtServerMs = started.playDeadlineAtServerMs;
            emit roundChanged();
            flushCompetitionMessages();
        },
        [this](const LiveStandingsSnapshot& snapshot) {
            handleStandings(snapshot);
        },
        [this](const RoundTerminalAccepted& accepted) {
            handleTerminalAccepted(accepted);
        },
        [this](const RoundFinalized& finalized) {
            handleRoundFinalized(finalized);
        },
        [this](const RoundLaunchCancelled& cancelled) {
            if (!acceptsRoomEvent(cancelled.roomId, cancelled.roomGeneration) ||
                m_roomPhase != RoomPhase::Loading || !m_round ||
                m_round->stage == FrozenRoundStage::Playing ||
                m_round->roundId != cancelled.roundId ||
                m_round->launchAttemptId != cancelled.launchAttemptId) {
                return;
            }
            const auto lifecycle = m_lifecycleGeneration;
            m_roomPhase = RoomPhase::Selecting;
            m_round.reset();
            clearActiveCompetitionRound();
            applySelection(cancelled.selection,
                           cancelled.selectionRevision,
                           cancelled.availabilityRevision,
                           std::nullopt);
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            setRoundLaunchCancellationStatusKey(
              roundLaunchCancellationStatusKey(cancelled.reason));
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            emit roundChanged();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            emit selectionChanged();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            emit readyChanged();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
            cancelPreparedRound(true);
        },
      },
      message);
}

void
ArenaSession::handleCommandError(const CommandError& error)
{
    if (m_pendingTerminal &&
        terminalRequestId(m_pendingTerminal->message) == error.requestId) {
        setError(commandCode(error.code), error.displayMessageKey);
        if (error.code == CommandErrorCode::RoomGenerationStale ||
            error.code == CommandErrorCode::ConnectionGenerationStale ||
            error.code == CommandErrorCode::RoundStale ||
            error.code == CommandErrorCode::LaunchStageStale ||
            error.code == CommandErrorCode::RoundAlreadyTerminal) {
            beginReconnect();
        } else {
            m_pendingTerminal.reset();
        }
        return;
    }
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
    std::optional<qint64> failedInventoryGeneration;
    if ((kind == PendingCommandKind::InventoryBegin ||
         kind == PendingCommandKind::InventoryCommit) &&
        m_pendingInventoryUpload) {
        failedInventoryGeneration =
          m_pendingInventoryUpload->snapshot.libraryGeneration;
        if (m_uncertainCommittedGeneration == *failedInventoryGeneration) {
            m_uncertainCommittedGeneration = 0;
            m_uncertainBaseInventoryRevision = 0;
        }
        m_pendingInventoryUpload.reset();
        emit selectionChanged();
        emit readyChanged();
    }
    if (kind == PendingCommandKind::AvailabilityResync) {
        m_availabilityResyncRequestId.clear();
    }
    if (kind == PendingCommandKind::AvailabilityApplied &&
        error.code == CommandErrorCode::AvailabilityStale) {
        m_availabilityAppliedRequestId.clear();
        requestAvailabilityResync();
    }
    if (kind == PendingCommandKind::Ready) {
        m_readyRequestId.clear();
        m_requestedReady.reset();
    }
    if (kind == PendingCommandKind::Selection) {
        m_selectionRequestId.clear();
        m_requestedSelection.reset();
    }
    if (kind == PendingCommandKind::ProbeResult) {
        m_probeResultRequestId.clear();
    }
    if (kind == PendingCommandKind::LoadResult) {
        m_loadResultRequestId.clear();
    }
    if ((error.code == CommandErrorCode::RoomGenerationStale ||
         error.code == CommandErrorCode::ConnectionGenerationStale) &&
        !m_roomId.isEmpty()) {
        beginReconnect();
        return;
    }
    if (failedInventoryGeneration && m_inventorySource != nullptr &&
        m_inventorySource->generation() > *failedInventoryGeneration) {
        requestInventorySnapshot();
    }
}

void
ArenaSession::applyRoomSnapshot(const RoomSnapshot& snapshot)
{
    if (m_competitionAvailable && !snapshot.competitionShape) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto resumesSameSeat = !m_roomId.isEmpty() &&
                                 m_roomId == snapshot.roomId &&
                                 m_selfMemberId == snapshot.self.memberId;
    const auto resumesSamePreparedRound =
      resumesSameSeat && m_preparedRunner != nullptr && m_round &&
      snapshot.round && sameFrozenRound(*m_round, *snapshot.round);
    const auto resumesSameCompetitionRound =
      resumesSameSeat && m_round && snapshot.round &&
      sameFrozenRound(*m_round, *snapshot.round);
    const auto resumesFinalizedPresentedRound =
      resumesSameSeat && snapshot.lastRoundResult &&
      m_resultPresentationActive && m_presentedResult.valid() &&
      m_presentedResult.roundId() == snapshot.lastRoundResult->roundId;
    if (!resumesSameCompetitionRound) {
        m_round.reset();
    }
    clearRoundTransfers(
      !resumesSameSeat, resumesSamePreparedRound, resumesSameCompetitionRound);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    if (!resumesSameCompetitionRound) {
        clearActiveCompetitionRound(resumesFinalizedPresentedRound);
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
    }
    const auto membersReplaced = m_members.replace(
      snapshot.members, snapshot.ownerMemberId, snapshot.self.memberId);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    const auto chatReplaced =
      membersReplaced && m_chat.replace(snapshot.chat, snapshot.self.memberId);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    if (!membersReplaced || !chatReplaced) {
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
    m_roomPhase = snapshot.phase;
    m_round = snapshot.round;
    if (m_round) {
        cacheCompetitionIdentities(*m_round);
        const auto summary = optionsSummary(m_round->selection);
        if (summary != m_arenaOptionsSummary) {
            m_arenaOptionsSummary = summary;
            emit competitionChanged();
        }
    }
    applySelection(snapshot.selection,
                   snapshot.selectionRevision,
                   snapshot.availabilityRevision,
                   std::nullopt);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    const auto self = std::find_if(
      snapshot.members.begin(), snapshot.members.end(), [&](const Member& row) {
          return row.memberId == snapshot.self.memberId;
      });
    if (self == snapshot.members.end()) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    applySelfMember(*self);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    if (snapshot.lastRoundResult) {
        if (m_lastResult.valid() &&
            m_lastResult.roundId() != snapshot.lastRoundResult->roundId) {
            m_lastResult.clear();
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
        }
        (void)m_lastResult.replaceFinal(
          *snapshot.lastRoundResult,
          m_selfMemberId,
          optionsSummary(snapshot.lastRoundResult->selection));
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
        if (m_presentedResult.valid() &&
            m_presentedResult.roundId() == snapshot.lastRoundResult->roundId) {
            (void)m_presentedResult.replaceFinal(
              *snapshot.lastRoundResult,
              m_selfMemberId,
              optionsSummary(snapshot.lastRoundResult->selection));
            if (lifecycle != m_lifecycleGeneration) {
                return;
            }
        }
    } else if (!resumesSameSeat) {
        m_lastResult.clear();
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
    }
    if (snapshot.liveStandings) {
        handleStandings(*snapshot.liveStandings);
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
    }
    if (m_uncertainCommittedGeneration > 0) {
        if (m_selfInventoryRevision > m_uncertainBaseInventoryRevision) {
            m_lastPublishedLibraryGeneration =
              (std::max)(m_lastPublishedLibraryGeneration,
                         m_uncertainCommittedGeneration);
        }
        m_uncertainCommittedGeneration = 0;
        m_uncertainBaseInventoryRevision = 0;
    }
    emit roomChanged();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    emit roundChanged();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    if (oldOwner != m_ownerMemberId || oldIsOwner != getIsOwner()) {
        emit ownerChanged();
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
    }
    setAuthenticated(true);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    clearError();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    setState(State::InRoom);
    if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom) {
        return;
    }
    requestInventorySnapshot();
    flushCompetitionMessages();

    if (!resumesSamePreparedRound || !m_round ||
        m_round->stage != FrozenRoundStage::Playing ||
        m_preparedRunner == nullptr) {
        return;
    }
    const auto roundId = m_round->roundId;
    const auto launchAttemptId = m_round->launchAttemptId;
    const auto guardedRunner = m_preparedRunner;
    if (guardedRunner->getStatus() == gameplay_logic::ChartRunner::Finished &&
        !m_gameplaySourceAttached) {
        return;
    }
    if (!attachGameplaySource(guardedRunner)) {
        sendAbandon(DnfReason::ResultUnavailable);
        cancelPreparedRound(false, true);
        return;
    }
    if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom ||
        !m_round || m_round->roundId != roundId ||
        m_round->launchAttemptId != launchAttemptId ||
        guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
        return;
    }
    if (!m_preparedGameplayExposed) {
        m_preparedGameplayExposed = true;
        emit preparedGameplayChanged(guardedRunner);
        if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom ||
            !m_round || m_round->roundId != roundId ||
            m_round->launchAttemptId != launchAttemptId ||
            guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
            return;
        }
    }
    if (guardedRunner->getStatus() == gameplay_logic::ChartRunner::Ready) {
        guardedRunner->start();
        guardedRunner->releaseStart();
    }
    if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom ||
        !m_round || m_round->roundId != roundId ||
        m_round->launchAttemptId != launchAttemptId ||
        guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
        return;
    }
    if (!m_roundRunnerStartedEmitted &&
        guardedRunner->getStatus() == gameplay_logic::ChartRunner::Running) {
        startTelemetrySampling();
        m_roundRunnerStartedEmitted = true;
        emit roundRunnerStarted(roundId, guardedRunner);
    }
}

void
ArenaSession::clearRoom()
{
    m_round.reset();
    clearRoundTransfers();
    clearCompetitionState();
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
    m_roomPhase = RoomPhase::Selecting;
    m_selection.reset();
    m_selectedByMemberId.clear();
    m_selectionRevision = 0;
    m_roomAvailabilityRevision = 0;
    m_selfReady = false;
    m_selfInventoryState = InventoryState::Missing;
    m_selfInventoryRevision = 0;
    m_selfAvailabilityAppliedRevision = 0;
    m_readyRequestId.clear();
    m_requestedReady.reset();
    m_selectionRequestId.clear();
    m_requestedSelection.reset();
    m_members.clear();
    m_chat.clear();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
    setRoundLaunchCancellationStatusKey({});
    if (hadRoom) {
        emit roomChanged();
        emit selectionChanged();
        emit readyChanged();
        emit roundChanged();
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
ArenaSession::applySelfMember(const Member& member)
{
    if (member.memberId != m_selfMemberId) {
        return;
    }
    const auto changed =
      m_selfReady != member.ready ||
      m_selfInventoryState != member.inventoryState ||
      m_selfInventoryRevision != member.inventoryRevision ||
      m_selfAvailabilityAppliedRevision != member.availabilityAppliedRevision;
    m_selfReady = member.ready;
    m_selfInventoryState = member.inventoryState;
    m_selfInventoryRevision = member.inventoryRevision;
    m_selfAvailabilityAppliedRevision = member.availabilityAppliedRevision;
    if (!m_readyRequestId.isEmpty() && m_requestedReady &&
        member.ready == *m_requestedReady) {
        m_pendingCommands.remove(m_readyRequestId);
        m_readyRequestId.clear();
        m_requestedReady.reset();
    }
    if (!m_availabilityAppliedRequestId.isEmpty() &&
        member.availabilityAppliedRevision >= m_roomAvailabilityRevision) {
        m_pendingCommands.remove(m_availabilityAppliedRequestId);
        m_availabilityAppliedRequestId.clear();
    }
    if (changed) {
        emit selectionChanged();
        emit readyChanged();
    }
}

void
ArenaSession::applySelection(std::optional<SelectionSnapshot> selection,
                             qint64 selectionRevision,
                             qint64 availabilityRevision,
                             std::optional<QString> selectedByMemberId)
{
    const auto summary = selection ? optionsSummary(*selection) : QString{};
    const auto summaryChanged = summary != m_arenaOptionsSummary;
    const auto changed =
      m_selection != selection || m_selectionRevision != selectionRevision ||
      m_roomAvailabilityRevision != availabilityRevision ||
      m_selectedByMemberId != selectedByMemberId.value_or(QString{});
    m_selection = std::move(selection);
    m_selectionRevision = selectionRevision;
    m_roomAvailabilityRevision = availabilityRevision;
    m_selectedByMemberId = selectedByMemberId.value_or(QString{});
    m_arenaOptionsSummary = summary;
    if (changed) {
        emit selectionChanged();
        emit readyChanged();
    }
    if (summaryChanged) {
        emit competitionChanged();
    }
}

void
ArenaSession::requestInventorySnapshot()
{
    if (!m_roundsAvailable || m_inventorySource == nullptr ||
        m_roomId.isEmpty() || m_inventorySourceRequestId != 0 ||
        m_pendingInventoryUpload.has_value()) {
        return;
    }
    const auto generation = m_inventorySource->generation();
    if (generation <= m_lastPublishedLibraryGeneration) {
        return;
    }
    m_inventorySourceRequestId = m_nextInventorySourceRequestId++;
    m_inventorySourceRequestGeneration = generation;
    m_inventorySource->requestSnapshot(m_inventorySourceRequestId);
}

void
ArenaSession::cancelInventorySnapshot()
{
    if (m_inventorySource == nullptr || m_inventorySourceRequestId == 0) {
        return;
    }
    m_inventorySource->cancel(m_inventorySourceRequestId);
    m_inventorySourceRequestId = 0;
    m_inventorySourceRequestGeneration = 0;
}

void
ArenaSession::handleInventoryGenerationChanged(qint64 generation)
{
    if (generation <= m_lastPublishedLibraryGeneration || m_roomId.isEmpty() ||
        !m_roundsAvailable) {
        return;
    }
    if (m_inventorySourceRequestId != 0) {
        cancelInventorySnapshot();
    }
    requestInventorySnapshot();
}

void
ArenaSession::handleInventorySnapshotReady(quint64 requestId,
                                           ArenaInventorySnapshot snapshot)
{
    if (requestId == 0 || requestId != m_inventorySourceRequestId ||
        m_roomId.isEmpty() || !m_roundsAvailable) {
        return;
    }
    const auto requestedGeneration = m_inventorySourceRequestGeneration;
    m_inventorySourceRequestId = 0;
    m_inventorySourceRequestGeneration = 0;
    if (snapshot.libraryGeneration != requestedGeneration ||
        snapshot.libraryGeneration <= m_lastPublishedLibraryGeneration ||
        snapshot.packedSha256.size() % ArenaSha256Bytes != 0 ||
        snapshot.packedSha256.size() > ArenaMaxInventoryBytes) {
        setError(QStringLiteral("inventory_invalid"),
                 QStringLiteral("arena.error.inventoryInvalid"));
        requestInventorySnapshot();
        return;
    }

    const auto hashCount = snapshot.packedSha256.size() / ArenaSha256Bytes;
    const auto chunkCount =
      (hashCount + ArenaMaxHashesPerChunk - 1) / ArenaMaxHashesPerChunk;
    auto upload = PendingInventoryUpload{
        .snapshot = std::move(snapshot),
        .beginRequestId = nextRequestId(),
    };
    upload.digest = packedDigest(upload.snapshot.packedSha256);
    const auto command = InventoryUploadBegin{
        .requestId = upload.beginRequestId,
        .roomId = m_roomId,
        .roomGeneration = m_roomGeneration,
        .connectionGeneration = m_connectionGeneration,
        .libraryGeneration = upload.snapshot.libraryGeneration,
        .hashCount = hashCount,
        .byteCount = upload.snapshot.packedSha256.size(),
        .chunkCount = chunkCount,
        .vectorDigest = upload.digest,
    };
    if (!sendMessage(command)) {
        return;
    }
    m_pendingCommands.insert(
      upload.beginRequestId,
      PendingCommand{ .kind = PendingCommandKind::InventoryBegin,
                      .lifecycleGeneration = m_lifecycleGeneration });
    m_pendingInventoryUpload = std::move(upload);
    emit selectionChanged();
    emit readyChanged();
}

void
ArenaSession::handleInventorySnapshotFailed(quint64 requestId,
                                            ArenaInventoryFailure failure)
{
    if (requestId == 0 || requestId != m_inventorySourceRequestId) {
        return;
    }
    m_inventorySourceRequestId = 0;
    m_inventorySourceRequestGeneration = 0;
    if (failure != ArenaInventoryFailure::Cancelled) {
        setError(QStringLiteral("inventory_build_failed"),
                 QStringLiteral("arena.error.inventoryBuildFailed"));
    }
}

void
ArenaSession::handleInventoryUploadReady(const InventoryUploadReady& ready)
{
    if (!m_pendingInventoryUpload ||
        ready.requestId != m_pendingInventoryUpload->beginRequestId ||
        !m_pendingInventoryUpload->commitRequestId.isEmpty()) {
        return;
    }
    auto& upload = *m_pendingInventoryUpload;
    const auto hashCount =
      upload.snapshot.packedSha256.size() / ArenaSha256Bytes;
    const auto chunkCount =
      (hashCount + ArenaMaxHashesPerChunk - 1) / ArenaMaxHashesPerChunk;
    if (!acceptsRoomEvent(ready.roomId, ready.roomGeneration)) {
        return;
    }
    if (ready.connectionGeneration != m_connectionGeneration ||
        ready.libraryGeneration != upload.snapshot.libraryGeneration ||
        ready.hashCount != hashCount ||
        ready.byteCount != upload.snapshot.packedSha256.size() ||
        ready.chunkCount != chunkCount || ready.vectorDigest != upload.digest) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    const auto transferId = decodeTransferId(ready.uploadId);
    if (transferId.isEmpty()) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    m_pendingCommands.remove(upload.beginRequestId);
    upload.uploadId = ready.uploadId;
    for (qint64 chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex) {
        const auto firstHash = chunkIndex * ArenaMaxHashesPerChunk;
        const auto hashesInChunk =
          std::min<qint64>(ArenaMaxHashesPerChunk, hashCount - firstHash);
        const auto encoded = encodeArenaBinaryChunk(ArenaBinaryChunk{
          .kind = ArenaBinaryKind::InventoryUpload,
          .transferId = transferId,
          .chunkIndex = static_cast<quint32>(chunkIndex),
          .packedHashes = upload.snapshot.packedSha256.mid(
            firstHash * ArenaSha256Bytes, hashesInChunk * ArenaSha256Bytes),
        });
        const auto* bytes = std::get_if<QByteArray>(&encoded);
        if (bytes == nullptr) {
            failProtocol(ProtocolFailureCode::MalformedMessage);
            return;
        }
        m_transport->sendBinary(m_currentTransportGeneration, *bytes);
    }
    upload.commitRequestId = nextRequestId();
    const auto command = InventoryUploadCommit{
        .requestId = upload.commitRequestId,
        .roomId = m_roomId,
        .roomGeneration = m_roomGeneration,
        .connectionGeneration = m_connectionGeneration,
        .uploadId = upload.uploadId,
        .libraryGeneration = upload.snapshot.libraryGeneration,
        .hashCount = hashCount,
        .byteCount = upload.snapshot.packedSha256.size(),
        .chunkCount = chunkCount,
        .vectorDigest = upload.digest,
    };
    if (!sendMessage(command)) {
        return;
    }
    m_pendingCommands.insert(
      upload.commitRequestId,
      PendingCommand{ .kind = PendingCommandKind::InventoryCommit,
                      .lifecycleGeneration = m_lifecycleGeneration });
    m_uncertainCommittedGeneration = upload.snapshot.libraryGeneration;
    m_uncertainBaseInventoryRevision = m_selfInventoryRevision;
}

void
ArenaSession::handleInventoryCommitted(const InventoryCommitted& committed)
{
    if (!m_pendingInventoryUpload ||
        committed.requestId != m_pendingInventoryUpload->commitRequestId) {
        return;
    }
    const auto& upload = *m_pendingInventoryUpload;
    if (!acceptsRoomEvent(committed.roomId, committed.roomGeneration)) {
        return;
    }
    if (committed.connectionGeneration != m_connectionGeneration ||
        committed.libraryGeneration != upload.snapshot.libraryGeneration ||
        committed.inventoryRevision <= 0 ||
        committed.inventoryState != InventoryState::Ready) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    m_pendingCommands.remove(upload.commitRequestId);
    m_lastPublishedLibraryGeneration = committed.libraryGeneration;
    m_uncertainCommittedGeneration = 0;
    m_uncertainBaseInventoryRevision = 0;
    m_pendingInventoryUpload.reset();
    emit selectionChanged();
    emit readyChanged();
    clearError();
    requestInventorySnapshot();
}

void
ArenaSession::handleAvailabilityTransferBegin(
  const AvailabilityTransferBegin& begin)
{
    if (!acceptsRoomEvent(begin.roomId, begin.roomGeneration)) {
        return;
    }
    const auto transferId = decodeTransferId(begin.transferId);
    if (transferId.isEmpty()) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    if (begin.targetRevision < m_availability.revision() ||
        (m_pendingAvailabilityTransfer &&
         begin.targetRevision <=
           m_pendingAvailabilityTransfer->declaration.targetRevision &&
         begin.transferId !=
           m_pendingAvailabilityTransfer->declaration.transferId)) {
        return;
    }
    if (m_pendingAvailabilityTransfer &&
        begin.transferId ==
          m_pendingAvailabilityTransfer->declaration.transferId) {
        return;
    }
    if (!m_availabilityResyncRequestId.isEmpty()) {
        m_pendingCommands.remove(m_availabilityResyncRequestId);
        m_availabilityResyncRequestId.clear();
    }
    m_pendingAvailabilityTransfer = PendingAvailabilityTransfer{
        .declaration = begin,
        .transferId = transferId,
    };
    m_availability.setSyncing();
}

void
ArenaSession::handleAvailabilityTransferCommit(
  const AvailabilityTransferCommit& commit)
{
    if (!m_pendingAvailabilityTransfer ||
        !acceptsRoomEvent(commit.roomId, commit.roomGeneration)) {
        return;
    }
    if (commit.transferId !=
        m_pendingAvailabilityTransfer->declaration.transferId) {
        return;
    }
    auto transfer = std::move(*m_pendingAvailabilityTransfer);
    m_pendingAvailabilityTransfer.reset();
    const auto& declaration = transfer.declaration;
    if (commit.targetRevision != declaration.targetRevision) {
        requestAvailabilityResync();
        return;
    }
    const auto validVector = [](const QByteArray& packed,
                                qint64 count,
                                quint32 chunks,
                                qint64 declaredChunks,
                                QStringView digest) {
        return packed.size() == count * ArenaSha256Bytes &&
               chunks == declaredChunks && packedDigest(packed) == digest;
    };
    bool applied = false;
    if (declaration.mode == AvailabilityTransferMode::Reset) {
        applied = validVector(transfer.reset,
                              declaration.resetCount,
                              transfer.resetChunks,
                              declaration.resetChunkCount,
                              declaration.resetDigest) &&
                  m_availability.applyReset(declaration.targetRevision,
                                            std::move(transfer.reset));
    } else {
        applied = validVector(transfer.added,
                              declaration.addedCount,
                              transfer.addedChunks,
                              declaration.addedChunkCount,
                              declaration.addedDigest) &&
                  validVector(transfer.removed,
                              declaration.removedCount,
                              transfer.removedChunks,
                              declaration.removedChunkCount,
                              declaration.removedDigest) &&
                  m_availability.applyDelta(declaration.baseRevision,
                                            declaration.targetRevision,
                                            std::move(transfer.added),
                                            std::move(transfer.removed));
    }
    if (!applied) {
        requestAvailabilityResync();
        return;
    }
    if (m_roomAvailabilityRevision != declaration.targetRevision) {
        m_roomAvailabilityRevision = declaration.targetRevision;
        emit selectionChanged();
        emit readyChanged();
    }
    const auto requestId = nextRequestId();
    if (!m_availabilityAppliedRequestId.isEmpty()) {
        m_pendingCommands.remove(m_availabilityAppliedRequestId);
    }
    if (sendMessage(AvailabilityApplied{
          .requestId = requestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .availabilityRevision = declaration.targetRevision,
        })) {
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::AvailabilityApplied,
                          .lifecycleGeneration = m_lifecycleGeneration });
        m_availabilityAppliedRequestId = requestId;
    }
}

void
ArenaSession::requestAvailabilityResync()
{
    m_pendingAvailabilityTransfer.reset();
    m_availability.setSyncing();
    if (m_roomId.isEmpty() || !m_roundsAvailable ||
        !m_availabilityResyncRequestId.isEmpty()) {
        return;
    }
    m_availabilityResyncRequestId = nextRequestId();
    if (sendMessage(AvailabilityResync{
          .requestId = m_availabilityResyncRequestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .currentRevision = m_availability.revision(),
        })) {
        m_pendingCommands.insert(
          m_availabilityResyncRequestId,
          PendingCommand{ .kind = PendingCommandKind::AvailabilityResync,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::cancelRoundLoader()
{
    const auto requestId = std::exchange(m_roundLoaderRequestId, quint64{});
    m_roundLoaderOperation = RoundLoaderOperation::None;
    m_probeRequest.reset();
    m_loadRequestRound.reset();
    if (requestId != 0 && m_roundLoader != nullptr) {
        m_roundLoader->cancel(requestId);
    }
}

void
ArenaSession::handleProbeRequested(const RoundProbeRequested& requested)
{
    if (!acceptsRoomEvent(requested.roomId, requested.roomGeneration) ||
        requested.connectionGeneration != m_connectionGeneration ||
        m_roomPhase != RoomPhase::Loading || m_roundLoader == nullptr ||
        !m_round || m_round->stage != FrozenRoundStage::Probing ||
        requested.roundId != m_round->roundId ||
        requested.launchAttemptId != m_round->launchAttemptId ||
        requested.selectionRevision != m_round->selectionRevision ||
        requested.availabilityRevision != m_round->availabilityRevision ||
        requested.sha256 != m_round->selection.sha256) {
        return;
    }
    const auto frozenRevision =
      frozenInventoryRevision(*m_round, m_selfMemberId);
    if (!frozenRevision || requested.inventoryRevision != *frozenRevision) {
        return;
    }
    cancelRoundLoader();
    auto requestId = m_nextRoundLoaderRequestId++;
    if (requestId == 0) {
        requestId = m_nextRoundLoaderRequestId++;
    }
    m_roundLoaderRequestId = requestId;
    m_roundLoaderOperation = RoundLoaderOperation::Probe;
    m_probeRequest = requested;
    m_round->stage = FrozenRoundStage::Probing;
    emit roundChanged();
    m_roundLoader->probe(requestId,
                         QByteArray::fromHex(requested.sha256.toLatin1()));
}

void
ArenaSession::handleLoadRequested(const RoundLoadRequested& requested)
{
    if (!acceptsRoomEvent(requested.roomId, requested.roomGeneration)) {
        return;
    }
    if (m_competitionAvailable && !hasCompetitionRoundShape(requested.round)) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    if (requested.connectionGeneration != m_connectionGeneration ||
        m_roomPhase != RoomPhase::Loading || m_roundLoader == nullptr ||
        !m_round || requested.round.stage != FrozenRoundStage::Loading ||
        (m_round->stage != FrozenRoundStage::Probing &&
         m_round->stage != FrozenRoundStage::Loading) ||
        !sameFrozenRound(requested.round, *m_round)) {
        return;
    }
    if (!frozenInventoryRevision(requested.round, m_selfMemberId)) {
        return;
    }
    if (!m_probeResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_probeResultRequestId);
        m_probeResultRequestId.clear();
    }
    cancelPreparedRound(false);
    const auto& selection = requested.round.selection;
    bool seedOk = false;
    const auto laneSeed = selection.laneSeed.toULongLong(&seedOk, 16);
    const auto sha256 = QByteArray::fromHex(selection.sha256.toLatin1());
    if (!seedOk || sha256.size() != ArenaSha256Bytes) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    auto requestId = m_nextRoundLoaderRequestId++;
    if (requestId == 0) {
        requestId = m_nextRoundLoaderRequestId++;
    }
    auto randomSequence = QList<qint64>{};
    randomSequence.reserve(selection.randomSequence.size());
    for (const auto value : selection.randomSequence) {
        randomSequence.push_back(value);
    }
    const auto request = ArenaRoundLoadRequest{
        .sha256 = sha256,
        .playConfig =
          resource_managers::ChartPlayConfig{
            .randomSequence = std::move(randomSequence),
            .noteOrderP1 = playNoteOrder(selection.noteOrderP1),
            .noteOrderP2 = playNoteOrder(selection.noteOrderP2),
            .dpMode = playDpMode(selection.dpMode),
            .laneSeed = laneSeed,
            .randomizationVersion = selection.randomizationVersion,
          },
    };
    m_roundLoaderRequestId = requestId;
    m_roundLoaderOperation = RoundLoaderOperation::Load;
    m_loadRequestRound = requested.round;
    m_round = requested.round;
    m_round->stage = FrozenRoundStage::Loading;
    emit roundChanged();
    m_roundLoader->load(requestId, request);
}

void
ArenaSession::handleProbeFinished(quint64 requestId, ArenaProbeResult result)
{
    if (requestId == 0 || requestId != m_roundLoaderRequestId ||
        m_roundLoaderOperation != RoundLoaderOperation::Probe ||
        !m_probeRequest) {
        return;
    }
    auto requested = std::move(*m_probeRequest);
    m_probeRequest.reset();
    m_roundLoaderRequestId = 0;
    m_roundLoaderOperation = RoundLoaderOperation::None;
    auto failure = probeFailureReason(result.failure);
    const auto expectedSha256 =
      QByteArray::fromHex(requested.sha256.toLatin1());
    if (!failure && result.observedSha256 != expectedSha256) {
        failure = result.observedSha256.size() == ArenaSha256Bytes
                    ? RoundProbeFailureReason::HashMismatch
                    : RoundProbeFailureReason::ReadFailed;
    }
    const auto ok = !failure;
    const auto protocolRequestId = nextRequestId();
    if (sendMessage(RoundProbeResult{
          .requestId = protocolRequestId,
          .roomId = requested.roomId,
          .roomGeneration = requested.roomGeneration,
          .connectionGeneration = requested.connectionGeneration,
          .roundId = requested.roundId,
          .launchAttemptId = requested.launchAttemptId,
          .selectionRevision = requested.selectionRevision,
          .availabilityRevision = requested.availabilityRevision,
          .inventoryRevision = requested.inventoryRevision,
          .nonce = requested.nonce,
          .ok = ok,
          .sha256 = ok ? std::optional<QString>{ QString::fromLatin1(
                           result.observedSha256.toHex()) }
                       : std::nullopt,
          .failureReason = failure,
        })) {
        if (!m_probeResultRequestId.isEmpty()) {
            m_pendingCommands.remove(m_probeResultRequestId);
        }
        m_probeResultRequestId = protocolRequestId;
        m_pendingCommands.insert(
          protocolRequestId,
          PendingCommand{ .kind = PendingCommandKind::ProbeResult,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::handleLoadFinished(quint64 requestId,
                                 gameplay_logic::ChartRunner* runner)
{
    if (requestId == 0 || requestId != m_roundLoaderRequestId ||
        m_roundLoaderOperation != RoundLoaderOperation::Load ||
        !m_loadRequestRound || m_preparedRunner != nullptr ||
        runner == nullptr ||
        runner->getStatus() != gameplay_logic::ChartRunner::Ready) {
        return;
    }
    constexpr qint64 NanosecondsPerMillisecond = 1'000'000;
    const auto* player = runner->getPlayer1();
    const auto chartLengthNs =
      player != nullptr ? player->getChartLength() : qint64{ -1 };
    if (chartLengthNs < 0 ||
        chartLengthNs > MaxChartLengthMs * NanosecondsPerMillisecond) {
        handleLoadFailed(requestId, ArenaLoadFailure::ResourceFailed);
        if (m_roundLoader != nullptr) {
            m_roundLoader->cancel(requestId);
        }
        return;
    }
    const auto chartLengthMs =
      chartLengthNs / NanosecondsPerMillisecond +
      (chartLengthNs % NanosecondsPerMillisecond == 0 ? 0 : 1);
    m_preparedRunner = runner;
    const auto& round = *m_loadRequestRound;
    const auto inventoryRevision =
      frozenInventoryRevision(round, m_selfMemberId);
    if (!inventoryRevision) {
        cancelPreparedRound(false);
        return;
    }
    const auto protocolRequestId = nextRequestId();
    if (sendMessage(RoundLoadResult{
          .requestId = protocolRequestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .roundId = round.roundId,
          .launchAttemptId = round.launchAttemptId,
          .selectionRevision = round.selectionRevision,
          .availabilityRevision = round.availabilityRevision,
          .inventoryRevision = *inventoryRevision,
          .ok = true,
          .chartLengthMs = chartLengthMs,
        })) {
        if (!m_loadResultRequestId.isEmpty()) {
            m_pendingCommands.remove(m_loadResultRequestId);
        }
        m_loadResultRequestId = protocolRequestId;
        m_pendingCommands.insert(
          protocolRequestId,
          PendingCommand{ .kind = PendingCommandKind::LoadResult,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::handleLoadFailed(quint64 requestId, ArenaLoadFailure failure)
{
    if (requestId == 0 || requestId != m_roundLoaderRequestId ||
        m_roundLoaderOperation != RoundLoaderOperation::Load ||
        !m_loadRequestRound) {
        return;
    }
    auto round = std::move(*m_loadRequestRound);
    m_loadRequestRound.reset();
    m_roundLoaderRequestId = 0;
    m_roundLoaderOperation = RoundLoaderOperation::None;
    m_preparedRunner = nullptr;
    const auto inventoryRevision =
      frozenInventoryRevision(round, m_selfMemberId);
    if (!inventoryRevision) {
        return;
    }
    const auto reason = loadFailureReason(failure);
    const auto protocolRequestId = nextRequestId();
    if (sendMessage(RoundLoadResult{
          .requestId = protocolRequestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .roundId = round.roundId,
          .launchAttemptId = round.launchAttemptId,
          .selectionRevision = round.selectionRevision,
          .availabilityRevision = round.availabilityRevision,
          .inventoryRevision = *inventoryRevision,
          .ok = false,
          .failureReason = reason,
        })) {
        if (!m_loadResultRequestId.isEmpty()) {
            m_pendingCommands.remove(m_loadResultRequestId);
        }
        m_loadResultRequestId = protocolRequestId;
        m_pendingCommands.insert(
          protocolRequestId,
          PendingCommand{ .kind = PendingCommandKind::LoadResult,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::handleRoundStartScheduled(const RoundStartScheduled& scheduled)
{
    const auto receivedAtMs = m_scheduler->monotonicNowMs();
    if (!acceptsRoomEvent(scheduled.roomId, scheduled.roomGeneration)) {
        return;
    }
    if (m_competitionAvailable &&
        !scheduled.playDeadlineAtServerMs.has_value()) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    if (scheduled.connectionGeneration != m_connectionGeneration ||
        m_roomPhase != RoomPhase::Loading || !m_round ||
        (m_round->stage != FrozenRoundStage::Loading &&
         m_round->stage != FrozenRoundStage::Scheduled) ||
        !m_loadRequestRound || m_preparedRunner == nullptr ||
        scheduled.roundId != m_round->roundId ||
        scheduled.launchAttemptId != m_round->launchAttemptId ||
        m_preparedRunner->getStatus() != gameplay_logic::ChartRunner::Ready) {
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    if (!m_loadResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_loadResultRequestId);
        m_loadResultRequestId.clear();
    }
    cancelTask(m_roundStartTask);
    m_round->stage = FrozenRoundStage::Scheduled;
    m_round->playDeadlineAtServerMs = scheduled.playDeadlineAtServerMs;
    emit roundChanged();
    if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom ||
        !m_round || m_round->roundId != scheduled.roundId ||
        m_round->launchAttemptId != scheduled.launchAttemptId ||
        m_preparedRunner == nullptr) {
        return;
    }
    const auto guardedRunner = m_preparedRunner;
    if (!attachGameplaySource(guardedRunner)) {
        sendAbandon(DnfReason::ResultUnavailable);
        cancelPreparedRound(false, true);
        return;
    }
    if (lifecycle != m_lifecycleGeneration || m_state != State::InRoom ||
        !m_round || m_round->roundId != scheduled.roundId ||
        m_round->launchAttemptId != scheduled.launchAttemptId ||
        guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
        return;
    }
    if (!m_preparedGameplayExposed) {
        m_preparedGameplayExposed = true;
        emit preparedGameplayChanged(guardedRunner);
    }
    if (lifecycle != m_lifecycleGeneration || guardedRunner == nullptr ||
        guardedRunner != m_preparedRunner || !m_round ||
        m_round->roundId != scheduled.roundId ||
        m_round->launchAttemptId != scheduled.launchAttemptId) {
        return;
    }
    guardedRunner->start();
    const auto roundId = scheduled.roundId;
    const auto launchAttemptId = scheduled.launchAttemptId;
    const auto releaseAtMs =
      scheduled.startAfterMs <=
          (std::numeric_limits<qint64>::max)() - receivedAtMs
        ? receivedAtMs + scheduled.startAfterMs
        : (std::numeric_limits<qint64>::max)();
    const auto nowMs = m_scheduler->monotonicNowMs();
    const auto remainingMs = nowMs >= releaseAtMs ? 0 : releaseAtMs - nowMs;
    m_roundStartTask = m_scheduler->scheduleOnce(
      remainingMs,
      this,
      [this, lifecycle, roundId, launchAttemptId, guardedRunner] {
          m_roundStartTask = ArenaScheduler::InvalidTaskId;
          if (lifecycle != m_lifecycleGeneration || !m_round ||
              m_round->roundId != roundId ||
              m_round->launchAttemptId != launchAttemptId ||
              guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
              return;
          }
          guardedRunner->releaseStart();
          if (lifecycle != m_lifecycleGeneration || !m_round ||
              m_round->roundId != roundId ||
              m_round->launchAttemptId != launchAttemptId ||
              guardedRunner == nullptr || guardedRunner != m_preparedRunner) {
              return;
          }
          startTelemetrySampling();
          m_roundRunnerStartedEmitted = true;
          emit roundRunnerStarted(roundId, guardedRunner);
      });
}

void
ArenaSession::cancelPreparedRound(bool notify, bool preserveScoreGuid)
{
    const auto lifecycle = m_lifecycleGeneration;
    const auto hadExposedGameplay = m_preparedGameplayExposed;
    cancelTask(m_roundStartTask);
    stopTelemetrySampling();
    detachGameplaySource(preserveScoreGuid);
    setGameplayChatOpen(false);
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    m_preparedRunner = nullptr;
    m_preparedGameplayExposed = false;
    m_roundRunnerStartedEmitted = false;
    if (hadExposedGameplay) {
        emit preparedGameplayChanged(nullptr);
        if (lifecycle != m_lifecycleGeneration) {
            return;
        }
    }
    if (notify && hadExposedGameplay) {
        emit roundLaunchCancelled();
    }
    // The UI observes the synchronous signals above before the loader is
    // allowed to destroy its owned runner.
    cancelRoundLoader();
    if (!m_probeResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_probeResultRequestId);
        m_probeResultRequestId.clear();
    }
    if (!m_loadResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_loadResultRequestId);
        m_loadResultRequestId.clear();
    }
}

void
ArenaSession::retainPreparedGameplayUntilReleased()
{
    const auto hadExposedGameplay = m_preparedGameplayExposed;
    cancelTask(m_roundStartTask);
    stopTelemetrySampling();
    detachGameplaySource();
    setGameplayChatOpen(false);

    Q_ASSERT(m_retainedGameplayRunner == nullptr ||
             m_retainedGameplayRunner == m_preparedRunner);
    m_retainedGameplayRunner = m_preparedRunner;
    m_retainedGameplayLoadRequestId =
      std::exchange(m_roundLoaderRequestId, quint64{});
    m_roundLoaderOperation = RoundLoaderOperation::None;
    m_probeRequest.reset();
    m_loadRequestRound.reset();
    m_preparedRunner = nullptr;
    m_preparedGameplayExposed = false;
    m_roundRunnerStartedEmitted = false;
    if (!m_probeResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_probeResultRequestId);
        m_probeResultRequestId.clear();
    }
    if (!m_loadResultRequestId.isEmpty()) {
        m_pendingCommands.remove(m_loadResultRequestId);
        m_loadResultRequestId.clear();
    }
    if (hadExposedGameplay) {
        emit preparedGameplayChanged(nullptr);
    }
}

void
ArenaSession::clearRoundTransfers(bool abandonSeat,
                                  bool preservePreparedRound,
                                  bool preserveCompetitionScore)
{
    if (preservePreparedRound) {
        cancelTask(m_roundStartTask);
        if (!m_probeResultRequestId.isEmpty()) {
            m_pendingCommands.remove(m_probeResultRequestId);
            m_probeResultRequestId.clear();
        }
        if (!m_loadResultRequestId.isEmpty()) {
            m_pendingCommands.remove(m_loadResultRequestId);
            m_loadResultRequestId.clear();
        }
    } else {
        cancelPreparedRound(false, preserveCompetitionScore);
    }
    const auto hadPendingUpload = m_pendingInventoryUpload.has_value();
    cancelInventorySnapshot();
    m_pendingInventoryUpload.reset();
    m_pendingAvailabilityTransfer.reset();
    m_availabilityResyncRequestId.clear();
    m_availabilityAppliedRequestId.clear();
    m_readyRequestId.clear();
    m_requestedReady.reset();
    m_selectionRequestId.clear();
    m_requestedSelection.reset();
    if (abandonSeat) {
        m_lastPublishedLibraryGeneration = 0;
        m_uncertainCommittedGeneration = 0;
        m_uncertainBaseInventoryRevision = 0;
    }
    m_availability.clear();
    if (hadPendingUpload) {
        emit selectionChanged();
        emit readyChanged();
    }
}

void
ArenaSession::cacheCompetitionIdentities(const FrozenRound& round)
{
    for (const auto& participant : round.participants) {
        if (participant.identity) {
            m_competitionIdentities.insert(participant.memberId,
                                           *participant.identity);
        }
    }
}

void
ArenaSession::beginCompetitionRound(const FrozenRound& round)
{
    const auto lifecycle = m_lifecycleGeneration;
    setOverlayCustomizationActive(false);
    stopTelemetrySampling();
    m_pendingTelemetry.reset();
    m_pendingTerminal.reset();
    m_nextTelemetrySequence = 1;
    m_localRoundAbandoned = false;
    m_localTerminalSubmitted = false;
    m_liveStandings.clear();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    m_opponentTarget.clear();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    m_competitionIdentities.clear();
    cacheCompetitionIdentities(round);
    const auto hadPresentation = m_resultPresentationActive;
    m_resultPresentationActive = false;
    m_presentedResult.clear();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    const auto summary = optionsSummary(round.selection);
    const auto summaryChanged = summary != m_arenaOptionsSummary;
    m_arenaOptionsSummary = summary;
    if (hadPresentation || summaryChanged) {
        emit competitionChanged();
    }
}

void
ArenaSession::clearActiveCompetitionRound(bool preservePresentedResult)
{
    stopTelemetrySampling();
    m_pendingTelemetry.reset();
    m_pendingTerminal.reset();
    m_nextTelemetrySequence = 1;
    m_nextTelemetryDueMs = 0;
    m_localRoundAbandoned = false;
    m_localTerminalSubmitted = false;
    detachGameplaySource();
    setGameplayChatOpen(false);
    m_liveStandings.clear();
    m_opponentTarget.clear();
    m_competitionIdentities.clear();
    auto visibleChanged = false;
    if (!preservePresentedResult) {
        visibleChanged = m_resultPresentationActive;
        m_resultPresentationActive = false;
        m_presentedResult.clear();
    }
    if (!m_arenaOptionsSummary.isEmpty()) {
        m_arenaOptionsSummary.clear();
        visibleChanged = true;
    }
    if (visibleChanged) {
        emit competitionChanged();
    }
}

void
ArenaSession::clearCompetitionState(bool clearLastResult)
{
    clearActiveCompetitionRound();
    if (clearLastResult) {
        m_lastResult.clear();
    }
}

auto
ArenaSession::attachGameplaySource(gameplay_logic::ChartRunner* runner) -> bool
{
    if (runner == nullptr || m_gameplaySource == nullptr) {
        return false;
    }
    if (m_arenaRunner == runner) {
        return true;
    }
    const auto preserveCustomization = m_overlayCustomizationActive;
    const auto lifecycle = m_lifecycleGeneration;
    detachGameplaySource(false, preserveCustomization);
    if (lifecycle != m_lifecycleGeneration) {
        setOverlayCustomizationActive(false);
        if (preserveCustomization) {
            emit competitionChanged();
        }
        return false;
    }
    const auto attached = m_gameplaySource->attach(runner);
    if (lifecycle != m_lifecycleGeneration || !attached ||
        attached->isEmpty()) {
        m_gameplaySource->detach();
        setOverlayCustomizationActive(false);
        if (preserveCustomization) {
            emit competitionChanged();
        }
        return false;
    }
    m_expectedScoreGuid = *attached;
    m_gameplaySourceAttached = true;
    m_arenaRunner = runner;
    m_arenaGameplayActive =
      runner->getStatus() != gameplay_logic::ChartRunner::Finished;
    m_arenaRunnerStatusConnection =
      connect(runner,
              &gameplay_logic::ChartRunner::statusChanged,
              this,
              &ArenaSession::handleArenaRunnerStatusChanged);
    if (preserveCustomization && m_arenaGameplayActive) {
        synchronizeCustomizationRunner();
    } else if (preserveCustomization) {
        setOverlayCustomizationActive(false);
    }
    emit competitionChanged();
    return true;
}

void
ArenaSession::detachGameplaySource(bool preserveScoreGuid,
                                   bool preserveCustomization)
{
    if (preserveCustomization) {
        releaseCustomizationRunner();
    } else {
        setOverlayCustomizationActive(false);
    }
    const auto visibleChanged =
      m_arenaRunner != nullptr || m_arenaGameplayActive;
    QObject::disconnect(m_arenaRunnerStatusConnection);
    m_arenaRunnerStatusConnection = {};
    if (m_gameplaySourceAttached && m_gameplaySource != nullptr) {
        m_gameplaySource->detach();
    }
    m_gameplaySourceAttached = false;
    m_arenaRunner = nullptr;
    m_arenaGameplayActive = false;
    if (!preserveScoreGuid) {
        m_expectedScoreGuid.clear();
    }
    if (visibleChanged && !preserveCustomization) {
        emit competitionChanged();
    }
}

void
ArenaSession::handleArenaRunnerStatusChanged()
{
    if (m_arenaRunner == nullptr ||
        m_arenaRunner->getStatus() != gameplay_logic::ChartRunner::Finished) {
        return;
    }
    setOverlayCustomizationActive(false);
    stopTelemetrySampling();
    setGameplayChatOpen(false);
    if (m_arenaGameplayActive) {
        m_arenaGameplayActive = false;
        emit competitionChanged();
    }
}

void
ArenaSession::startTelemetrySampling()
{
    if (m_gameplaySource == nullptr || !m_gameplaySourceAttached ||
        m_arenaRunner == nullptr ||
        m_arenaRunner->getStatus() != gameplay_logic::ChartRunner::Running ||
        m_pendingTerminal || m_localTerminalSubmitted ||
        m_localRoundAbandoned || !m_round) {
        return;
    }
    if (m_telemetryTask != ArenaScheduler::InvalidTaskId) {
        return;
    }
    constexpr qint64 IntervalMs = 200;
    const auto now = m_scheduler->monotonicNowMs();
    m_nextTelemetryDueMs =
      now <= (std::numeric_limits<qint64>::max)() - IntervalMs
        ? now + IntervalMs
        : (std::numeric_limits<qint64>::max)();
    scheduleTelemetryTick();
}

void
ArenaSession::scheduleTelemetryTick()
{
    if (m_nextTelemetryDueMs <= 0 ||
        m_telemetryTask != ArenaScheduler::InvalidTaskId) {
        return;
    }
    const auto now = m_scheduler->monotonicNowMs();
    const auto delay =
      now >= m_nextTelemetryDueMs ? qint64{ 0 } : m_nextTelemetryDueMs - now;
    m_telemetryTask = m_scheduler->scheduleOnce(delay, this, [this] {
        m_telemetryTask = ArenaScheduler::InvalidTaskId;
        sampleTelemetry();
    });
}

void
ArenaSession::stopTelemetrySampling()
{
    cancelTask(m_telemetryTask);
    m_nextTelemetryDueMs = 0;
}

void
ArenaSession::sampleTelemetry()
{
    constexpr qint64 IntervalMs = 200;
    if (m_gameplaySource == nullptr || !m_gameplaySourceAttached ||
        m_arenaRunner == nullptr ||
        m_arenaRunner->getStatus() != gameplay_logic::ChartRunner::Running ||
        m_pendingTerminal || m_localTerminalSubmitted ||
        m_localRoundAbandoned || !m_round) {
        stopTelemetrySampling();
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto roundId = m_round->roundId;
    const auto launchAttemptId = m_round->launchAttemptId;
    const auto guardedRunner = m_arenaRunner;
    const auto requestedSequence = m_nextTelemetrySequence;
    const auto sample = m_gameplaySource->sample(requestedSequence);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId) ||
        guardedRunner == nullptr || guardedRunner != m_arenaRunner) {
        return;
    }
    if (sample && sample->sequence == requestedSequence) {
        sendOrQueueTelemetry(*sample);
        if (lifecycle != m_lifecycleGeneration ||
            !currentCompetitionRound(roundId, launchAttemptId)) {
            return;
        }
        if (m_nextTelemetrySequence == (std::numeric_limits<quint32>::max)()) {
            stopTelemetrySampling();
            return;
        }
        ++m_nextTelemetrySequence;
    }

    const auto now = m_scheduler->monotonicNowMs();
    if (m_nextTelemetryDueMs > now) {
        m_nextTelemetryDueMs += IntervalMs;
    } else {
        const auto elapsed = now - m_nextTelemetryDueMs;
        const auto quanta = elapsed / IntervalMs + 1;
        if (quanta >
            ((std::numeric_limits<qint64>::max)() - m_nextTelemetryDueMs) /
              IntervalMs) {
            stopTelemetrySampling();
            return;
        }
        m_nextTelemetryDueMs += quanta * IntervalMs;
    }
    scheduleTelemetryTick();
}

void
ArenaSession::sendOrQueueTelemetry(TelemetrySnapshot telemetry)
{
    if (!m_round) {
        return;
    }
    m_pendingTelemetry = RoundTelemetry{
        .roomId = m_roomId,
        .roomGeneration = m_roomGeneration,
        .connectionGeneration = m_connectionGeneration,
        .roundId = m_round->roundId,
        .launchAttemptId = m_round->launchAttemptId,
        .telemetry = std::move(telemetry),
    };
    if (m_state == State::InRoom && m_protocolReady &&
        sendMessage(*m_pendingTelemetry)) {
        m_pendingTelemetry.reset();
    }
}

void
ArenaSession::flushCompetitionMessages()
{
    if (m_state != State::InRoom || !m_protocolReady || !m_round ||
        m_round->stage != FrozenRoundStage::Playing) {
        return;
    }
    if (m_pendingTerminal) {
        std::visit(
          [&](auto& message) {
              message.connectionGeneration = m_connectionGeneration;
          },
          m_pendingTerminal->message);
        const auto outbound = m_pendingTerminal->message;
        std::visit([this](const auto& message) { (void)sendMessage(message); },
                   outbound);
    }
    if (m_pendingTelemetry && m_pendingTelemetry->roundId == m_round->roundId &&
        m_pendingTelemetry->launchAttemptId == m_round->launchAttemptId) {
        m_pendingTelemetry->connectionGeneration = m_connectionGeneration;
        if (sendMessage(*m_pendingTelemetry)) {
            m_pendingTelemetry.reset();
        }
    }
}

void
ArenaSession::sendTerminal(
  std::variant<RoundResultSubmit, RoundAbandon> message)
{
    if (m_pendingTerminal || m_localTerminalSubmitted) {
        return;
    }
    m_localTerminalSubmitted = true;
    m_pendingTerminal = PendingTerminal{ .message = std::move(message) };
    if (m_state == State::InRoom && m_protocolReady && m_round &&
        m_round->stage == FrozenRoundStage::Playing) {
        const auto outbound = m_pendingTerminal->message;
        std::visit([this](const auto& value) { (void)sendMessage(value); },
                   outbound);
    }
}

void
ArenaSession::showPendingResult(bool localDnf)
{
    if (!m_round) {
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto roundId = m_round->roundId;
    const auto launchAttemptId = m_round->launchAttemptId;
    const auto participantCount =
      static_cast<int>(m_round->participants.size());
    (void)m_presentedResult.setPending(roundId,
                                       participantCount,
                                       m_round->selection.title,
                                       m_arenaOptionsSummary,
                                       localDnf);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return;
    }
    if (!m_resultPresentationActive) {
        m_resultPresentationActive = true;
        emit competitionChanged();
    }
}

void
ArenaSession::sendAbandon(DnfReason reason)
{
    if (!m_round || m_pendingTerminal || m_localTerminalSubmitted) {
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto roomId = m_roomId;
    const auto roomGeneration = m_roomGeneration;
    const auto connectionGeneration = m_connectionGeneration;
    const auto roundId = m_round->roundId;
    const auto launchAttemptId = m_round->launchAttemptId;
    m_localRoundAbandoned = true;
    sendTerminal(RoundAbandon{
      .requestId = nextRequestId(),
      .roomId = roomId,
      .roomGeneration = roomGeneration,
      .connectionGeneration = connectionGeneration,
      .roundId = roundId,
      .launchAttemptId = launchAttemptId,
      .reason = reason,
    });
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return;
    }
    showPendingResult(true);
}

auto
ArenaSession::currentCompetitionRound(QStringView roundId,
                                      QStringView launchAttemptId) const -> bool
{
    return m_round && m_round->roundId == roundId &&
           m_round->launchAttemptId == launchAttemptId;
}

void
ArenaSession::handleStandings(const LiveStandingsSnapshot& snapshot)
{
    if (!acceptsRoomEvent(snapshot.roomId, snapshot.roomGeneration) ||
        !currentCompetitionRound(snapshot.roundId, snapshot.launchAttemptId) ||
        (m_liveStandings.roundId() == snapshot.roundId &&
         snapshot.standingsRevision <= m_liveStandings.revision())) {
        return;
    }
    ArenaStandingsModel validatedStandings;
    if (!validatedStandings.replace(snapshot, m_competitionIdentities)) {
        return;
    }
    const auto self = std::ranges::find_if(
      snapshot.entries, [this](const LiveStandingEntry& entry) {
          return entry.memberId == m_selfMemberId;
      });
    if (self != snapshot.entries.cend() && standingIsTerminal(*self)) {
        stopTelemetrySampling();
        m_pendingTelemetry.reset();
        m_pendingTerminal.reset();
        m_localTerminalSubmitted = true;
        m_localRoundAbandoned =
          std::holds_alternative<LiveDnfStanding>(self->state);
    }
    const auto lifecycle = m_lifecycleGeneration;
    if (!m_liveStandings.replace(snapshot, m_competitionIdentities)) {
        return;
    }
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(snapshot.roundId, snapshot.launchAttemptId)) {
        return;
    }
    m_opponentTarget.update(snapshot, m_selfMemberId, m_competitionIdentities);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(snapshot.roundId, snapshot.launchAttemptId)) {
        return;
    }
}

void
ArenaSession::handleTerminalAccepted(const RoundTerminalAccepted& accepted)
{
    if (!acceptsRoomEvent(accepted.roomId, accepted.roomGeneration) ||
        !currentCompetitionRound(accepted.roundId, accepted.launchAttemptId) ||
        !m_pendingTerminal ||
        terminalRequestId(m_pendingTerminal->message) != accepted.requestId ||
        terminalKind(m_pendingTerminal->message) != accepted.terminal) {
        return;
    }
    m_pendingTerminal.reset();
}

void
ArenaSession::handleRoundFinalized(const RoundFinalized& finalized)
{
    if (!acceptsRoomEvent(finalized.roomId, finalized.roomGeneration) ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId) ||
        finalized.result.roundId != finalized.roundId) {
        return;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto selfMemberId = m_selfMemberId;
    const auto resultSelf = std::ranges::find_if(
      finalized.result.entries, [&](const FinalStandingEntry& entry) {
          return entry.memberId == selfMemberId;
      });
    if (resultSelf != finalized.result.entries.cend()) {
        stopTelemetrySampling();
        m_pendingTelemetry.reset();
        m_pendingTerminal.reset();
        m_localTerminalSubmitted = true;
        m_localRoundAbandoned =
          std::holds_alternative<FinalDnfStanding>(resultSelf->state);
    }
    const auto membersReplaced =
      m_members.replace(finalized.members, m_ownerMemberId, selfMemberId);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId)) {
        return;
    }
    if (!membersReplaced) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    const auto self =
      std::ranges::find_if(finalized.members, [&](const Member& member) {
          return member.memberId == selfMemberId;
      });
    if (self == finalized.members.cend()) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    applySelfMember(*self);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId)) {
        return;
    }
    const auto summary = optionsSummary(finalized.result.selection);
    const auto standingsReplaced =
      m_liveStandings.replaceFinal(finalized.result);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId)) {
        return;
    }
    if (!standingsReplaced) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    if (m_lastResult.valid() &&
        m_lastResult.roundId() != finalized.result.roundId) {
        m_lastResult.clear();
        if (lifecycle != m_lifecycleGeneration ||
            !currentCompetitionRound(finalized.roundId,
                                     finalized.launchAttemptId)) {
            return;
        }
    }
    const auto lastResultReplaced =
      m_lastResult.replaceFinal(finalized.result, selfMemberId, summary);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId)) {
        return;
    }
    if (!lastResultReplaced) {
        failProtocol(ProtocolFailureCode::MalformedMessage);
        return;
    }
    if (m_presentedResult.valid() &&
        m_presentedResult.roundId() == finalized.result.roundId) {
        (void)m_presentedResult.replaceFinal(
          finalized.result, selfMemberId, summary);
        if (lifecycle != m_lifecycleGeneration ||
            !currentCompetitionRound(finalized.roundId,
                                     finalized.launchAttemptId)) {
            return;
        }
    }
    m_pendingTerminal.reset();
    m_pendingTelemetry.reset();
    m_opponentTarget.clear();
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(finalized.roundId,
                                 finalized.launchAttemptId)) {
        return;
    }
    m_competitionIdentities.clear();
    m_roomPhase = RoomPhase::Selecting;
    m_round.reset();
    m_localRoundAbandoned = false;
    m_localTerminalSubmitted = false;
    retainPreparedGameplayUntilReleased();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    emit roundChanged();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    emit selectionChanged();
    if (lifecycle != m_lifecycleGeneration) {
        return;
    }
    emit readyChanged();
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
ArenaSession::selectChart(gameplay_logic::ChartData* chart)
{
    clearError();
    if (!getCanSelect() || m_roundLoader == nullptr || chart == nullptr ||
        !m_selectionRequestId.isEmpty()) {
        return;
    }
    auto built = m_roundLoader->buildSelection(chart);
    if (!built) {
        const auto unsupported =
          built.error() == ArenaSelectionBuildFailure::UnsupportedConfig;
        setError(unsupported ? QStringLiteral("unsupported_config")
                             : QStringLiteral("selection_invalid"),
                 unsupported ? QStringLiteral("arena.error.unsupportedConfig")
                             : QStringLiteral("arena.error.selectionInvalid"));
        return;
    }
    auto selection = std::move(*built);
    if (m_availability.availability(selection.sha256) !=
        ArenaAvailabilityIndex::Availability::AvailableToAll) {
        setError(QStringLiteral("selection_not_common"),
                 QStringLiteral("arena.error.notCommon"));
        return;
    }
    const auto requestId = nextRequestId();
    if (sendMessage(SelectionSet{
          .requestId = requestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .availabilityRevision = m_roomAvailabilityRevision,
          .inventoryRevision = m_selfInventoryRevision,
          .selection = selection,
        })) {
        m_selectionRequestId = requestId;
        m_requestedSelection = std::move(selection);
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::Selection,
                          .lifecycleGeneration = m_lifecycleGeneration });
    }
}

void
ArenaSession::setReady(bool ready)
{
    clearError();
    if (!m_readyRequestId.isEmpty() || (ready && !getCanReady()) ||
        (!ready && (!m_selfReady || m_state != State::InRoom ||
                    m_roomPhase != RoomPhase::Selecting))) {
        return;
    }
    const auto requestId = nextRequestId();
    if (sendMessage(ReadySet{
          .requestId = requestId,
          .roomId = m_roomId,
          .roomGeneration = m_roomGeneration,
          .connectionGeneration = m_connectionGeneration,
          .ready = ready,
          .selectionRevision = m_selectionRevision,
          .availabilityRevision = m_roomAvailabilityRevision,
          .inventoryRevision = m_selfInventoryRevision,
        })) {
        m_pendingCommands.insert(
          requestId,
          PendingCommand{ .kind = PendingCommandKind::Ready,
                          .lifecycleGeneration = m_lifecycleGeneration });
        m_readyRequestId = requestId;
        m_requestedReady = ready;
    }
}

auto
ArenaSession::submitLocalResult(gameplay_logic::BmsScore* score) -> bool
{
    if (!m_round || score == nullptr || score->getResult() == nullptr ||
        m_expectedScoreGuid.isEmpty() ||
        score->getResult()->getGuid() != m_expectedScoreGuid) {
        return false;
    }
    if (m_localRoundAbandoned) {
        const auto lifecycle = m_lifecycleGeneration;
        const auto roundId = m_round->roundId;
        const auto launchAttemptId = m_round->launchAttemptId;
        detachGameplaySource(true);
        if (lifecycle != m_lifecycleGeneration ||
            !currentCompetitionRound(roundId, launchAttemptId)) {
            return true;
        }
        showPendingResult(true);
        return true;
    }
    if (m_localTerminalSubmitted) {
        return true;
    }
    if (m_pendingTerminal) {
        return std::holds_alternative<RoundResultSubmit>(
          m_pendingTerminal->message);
    }
    if (m_gameplaySource == nullptr || !m_gameplaySourceAttached ||
        m_arenaRunner == nullptr ||
        m_arenaRunner->getStatus() != gameplay_logic::ChartRunner::Finished) {
        return false;
    }
    const auto lifecycle = m_lifecycleGeneration;
    const auto roomId = m_roomId;
    const auto roomGeneration = m_roomGeneration;
    const auto connectionGeneration = m_connectionGeneration;
    const auto roundId = m_round->roundId;
    const auto launchAttemptId = m_round->launchAttemptId;
    stopTelemetrySampling();
    setGameplayChatOpen(false);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return true;
    }
    const auto result = m_gameplaySource->captureFinal(score);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return true;
    }
    if (!result) {
        m_localRoundAbandoned = true;
        sendAbandon(DnfReason::ResultUnavailable);
        if (lifecycle != m_lifecycleGeneration ||
            !currentCompetitionRound(roundId, launchAttemptId)) {
            return true;
        }
        detachGameplaySource(true);
        return true;
    }
    sendTerminal(RoundResultSubmit{
      .requestId = nextRequestId(),
      .roomId = roomId,
      .roomGeneration = roomGeneration,
      .connectionGeneration = connectionGeneration,
      .roundId = roundId,
      .launchAttemptId = launchAttemptId,
      .result = *result,
    });
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return true;
    }
    detachGameplaySource(true);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return true;
    }
    showPendingResult(false);
    if (lifecycle != m_lifecycleGeneration ||
        !currentCompetitionRound(roundId, launchAttemptId)) {
        return true;
    }
    return true;
}

void
ArenaSession::releasePreparedGameplay(gameplay_logic::ChartRunner* runner)
{
    if (runner == nullptr) {
        return;
    }
    if (m_retainedGameplayRunner == runner) {
        m_retainedGameplayRunner = nullptr;
        const auto requestId =
          std::exchange(m_retainedGameplayLoadRequestId, quint64{});
        if (requestId != 0 && m_roundLoader != nullptr) {
            m_roundLoader->cancel(requestId);
        }
        return;
    }
    if (m_preparedRunner != runner) {
        return;
    }

    // The gameplay item is already being removed, so no close notification is
    // necessary. The loader uses deleteLater(), keeping the runner valid for
    // the remainder of QML component destruction.
    m_preparedGameplayExposed = false;
    cancelPreparedRound(false);
}

void
ArenaSession::abandonCurrentRound()
{
    if (!m_round ||
        !frozenInventoryRevision(*m_round, m_selfMemberId).has_value() ||
        m_localTerminalSubmitted) {
        return;
    }
    setOverlayCustomizationActive(false);
    m_localRoundAbandoned = true;
    stopTelemetrySampling();
    setGameplayChatOpen(false);
    sendAbandon(DnfReason::Aborted);
    cancelPreparedRound(false, true);
}

void
ArenaSession::setGameplayChatOpen(bool open)
{
    if (open) {
        setOverlayCustomizationActive(false);
    }
    const auto accepted =
      open && ((m_arenaGameplayActive && m_round) ||
               (m_resultPresentationActive && m_presentedResult.valid()));
    if (m_gameplayChatOpen == accepted) {
        return;
    }
    m_gameplayChatOpen = accepted;
    emit gameplayChatOpenChanged();
}

void
ArenaSession::toggleGameplayChat()
{
    setGameplayChatOpen(!m_gameplayChatOpen);
}

void
ArenaSession::releaseCustomizationRunner()
{
    QObject::disconnect(m_customizationRunnerDestroyedConnection);
    m_customizationRunnerDestroyedConnection = {};
    if (m_customizationRunner != nullptr) {
        m_customizationRunner->setInputSuppressed(false);
    }
    m_customizationRunner = nullptr;
}

void
ArenaSession::synchronizeCustomizationRunner()
{
    auto* desired =
      m_overlayCustomizationActive ? m_arenaRunner.data() : nullptr;
    if (m_customizationRunner == desired) {
        return;
    }
    releaseCustomizationRunner();
    if (desired == nullptr) {
        return;
    }
    m_customizationRunner = desired;
    desired->setInputSuppressed(true);
    m_customizationRunnerDestroyedConnection =
      connect(desired, &QObject::destroyed, this, [this] {
          m_customizationRunner = nullptr;
          m_customizationRunnerDestroyedConnection = {};
          if (!m_overlayCustomizationActive) {
              return;
          }
          m_overlayCustomizationActive = false;
          stopTelemetrySampling();
          setGameplayChatOpen(false);
          if (m_gameplaySourceAttached && m_gameplaySource != nullptr) {
              m_gameplaySource->detach();
          }
          m_gameplaySourceAttached = false;
          m_arenaRunnerStatusConnection = {};
          m_arenaGameplayActive = false;
          m_expectedScoreGuid.clear();
          emit overlayCustomizationActiveChanged();
          emit competitionChanged();
      });
}

void
ArenaSession::setOverlayCustomizationActive(bool active)
{
    const auto accepted =
      active && m_arenaGameplayActive && m_arenaRunner != nullptr;
    if (active && !accepted) {
        return;
    }
    if (m_overlayCustomizationActive == accepted) {
        synchronizeCustomizationRunner();
        return;
    }
    if (!accepted) {
        releaseCustomizationRunner();
    }
    m_overlayCustomizationActive = accepted;
    if (accepted) {
        synchronizeCustomizationRunner();
    }
    emit overlayCustomizationActiveChanged();
}

void
ArenaSession::endResultPresentation(const QString& roundId)
{
    if (!m_resultPresentationActive || !m_presentedResult.valid() ||
        m_presentedResult.roundId() != roundId) {
        return;
    }
    setGameplayChatOpen(false);
    m_resultPresentationActive = false;
    m_presentedResult.clear();
    emit competitionChanged();
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
    setRoundLaunchCancellationStatusKey({});
    cancelReconnectTasks();
    m_currentTicketRequestId = 0;
    m_pendingTicket.clear();
    m_pendingCommands.clear();
    m_pendingChatCommandIds.clear();
    const auto preservePreparedRound =
      m_preparedRunner != nullptr && m_round && m_loadRequestRound &&
      sameFrozenRound(*m_round, *m_loadRequestRound);
    const auto preserveCompetitionScore =
      m_round.has_value() && !m_expectedScoreGuid.isEmpty();
    clearRoundTransfers(false, preservePreparedRound, preserveCompetitionScore);
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
    if (m_state == State::InRoom) {
        clearError();
        requestInventorySnapshot();
        if (m_availability.state() == ArenaAvailabilityIndex::State::Syncing) {
            requestAvailabilityResync();
        }
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
    m_legacyFallbackAvailable = false;
    m_legacyBrowseOnly = false;
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
