#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>
#include <QtTypes>

#include <optional>
#include <variant>

namespace arena {

inline constexpr int ProtocolMajor = 1;
inline constexpr int ProtocolMinor = 0;
inline constexpr auto RoomsCapability = "rooms-v1";
inline constexpr auto RoundsCapability = "rounds-v1";
inline constexpr auto CompetitionCapability = "competition-v1";
inline constexpr auto RequiredCapability = RoomsCapability;

inline constexpr qsizetype MaxClientMessageBytes = 64 * 1024;
inline constexpr qsizetype MaxServerMessageBytes = 4 * 1024 * 1024;
inline constexpr qsizetype MaxStandingsMessageBytes = 64 * 1024;
inline constexpr qsizetype MaxResultSnapshotBytes = 256 * 1024;
inline constexpr qsizetype MaxFinalizationMessageBytes = 512 * 1024;
inline constexpr int MaxCapabilities = 16;
inline constexpr int MaxCapabilityCharacters = 64;
inline constexpr int MaxClientVersionCodePoints = 64;
inline constexpr int MaxRoomNameCodePoints = 80;
inline constexpr int MaxPasswordBytes = 128;
inline constexpr int MaxChatCodePoints = 500;
inline constexpr int MaxOpaqueIdCharacters = 128;
inline constexpr int MaxRequestIdCharacters = 64;
inline constexpr int MaxTicketCharacters = 16 * 1024;
inline constexpr int MaxDisplayNameCodePoints = 80;
inline constexpr int MaxAvatarUrlCharacters = 2048;
inline constexpr int RoomCapacity = 32;
inline constexpr int MaxWireChatBacklog = 1000;
inline constexpr int MaxDisplayMessageKeyCharacters = 128;
inline constexpr int MaxSelectionMetadataCodePoints = 200;
inline constexpr int MaxRandomSequenceEntries = 4096;
inline constexpr int MaxInventoryHashes = 250'000;
inline constexpr int MaxInventoryBytes = MaxInventoryHashes * 32;
inline constexpr int InventoryHashesPerChunk = 2047;
inline constexpr int MaxInventoryChunks = 123;
inline constexpr int TransferIdCharacters = 22;
inline constexpr int Sha256Characters = 64;
inline constexpr int Md5Characters = 32;
inline constexpr int LaneSeedCharacters = 16;
inline constexpr int MinRoundStartAfterMs = 250;
inline constexpr int MaxRoundStartAfterMs = 5000;
inline constexpr qint64 MaxUInt32 = 0xffff'ffffLL;
inline constexpr qint64 MaxScoreCounter = 100'000'000;
inline constexpr qint64 MaxChartLengthMs = 21'600'000;
inline constexpr qint64 MaxJsonSafeInteger = 9'007'199'254'740'991LL;

enum class RoomPhase
{
    Selecting,
    Loading,
    Playing,
};

enum class NoteOrder
{
    NormalOrMirror,
    Random,
    SRandom,
    RRandom,
    RandomPlus,
    SRandomPlus,
    BeatorajaRandom,
    BeatorajaRandomEx,
    Lr2Random,
    Lr2RandomEx,
};

enum class DpMode
{
    Off,
    Flip,
    Lr2Flip,
    Battle,
};

enum class GaugeType
{
    Fc,
    ExHard,
    Hard,
    Normal,
    Easy,
    AssistEasy,
};

enum class ClearType
{
    Max,
    Perfect,
    FullCombo,
    ExHard,
    Hard,
    Normal,
    Easy,
    AssistEasy,
    Failed,
};

enum class DnfReason
{
    Aborted,
    ResultUnavailable,
    Left,
    Kicked,
    GraceExpired,
    PlayDeadline,
};

enum class ActiveCompetitionState
{
    Loading,
    Playing,
};

enum class TerminalKind
{
    Finished,
    Dnf,
};

enum class ResumeFailureCode
{
    RoomResumeFailed,
    CompetitionCapabilityRequired,
};

enum class FrozenRoundStage
{
    Probing,
    Loading,
    Scheduled,
    Playing,
};

enum class AvailabilityTransferMode
{
    Reset,
    Delta,
};

enum class SelectionRejectionReason
{
    NotCommon,
    Stale,
    NotAllowed,
};

enum class RoundProbeFailureReason
{
    MissingFile,
    HashMismatch,
    ReadFailed,
    Cancelled,
};

enum class RoundLoadFailureReason
{
    MissingFile,
    HashMismatch,
    ParseFailed,
    UnsupportedConfig,
    ResourceFailed,
    Cancelled,
};

enum class RoundLaunchCancellationReason
{
    MissingFile,
    HashMismatch,
    ReadFailed,
    ParseFailed,
    UnsupportedConfig,
    ResourceFailed,
    ProbeTimeout,
    LoadTimeout,
    ParticipantLeft,
    ParticipantKicked,
    ChartLengthMismatch,
    ServerShutdown,
    Cancelled,
};
enum class MemberStatus
{
    Connected,
    Reserved
};
enum class MemberLeftReason
{
    Left,
    Kicked,
    GraceExpired
};

enum class InventoryState
{
    Missing,
    Syncing,
    Ready,
};

enum class MemberRoundState
{
    Eligible,
    Waiting,
    Probing,
    Loading,
    Loaded,
    Playing,
};

enum class CommandErrorCode
{
    AuthRequired,
    AlreadyInRoom,
    NotInRoom,
    RoomNotFound,
    RoomPasswordInvalid,
    RoomFull,
    RoomBanned,
    RoomDuplicateIdentity,
    RoomGenerationStale,
    ConnectionGenerationStale,
    PermissionDenied,
    TargetNotFound,
    CannotKickSelf,
    ChatEmpty,
    ChatTooLong,
    RateLimited,
    RoundsCapabilityRequired,
    CompetitionCapabilityRequired,
    InventoryInvalid,
    InventoryStale,
    InventoryCapacityExceeded,
    AvailabilityStale,
    SelectionNotCommon,
    SelectionStale,
    ReadyNotAllowed,
    RoundStale,
    LaunchStageStale,
    ResultInvalid,
    RoundAlreadyTerminal,
    ServerCapacity,
};

enum class FatalErrorCode
{
    MalformedMessage,
    FrameTooLarge,
    UnexpectedBinary,
    HelloRequired,
    HelloRepeated,
    ProtocolIncompatible,
    CapabilityRequired,
    InvalidTicket,
    TicketReplayed,
    MalformedInventory,
};

struct SelectionSnapshot
{
    QString sha256;
    std::optional<QString> md5{ std::nullopt };
    QString title;
    QString subtitle;
    QString artist;
    int keyMode{};
    QVector<qint64> randomSequence;
    NoteOrder noteOrderP1{ NoteOrder::NormalOrMirror };
    NoteOrder noteOrderP2{ NoteOrder::NormalOrMirror };
    DpMode dpMode{ DpMode::Off };
    QString laneSeed;
    int randomizationVersion{ 1 };
    bool operator==(const SelectionSnapshot&) const = default;
};

struct PublicIdentity
{
    QString userId;
    QString displayName;
    std::optional<QString> avatarUrl{ std::nullopt };
    bool operator==(const PublicIdentity&) const = default;
};

struct FrozenParticipant
{
    QString memberId;
    qint64 inventoryRevision{};
    std::optional<PublicIdentity> identity{ std::nullopt };
    bool operator==(const FrozenParticipant&) const = default;
};

struct FrozenRound
{
    QString roundId;
    QString launchAttemptId;
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    SelectionSnapshot selection;
    QVector<FrozenParticipant> participants;
    FrozenRoundStage stage{ FrozenRoundStage::Probing };
    std::optional<qint64> playDeadlineAtServerMs{ std::nullopt };
    bool operator==(const FrozenRound&) const = default;
};

struct ArenaJudgements
{
    qint64 perfect{};
    qint64 great{};
    qint64 good{};
    qint64 bad{};
    qint64 poor{};
    qint64 emptyPoor{};
    bool operator==(const ArenaJudgements&) const = default;
};

struct GaugeSnapshot
{
    GaugeType type{ GaugeType::Normal };
    qint64 valueMilli{};
    bool operator==(const GaugeSnapshot&) const = default;
};

struct TelemetrySnapshot
{
    qint64 sequence{};
    qint64 exScore{};
    qint64 progressPermille{};
    qint64 maxCombo{};
    qint64 badPoorCount{};
    ArenaJudgements judgements;
    GaugeSnapshot gauge;
    bool operator==(const TelemetrySnapshot&) const = default;
};

struct FinalResult
{
    qint64 exScore{};
    qint64 maxCombo{};
    qint64 badPoorCount{};
    ArenaJudgements judgements;
    ClearType clearType{ ClearType::Failed };
    GaugeSnapshot finalGauge;
    bool operator==(const FinalResult&) const = default;
};

struct LiveActiveStanding
{
    ActiveCompetitionState competitionState{ ActiveCompetitionState::Loading };
    std::optional<int> rank{ std::nullopt };
    std::optional<TelemetrySnapshot> telemetry{ std::nullopt };
    bool operator==(const LiveActiveStanding&) const = default;
};

struct LiveFinishedStanding
{
    int rank{};
    FinalResult result;
    bool operator==(const LiveFinishedStanding&) const = default;
};

struct LiveDnfStanding
{
    DnfReason reason{ DnfReason::Aborted };
    bool operator==(const LiveDnfStanding&) const = default;
};

using LiveStandingState =
  std::variant<LiveActiveStanding, LiveFinishedStanding, LiveDnfStanding>;

struct LiveStandingEntry
{
    QString memberId;
    MemberStatus connectionStatus{ MemberStatus::Connected };
    LiveStandingState state;
    bool operator==(const LiveStandingEntry&) const = default;
};

struct LiveStandingsSnapshot
{
    QString roomId;
    qint64 roomGeneration{};
    QString roundId;
    QString launchAttemptId;
    qint64 standingsRevision{};
    QVector<LiveStandingEntry> entries;
    bool operator==(const LiveStandingsSnapshot&) const = default;
};

struct FinalFinishedStanding
{
    int rank{};
    FinalResult result;
    bool operator==(const FinalFinishedStanding&) const = default;
};

struct FinalDnfStanding
{
    DnfReason reason{ DnfReason::Aborted };
    bool operator==(const FinalDnfStanding&) const = default;
};

using FinalStandingState =
  std::variant<FinalFinishedStanding, FinalDnfStanding>;

struct FinalStandingEntry
{
    QString memberId;
    PublicIdentity identity;
    std::optional<qint64> lobbyWinsAfter{ std::nullopt };
    FinalStandingState state;
    bool operator==(const FinalStandingEntry&) const = default;
};

struct RoundResultSnapshot
{
    qint64 resultRevision{};
    QString roundId;
    qint64 selectionRevision{};
    qint64 finalizedAtServerMs{};
    int participantCount{};
    SelectionSnapshot selection;
    QVector<QString> winnerMemberIds;
    QVector<FinalStandingEntry> entries;
    bool operator==(const RoundResultSnapshot&) const = default;
};

struct Member
{
    QString memberId;
    PublicIdentity identity;
    MemberStatus status;
    qint64 lobbyWins{};
    bool ready{};
    InventoryState inventoryState{ InventoryState::Missing };
    qint64 inventoryRevision{};
    qint64 availabilityAppliedRevision{};
    MemberRoundState roundState{ MemberRoundState::Eligible };
    bool operator==(const Member&) const = default;
};

struct ChatMessage
{
    QString messageId;
    QString authorMemberId;
    QString authorDisplayName;
    qint64 sentAtMs{};
    QString text;
    bool operator==(const ChatMessage&) const = default;
};

struct RoomMemberPreview
{
    QString displayName;
    std::optional<QUrl> avatarUrl{ std::nullopt };
    bool connected{};
    bool operator==(const RoomMemberPreview&) const = default;
};

struct RoomSummary
{
    QString roomId;
    QString name;
    RoomPhase phase{ RoomPhase::Selecting };
    bool hasPassword{};
    int connectedCount{};
    int reservedCount{};
    int maxCount{ RoomCapacity };
    QVector<RoomMemberPreview> members;
    bool operator==(const RoomSummary&) const = default;
};

struct SelfSeat
{
    QString memberId;
    qint64 connectionGeneration{};
    QString resumeToken;
    bool operator==(const SelfSeat&) const = default;
};

struct RoomSnapshot
{
    QString roomId;
    qint64 roomGeneration{};
    QString name;
    RoomPhase phase{ RoomPhase::Selecting };
    bool hasPassword{};
    int maxCount{ RoomCapacity };
    std::optional<QString> ownerMemberId{ std::nullopt };
    SelfSeat self;
    QVector<Member> members;
    QVector<ChatMessage> chat;
    std::optional<SelectionSnapshot> selection{ std::nullopt };
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    std::optional<FrozenRound> round{ std::nullopt };
    std::optional<LiveStandingsSnapshot> liveStandings{ std::nullopt };
    std::optional<RoundResultSnapshot> lastRoundResult{ std::nullopt };
    bool competitionShape{};
    bool operator==(const RoomSnapshot&) const = default;
};

struct ResumeRequest
{
    QString roomId;
    QString seatToken;
    bool operator==(const ResumeRequest&) const = default;
};

struct ClientHello
{
    int protocolMajor{ ProtocolMajor };
    int protocolMinor{ ProtocolMinor };
    QString clientVersion;
    QStringList capabilities{ QString::fromLatin1(RoomsCapability),
                              QString::fromLatin1(RoundsCapability),
                              QString::fromLatin1(CompetitionCapability) };
    std::optional<QString> ticket{ std::nullopt };
    std::optional<ResumeRequest> resume{ std::nullopt };
    bool operator==(const ClientHello&) const = default;
};

struct DirectorySubscribe
{
    bool operator==(const DirectorySubscribe&) const = default;
};

struct RoomCreate
{
    QString requestId;
    QString name;
    std::optional<QString> password{ std::nullopt };
    bool operator==(const RoomCreate&) const = default;
};

struct RoomJoin
{
    QString requestId;
    QString roomId;
    std::optional<QString> password{ std::nullopt };
    bool operator==(const RoomJoin&) const = default;
};

struct RoomLeave
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    bool operator==(const RoomLeave&) const = default;
};

