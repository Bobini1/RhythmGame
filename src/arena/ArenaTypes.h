#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtTypes>

#include <optional>
#include <variant>

namespace arena {

inline constexpr int ProtocolMajor = 1;
inline constexpr int ProtocolMinor = 0;
inline constexpr auto RequiredCapability = "rooms-v1";

inline constexpr qsizetype MaxClientMessageBytes = 64 * 1024;
inline constexpr qsizetype MaxServerMessageBytes = 4 * 1024 * 1024;
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
inline constexpr int RoomCapacity = 16;
inline constexpr int MaxWireChatBacklog = 1000;
inline constexpr int MaxDisplayMessageKeyCharacters = 128;
inline constexpr qint64 MaxJsonSafeInteger = 9'007'199'254'740'991LL;

enum class RoomPhase
{
    Selecting
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
    ServerShuttingDown,
};

struct PublicIdentity
{
    QString userId;
    QString displayName;
    std::optional<QString> avatarUrl{ std::nullopt };
    bool operator==(const PublicIdentity&) const = default;
};

struct Member
{
    QString memberId;
    PublicIdentity identity;
    MemberStatus status;
    qint64 lobbyWins{};
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

struct RoomSummary
{
    QString roomId;
    QString name;
    RoomPhase phase{ RoomPhase::Selecting };
    bool hasPassword{};
    int connectedCount{};
    int reservedCount{};
    int maxCount{ RoomCapacity };
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
    QStringList capabilities{ QString::fromLatin1(RequiredCapability) };
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

using ClientMessage = std::variant<ClientHello,
                                   DirectorySubscribe,
                                   RoomCreate,
                                   RoomJoin,
                                   RoomLeave,
                                   RoomKick,
                                   ChatSend,
                                   HeartbeatReply>;

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
                                   CommandError>;

} // namespace arena