struct RoomKick
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString targetMemberId;
    bool operator==(const RoomKick&) const = default;
};

struct ChatSend
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString text;
    bool operator==(const ChatSend&) const = default;
};

struct HeartbeatReply
{
    QString nonce;
    bool operator==(const HeartbeatReply&) const = default;
};

struct InventoryUploadBegin
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    qint64 libraryGeneration{};
    qint64 hashCount{};
    qint64 byteCount{};
    qint64 chunkCount{};
    QString vectorDigest;
    bool operator==(const InventoryUploadBegin&) const = default;
};

struct InventoryUploadCommit
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString uploadId;
    qint64 libraryGeneration{};
    qint64 hashCount{};
    qint64 byteCount{};
    qint64 chunkCount{};
    QString vectorDigest;
    bool operator==(const InventoryUploadCommit&) const = default;
};

struct InventoryUploadAbort
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString uploadId;
    qint64 libraryGeneration{};
    bool operator==(const InventoryUploadAbort&) const = default;
};

struct AvailabilityApplied
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    qint64 availabilityRevision{};
    bool operator==(const AvailabilityApplied&) const = default;
};

struct AvailabilityResync
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    qint64 currentRevision{};
    bool operator==(const AvailabilityResync&) const = default;
};

struct SelectionSet
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    qint64 availabilityRevision{};
    qint64 inventoryRevision{};
    SelectionSnapshot selection;
    bool operator==(const SelectionSet&) const = default;
};

struct ReadySet
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    bool ready{};
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    qint64 inventoryRevision{};
    bool operator==(const ReadySet&) const = default;
};

struct RoundProbeResult
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    qint64 inventoryRevision{};
    QString nonce;
    bool ok{};
    std::optional<QString> sha256{ std::nullopt };
    std::optional<RoundProbeFailureReason> failureReason{ std::nullopt };
    bool operator==(const RoundProbeResult&) const = default;
};

struct RoundLoadResult
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    qint64 inventoryRevision{};
    bool ok{};
    std::optional<RoundLoadFailureReason> failureReason{ std::nullopt };
    std::optional<qint64> chartLengthMs{ std::nullopt };
    bool operator==(const RoundLoadResult&) const = default;
};

struct RoundTelemetry
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    TelemetrySnapshot telemetry;
    bool operator==(const RoundTelemetry&) const = default;
};

struct RoundResultSubmit
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    FinalResult result;
    bool operator==(const RoundResultSubmit&) const = default;
};

struct RoundAbandon
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    DnfReason reason{ DnfReason::Aborted };
    bool operator==(const RoundAbandon&) const = default;
};

using ClientMessage = std::variant<ClientHello,
                                   DirectorySubscribe,
                                   RoomCreate,
                                   RoomJoin,
                                   RoomLeave,
                                   RoomKick,
                                   ChatSend,
                                   HeartbeatReply,
                                   InventoryUploadBegin,
                                   InventoryUploadCommit,
                                   InventoryUploadAbort,
                                   AvailabilityApplied,
                                   AvailabilityResync,
                                   SelectionSet,
                                   ReadySet,
                                   RoundProbeResult,
                                   RoundLoadResult,
                                   RoundTelemetry,
                                   RoundResultSubmit,
                                   RoundAbandon>;

struct ResumeNotRequested
{
    bool operator==(const ResumeNotRequested&) const = default;
};

struct ResumeSucceeded
{
    RoomSnapshot room;
    bool operator==(const ResumeSucceeded&) const = default;
};

struct ResumeFailed
{
    ResumeFailureCode code{ ResumeFailureCode::RoomResumeFailed };
    bool operator==(const ResumeFailed&) const = default;
};

using ResumeResult =
  std::variant<ResumeNotRequested, ResumeSucceeded, ResumeFailed>;

struct ServerHello
{
    int protocolMajor{};
    int protocolMinor{};
    QStringList capabilities;
    std::optional<PublicIdentity> identity{ std::nullopt };
    ResumeResult resume;
    bool operator==(const ServerHello&) const = default;
};

struct FatalError
{
    FatalErrorCode code;
    QString displayMessageKey;
    bool operator==(const FatalError&) const = default;
};

struct DirectorySnapshot
{
    qint64 revision{};
    QVector<RoomSummary> rooms;
    bool operator==(const DirectorySnapshot&) const = default;
};

struct RoomDirectoryUpdated
{
    qint64 revision{};
    QVector<RoomSummary> upserts;
    QVector<QString> removedRoomIds;
    bool operator==(const RoomDirectoryUpdated&) const = default;
};

struct RoomSnapshotEvent
{
    QString requestId;
    RoomSnapshot room;
    bool operator==(const RoomSnapshotEvent&) const = default;
};

struct RoomMemberJoined
{
    QString roomId;
    qint64 roomGeneration{};
    Member member;
    bool operator==(const RoomMemberJoined&) const = default;
};

struct RoomMemberUpdated
{
    QString roomId;
    qint64 roomGeneration{};
    Member member;
    bool operator==(const RoomMemberUpdated&) const = default;
};

struct RoomMemberLeft
{
    QString roomId;
    qint64 roomGeneration{};
    QString memberId;
    MemberLeftReason reason;
    bool operator==(const RoomMemberLeft&) const = default;
};

struct RoomOwnerChanged
{
    QString roomId;
    qint64 roomGeneration{};
    std::optional<QString> ownerMemberId{ std::nullopt };
    bool operator==(const RoomOwnerChanged&) const = default;
};

struct ChatMessageEvent
{
    QString roomId;
    qint64 roomGeneration{};
    ChatMessage message;
    bool operator==(const ChatMessageEvent&) const = default;
};

struct ServerHeartbeat
{
    QString nonce;
    qint64 sentAtMs{};
    bool operator==(const ServerHeartbeat&) const = default;
};

struct ServerGoingAway
{
    std::optional<qint64> retryAfterMs{ std::nullopt };
    bool operator==(const ServerGoingAway&) const = default;
};

struct CommandError
{
    QString requestId;
    CommandErrorCode code;
    QString displayMessageKey;
    bool operator==(const CommandError&) const = default;
};

struct InventoryUploadReady
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString uploadId;
    qint64 libraryGeneration{};
    qint64 hashCount{};
    qint64 byteCount{};
    qint64 chunkCount{};
    QString vectorDigest;
    qint64 deadlineMs{};
    bool operator==(const InventoryUploadReady&) const = default;
};

struct InventoryCommitted
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    qint64 libraryGeneration{};
    qint64 inventoryRevision{};
    InventoryState inventoryState{ InventoryState::Ready };
    bool operator==(const InventoryCommitted&) const = default;
};

struct AvailabilityTransferBegin
{
    QString roomId;
    qint64 roomGeneration{};
    QString transferId;
    AvailabilityTransferMode mode{ AvailabilityTransferMode::Reset };
    qint64 targetRevision{};
    QVector<FrozenParticipant> basis;

    qint64 resetCount{};
    qint64 resetChunkCount{};
    QString resetDigest;

    qint64 baseRevision{};
    qint64 addedCount{};
    qint64 addedChunkCount{};
    QString addedDigest;
    qint64 removedCount{};
    qint64 removedChunkCount{};
    QString removedDigest;
    bool operator==(const AvailabilityTransferBegin&) const = default;
};

struct AvailabilityTransferCommit
{
    QString roomId;
    qint64 roomGeneration{};
    QString transferId;
    qint64 targetRevision{};
    bool operator==(const AvailabilityTransferCommit&) const = default;
};

struct SelectionChanged
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    std::optional<SelectionSnapshot> selection{ std::nullopt };
    std::optional<QString> selectedByMemberId{ std::nullopt };
    bool operator==(const SelectionChanged&) const = default;
};

struct SelectionRejected
{
    QString requestId;
    SelectionRejectionReason reason{ SelectionRejectionReason::Stale };
    QVector<QString> missingMemberIds;
    bool operator==(const SelectionRejected&) const = default;
};

struct RoundLoadingStarted
{
    QString roomId;
    qint64 roomGeneration{};
    FrozenRound round;
    bool operator==(const RoundLoadingStarted&) const = default;
};

struct RoundProbeRequested
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    qint64 inventoryRevision{};
    QString nonce;
    QString sha256;
    qint64 deadlineMs{};
    bool operator==(const RoundProbeRequested&) const = default;
};

struct RoundLoadRequested
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    FrozenRound round;
    bool operator==(const RoundLoadRequested&) const = default;
};

struct RoundStartScheduled
{
    QString roomId;
    qint64 roomGeneration{};
    qint64 connectionGeneration{};
    QString roundId;
    QString launchAttemptId;
    qint64 startAtServerMs{};
    qint64 startAfterMs{};
    std::optional<qint64> playDeadlineAtServerMs{ std::nullopt };
    bool operator==(const RoundStartScheduled&) const = default;
};

struct RoundStarted
{
    QString roomId;
    qint64 roomGeneration{};
    QString roundId;
    QString launchAttemptId;
    std::optional<qint64> playDeadlineAtServerMs{ std::nullopt };
    bool operator==(const RoundStarted&) const = default;
};

struct RoundTerminalAccepted
{
    QString requestId;
    QString roomId;
    qint64 roomGeneration{};
    QString roundId;
    QString launchAttemptId;
    TerminalKind terminal{ TerminalKind::Finished };
    bool operator==(const RoundTerminalAccepted&) const = default;
};

struct RoundFinalized
{
    QString roomId;
    qint64 roomGeneration{};
    QString roundId;
    QString launchAttemptId;
    RoundResultSnapshot result;
    QVector<Member> members;
    bool operator==(const RoundFinalized&) const = default;
};

struct RoundLaunchCancelled
{
    QString roomId;
    qint64 roomGeneration{};
    QString roundId;
    QString launchAttemptId;
    RoundLaunchCancellationReason reason{
        RoundLaunchCancellationReason::Cancelled
    };
    std::optional<SelectionSnapshot> selection{ std::nullopt };
    qint64 selectionRevision{};
    qint64 availabilityRevision{};
    bool operator==(const RoundLaunchCancelled&) const = default;
};

using ServerMessage = std::variant<ServerHello,
                                   FatalError,
                                   DirectorySnapshot,
                                   RoomDirectoryUpdated,
                                   RoomSnapshotEvent,
                                   RoomMemberJoined,
                                   RoomMemberUpdated,
                                   RoomMemberLeft,
                                   RoomOwnerChanged,
                                   ChatMessageEvent,
                                   ServerHeartbeat,
                                   ServerGoingAway,
                                   CommandError,
                                   InventoryUploadReady,
                                   InventoryCommitted,
                                   AvailabilityTransferBegin,
                                   AvailabilityTransferCommit,
                                   SelectionChanged,
                                   SelectionRejected,
                                   RoundLoadingStarted,
                                   RoundProbeRequested,
                                   RoundLoadRequested,
                                   RoundStartScheduled,
                                   RoundStarted,
                                   LiveStandingsSnapshot,
                                   RoundTerminalAccepted,
                                   RoundFinalized,
                                   RoundLaunchCancelled>;

} // namespace arena
