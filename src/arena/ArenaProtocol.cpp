#include "ArenaProtocol.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <type_traits>
#include <utility>

namespace arena {
namespace {

struct DecodeFailure
{
    ProtocolFailureCode code;
};

[[noreturn]] void
fail(ProtocolFailureCode code = ProtocolFailureCode::MalformedMessage)
{
    throw DecodeFailure{ code };
}

auto
keySet(std::initializer_list<const char*> keys) -> QSet<QString>
{
    QSet<QString> result;
    for (const auto* key : keys) {
        result.insert(QString::fromLatin1(key));
    }
    return result;
}

void
requireExactKeys(const QJsonObject& object,
                 std::initializer_list<const char*> required,
                 std::initializer_list<const char*> optional = {})
{
    const auto requiredKeys = keySet(required);
    auto allowedKeys = requiredKeys;
    allowedKeys.unite(keySet(optional));
    for (const auto& key : requiredKeys) {
        if (!object.contains(key)) {
            fail();
        }
    }
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (!allowedKeys.contains(it.key())) {
            fail();
        }
    }
}

auto
requiredString(const QJsonObject& object, const char* key) -> QString
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isString()) {
        fail();
    }
    return value.toString();
}

auto
requiredObject(const QJsonObject& object, const char* key) -> QJsonObject
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isObject()) {
        fail();
    }
    return value.toObject();
}

auto
requiredArray(const QJsonObject& object, const char* key) -> QJsonArray
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isArray()) {
        fail();
    }
    return value.toArray();
}

auto
requiredBool(const QJsonObject& object, const char* key) -> bool
{
    const auto value = object.value(QString::fromLatin1(key));
    if (!value.isBool()) {
        fail();
    }
    return value.toBool();
}

auto
nullableString(const QJsonObject& object, const char* key)
  -> std::optional<QString>
{
    const auto value = object.value(QString::fromLatin1(key));
    if (value.isNull()) {
        return std::nullopt;
    }
    if (!value.isString()) {
        fail();
    }
    return value.toString();
}

auto
safeInteger(const QJsonValue& value, bool positive) -> qint64
{
    if (!value.isDouble()) {
        fail();
    }
    const auto number = value.toDouble();
    const auto minimum = positive ? 1.0 : 0.0;
    if (!std::isfinite(number) || std::trunc(number) != number ||
        number < minimum || number > static_cast<double>(MaxJsonSafeInteger)) {
        fail();
    }
    return static_cast<qint64>(number);
}

auto
requiredSafeInteger(const QJsonObject& object,
                    const char* key,
                    bool positive = false) -> qint64
{
    return safeInteger(object.value(QString::fromLatin1(key)), positive);
}

auto
unicodeCodePointCount(QStringView value) -> std::optional<int>
{
    int count = 0;
    for (qsizetype i = 0; i < value.size(); ++i) {
        const auto current = value[i];
        if (current.isHighSurrogate()) {
            if (i + 1 >= value.size() || !value[i + 1].isLowSurrogate()) {
                return std::nullopt;
            }
            ++i;
        } else if (current.isLowSurrogate()) {
            return std::nullopt;
        }
        ++count;
    }
    return count;
}

auto
validCodePointString(QStringView value, int maximum) -> bool
{
    const auto count = unicodeCodePointCount(value);
    return count && *count >= 1 && *count <= maximum;
}

auto
isEcmaWhitespace(QChar ch) -> bool
{
    const auto c = ch.unicode();
    return (c >= 0x0009 && c <= 0x000d) || c == 0x0020 || c == 0x00a0 ||
           c == 0x1680 || (c >= 0x2000 && c <= 0x200a) || c == 0x2028 ||
           c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000 ||
           c == 0xfeff;
}

auto
ecmaTrim(QString value) -> QString
{
    qsizetype begin = 0;
    qsizetype end = value.size();
    while (begin < end && isEcmaWhitespace(value[begin])) {
        ++begin;
    }
    while (end > begin && isEcmaWhitespace(value[end - 1])) {
        --end;
    }
    return value.mid(begin, end - begin);
}

void
requireValidUnicode(QStringView value)
{
    if (!unicodeCodePointCount(value)) {
        fail();
    }
}

auto
isSafeIdentifier(QStringView value, int maximum) -> bool
{
    if (value.isEmpty() || value.size() > maximum) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](QChar ch) {
        const auto c = ch.unicode();
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '.' || c == '_' || c == ':' ||
               c == '-';
    });
}

auto
isCapability(QStringView value) -> bool
{
    if (value.isEmpty() || value.size() > MaxCapabilityCharacters) {
        return false;
    }
    for (qsizetype i = 0; i < value.size(); ++i) {
        const auto c = value[i].unicode();
        const auto alphaNumeric = (c >= 'A' && c <= 'Z') ||
                                  (c >= 'a' && c <= 'z') ||
                                  (c >= '0' && c <= '9');
        if (!alphaNumeric && (i == 0 || (c != '.' && c != '_' && c != '-'))) {
            return false;
        }
    }
    return true;
}

void
validateOpaqueId(QStringView id);

auto
parseCapabilities(const QJsonArray& array, bool server) -> QStringList
{
    if (array.isEmpty() || (!server && array.size() > MaxCapabilities)) {
        fail();
    }
    QStringList result;
    QSet<QString> seen;
    for (const auto& value : array) {
        if (!value.isString() || !isCapability(value.toString())) {
            fail();
        }
        const auto capability = value.toString();
        if (seen.contains(capability)) {
            fail();
        }
        seen.insert(capability);
        result.push_back(capability);
    }
    if (!result.contains(QString::fromLatin1(RequiredCapability))) {
        fail(ProtocolFailureCode::CapabilityRequired);
    }
    if (server &&
        result != QStringList{ QString::fromLatin1(RequiredCapability) }) {
        fail();
    }
    return result;
}

auto
parsePublicIdentity(const QJsonObject& object) -> PublicIdentity
{
    requireExactKeys(object, { "userId", "displayName", "avatarUrl" });
    PublicIdentity result;
    result.userId = requiredString(object, "userId");
    result.displayName = requiredString(object, "displayName");
    if (!isSafeIdentifier(result.userId, MaxOpaqueIdCharacters) ||
        !validCodePointString(result.displayName, MaxDisplayNameCodePoints)) {
        fail();
    }
    const auto avatar = object.value(QStringLiteral("avatarUrl"));
    if (avatar.isNull()) {
        result.avatarUrl = std::nullopt;
    } else if (avatar.isString()) {
        const auto value = avatar.toString();
        const QUrl url(value);
        if (value.isEmpty() || value.size() > MaxAvatarUrlCharacters ||
            !url.isValid() || url.isRelative() || url.scheme().isEmpty()) {
            fail();
        }
        result.avatarUrl = value;
    } else {
        fail();
    }
    return result;
}

auto
parseMemberStatus(QStringView value) -> MemberStatus
{
    if (value == QStringLiteral("connected")) {
        return MemberStatus::Connected;
    }
    if (value == QStringLiteral("reserved")) {
        return MemberStatus::Reserved;
    }
    fail();
}

auto
parseMember(const QJsonObject& object) -> Member
{
    requireExactKeys(object, { "memberId", "identity", "status", "lobbyWins" });
    Member result;
    result.memberId = requiredString(object, "memberId");
    validateOpaqueId(result.memberId);
    result.identity = parsePublicIdentity(requiredObject(object, "identity"));
    result.status = parseMemberStatus(requiredString(object, "status"));
    result.lobbyWins = requiredSafeInteger(object, "lobbyWins");
    return result;
}

auto
parseChatMessage(const QJsonObject& object) -> ChatMessage
{
    requireExactKeys(object,
                     { "messageId",
                       "authorMemberId",
                       "authorDisplayName",
                       "sentAtMs",
                       "text" });
    ChatMessage result;
    result.messageId = requiredString(object, "messageId");
    result.authorMemberId = requiredString(object, "authorMemberId");
    result.authorDisplayName = requiredString(object, "authorDisplayName");
    result.sentAtMs = requiredSafeInteger(object, "sentAtMs");
    result.text = requiredString(object, "text");
    validateOpaqueId(result.messageId);
    validateOpaqueId(result.authorMemberId);
    if (!validCodePointString(result.authorDisplayName,
                              MaxDisplayNameCodePoints) ||
        !validCodePointString(result.text, MaxChatCodePoints)) {
        fail();
    }
    return result;
}

auto
parseRoomSummary(const QJsonObject& object) -> RoomSummary
{
    requireExactKeys(object,
                     { "roomId",
                       "name",
                       "phase",
                       "hasPassword",
                       "connectedCount",
                       "reservedCount",
                       "maxCount" });
    RoomSummary result;
    result.roomId = requiredString(object, "roomId");
    result.name = ecmaTrim(requiredString(object, "name"));
    if (!isSafeIdentifier(result.roomId, MaxOpaqueIdCharacters) ||
        !validCodePointString(result.name, MaxRoomNameCodePoints) ||
        requiredString(object, "phase") != QStringLiteral("selecting")) {
        fail();
    }
    result.hasPassword = requiredBool(object, "hasPassword");
    const auto connected = requiredSafeInteger(object, "connectedCount");
    const auto reserved = requiredSafeInteger(object, "reservedCount");
    const auto maximum = requiredSafeInteger(object, "maxCount");
    if (connected > RoomCapacity || reserved > RoomCapacity ||
        maximum != RoomCapacity || connected + reserved > maximum) {
        fail();
    }
    result.connectedCount = static_cast<int>(connected);
    result.reservedCount = static_cast<int>(reserved);
    result.maxCount = static_cast<int>(maximum);
    return result;
}

auto
parseRoomSnapshot(const QJsonObject& object) -> RoomSnapshot
{
    requireExactKeys(object,
                     { "roomId",
                       "roomGeneration",
                       "name",
                       "phase",
                       "hasPassword",
                       "maxCount",
                       "ownerMemberId",
                       "self",
                       "members",
                       "chat" });
    RoomSnapshot result;
    result.roomId = requiredString(object, "roomId");
    validateOpaqueId(result.roomId);
    result.roomGeneration = requiredSafeInteger(object, "roomGeneration", true);
    result.name = ecmaTrim(requiredString(object, "name"));
    if (!validCodePointString(result.name, MaxRoomNameCodePoints) ||
        requiredString(object, "phase") != QStringLiteral("selecting")) {
        fail();
    }
    result.hasPassword = requiredBool(object, "hasPassword");
    const auto maximum = requiredSafeInteger(object, "maxCount");
    if (maximum != RoomCapacity) {
        fail();
    }
    result.maxCount = static_cast<int>(maximum);
    result.ownerMemberId = nullableString(object, "ownerMemberId");
    if (result.ownerMemberId) {
        validateOpaqueId(*result.ownerMemberId);
    }

    const auto self = requiredObject(object, "self");
    requireExactKeys(self,
                     { "memberId", "connectionGeneration", "resumeToken" });
    result.self.memberId = requiredString(self, "memberId");
    result.self.connectionGeneration =
      requiredSafeInteger(self, "connectionGeneration", true);
    result.self.resumeToken = requiredString(self, "resumeToken");
    validateOpaqueId(result.self.memberId);
    validateOpaqueId(result.self.resumeToken);

    const auto members = requiredArray(object, "members");
    if (members.size() > RoomCapacity) {
        fail();
    }
    QSet<QString> memberIds;
    for (const auto& value : members) {
        if (!value.isObject()) {
            fail();
        }
        auto member = parseMember(value.toObject());
        if (memberIds.contains(member.memberId)) {
            fail();
        }
        memberIds.insert(member.memberId);
        result.members.push_back(std::move(member));
    }

    const auto chat = requiredArray(object, "chat");
    if (chat.size() > MaxWireChatBacklog) {
        fail();
    }
    QSet<QString> messageIds;
    for (const auto& value : chat) {
        if (!value.isObject()) {
            fail();
        }
        auto message = parseChatMessage(value.toObject());
        if (messageIds.contains(message.messageId)) {
            fail();
        }
        messageIds.insert(message.messageId);
        result.chat.push_back(std::move(message));
    }
    return result;
}

auto
parseMemberLeftReason(QStringView value) -> MemberLeftReason
{
    if (value == QStringLiteral("left")) {
        return MemberLeftReason::Left;
    }
    if (value == QStringLiteral("kicked")) {
        return MemberLeftReason::Kicked;
    }
    if (value == QStringLiteral("grace_expired")) {
        return MemberLeftReason::GraceExpired;
    }
    fail();
}

auto
parseCommandErrorCode(QStringView value) -> CommandErrorCode
{
    static const std::pair<QStringView, CommandErrorCode> entries[]{
        { u"auth_required", CommandErrorCode::AuthRequired },
        { u"already_in_room", CommandErrorCode::AlreadyInRoom },
        { u"not_in_room", CommandErrorCode::NotInRoom },
        { u"room_not_found", CommandErrorCode::RoomNotFound },
        { u"room_password_invalid", CommandErrorCode::RoomPasswordInvalid },
        { u"room_full", CommandErrorCode::RoomFull },
        { u"room_banned", CommandErrorCode::RoomBanned },
        { u"room_duplicate_identity", CommandErrorCode::RoomDuplicateIdentity },
        { u"room_generation_stale", CommandErrorCode::RoomGenerationStale },
        { u"connection_generation_stale",
          CommandErrorCode::ConnectionGenerationStale },
        { u"permission_denied", CommandErrorCode::PermissionDenied },
        { u"target_not_found", CommandErrorCode::TargetNotFound },
        { u"cannot_kick_self", CommandErrorCode::CannotKickSelf },
        { u"chat_empty", CommandErrorCode::ChatEmpty },
        { u"chat_too_long", CommandErrorCode::ChatTooLong },
        { u"rate_limited", CommandErrorCode::RateLimited },
    };
    for (const auto& [spelling, code] : entries) {
        if (value == spelling) {
            return code;
        }
    }
    fail();
}

auto
parseFatalErrorCode(QStringView value) -> FatalErrorCode
{
    static const std::pair<QStringView, FatalErrorCode> entries[]{
        { u"malformed_message", FatalErrorCode::MalformedMessage },
        { u"frame_too_large", FatalErrorCode::FrameTooLarge },
        { u"unexpected_binary", FatalErrorCode::UnexpectedBinary },
        { u"hello_required", FatalErrorCode::HelloRequired },
        { u"hello_repeated", FatalErrorCode::HelloRepeated },
        { u"protocol_incompatible", FatalErrorCode::ProtocolIncompatible },
        { u"capability_required", FatalErrorCode::CapabilityRequired },
        { u"invalid_ticket", FatalErrorCode::InvalidTicket },
        { u"ticket_replayed", FatalErrorCode::TicketReplayed },
        { u"server_shutting_down", FatalErrorCode::ServerShuttingDown },
    };
    for (const auto& [spelling, code] : entries) {
        if (value == spelling) {
            return code;
        }
    }
    fail();
}

auto
isDisplayMessageKey(QStringView value) -> bool
{
    if (value.size() <= 6 || value.size() > MaxDisplayMessageKeyCharacters ||
        !value.startsWith(QStringLiteral("arena."))) {
        return false;
    }
    return std::all_of(value.begin() + 6, value.end(), [](QChar ch) {
        const auto c = ch.unicode();
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '.';
    });
}

auto
encodeHello(const ClientHello& hello) -> QJsonObject
{
    if (hello.protocolMajor != ProtocolMajor ||
        hello.protocolMinor != ProtocolMinor) {
        fail(ProtocolFailureCode::ProtocolIncompatible);
    }
    if (!validCodePointString(hello.clientVersion,
                              MaxClientVersionCodePoints)) {
        fail();
    }
    QJsonArray capabilities;
    QSet<QString> seen;
    for (const auto& capability : hello.capabilities) {
        if (!isCapability(capability) || seen.contains(capability)) {
            fail();
        }
        seen.insert(capability);
        capabilities.append(capability);
    }
    if (hello.capabilities.isEmpty() ||
        hello.capabilities.size() > MaxCapabilities) {
        fail();
    }
    if (!hello.capabilities.contains(QString::fromLatin1(RequiredCapability))) {
        fail(ProtocolFailureCode::CapabilityRequired);
    }
    if (hello.ticket && (hello.ticket->isEmpty() ||
                         hello.ticket->size() > MaxTicketCharacters)) {
        fail();
    }
    if (hello.ticket) {
        requireValidUnicode(*hello.ticket);
    }
    if (hello.resume && !hello.ticket) {
        fail();
    }
    QJsonObject data{
        { QStringLiteral("protocolMajor"), hello.protocolMajor },
        { QStringLiteral("protocolMinor"), hello.protocolMinor },
        { QStringLiteral("clientVersion"), hello.clientVersion },
        { QStringLiteral("capabilities"), capabilities },
    };
    if (hello.ticket) {
        data.insert(QStringLiteral("ticket"), *hello.ticket);
    }
    if (hello.resume) {
        if (!isSafeIdentifier(hello.resume->roomId, MaxOpaqueIdCharacters) ||
            !isSafeIdentifier(hello.resume->seatToken, MaxOpaqueIdCharacters)) {
            fail();
        }
        data.insert(
          QStringLiteral("resume"),
          QJsonObject{
            { QStringLiteral("roomId"), hello.resume->roomId },
            { QStringLiteral("seatToken"), hello.resume->seatToken } });
    }
    return { { QStringLiteral("type"), QStringLiteral("client_hello") },
             { QStringLiteral("data"), data } };
}

void
validateRequestId(QStringView requestId)
{
    if (!isSafeIdentifier(requestId, MaxRequestIdCharacters)) {
        fail();
    }
}

void
validateOpaqueId(QStringView id)
{
    if (!isSafeIdentifier(id, MaxOpaqueIdCharacters)) {
        fail();
    }
}

void
validatePassword(const std::optional<QString>& password)
{
    if (!password) {
        return;
    }
    requireValidUnicode(*password);
    if (password->isEmpty() || password->toUtf8().size() > MaxPasswordBytes) {
        fail();
    }
}

void
validatePositiveGeneration(qint64 value)
{
    if (value < 1 || value > MaxJsonSafeInteger) {
        fail();
    }
}

auto
requestEnvelope(QString type, QString requestId, QJsonObject data)
  -> QJsonObject
{
    validateRequestId(requestId);
    return { { QStringLiteral("type"), std::move(type) },
             { QStringLiteral("requestId"), std::move(requestId) },
             { QStringLiteral("data"), std::move(data) } };
}

auto
encodeDirectorySubscribe(const DirectorySubscribe&) -> QJsonObject
{
    return { { QStringLiteral("type"), QStringLiteral("directory_subscribe") },
             { QStringLiteral("data"), QJsonObject{} } };
}

auto
encodeRoomCreate(const RoomCreate& command) -> QJsonObject
{
    auto name = ecmaTrim(command.name);
    if (!validCodePointString(name, MaxRoomNameCodePoints)) {
        fail();
    }
    validatePassword(command.password);
    QJsonObject data{ { QStringLiteral("name"), std::move(name) } };
    if (command.password) {
        data.insert(QStringLiteral("password"), *command.password);
    }
    return requestEnvelope(
      QStringLiteral("room_create"), command.requestId, std::move(data));
}

auto
encodeRoomJoin(const RoomJoin& command) -> QJsonObject
{
    validateOpaqueId(command.roomId);
    validatePassword(command.password);
    QJsonObject data{ { QStringLiteral("roomId"), command.roomId } };
    if (command.password) {
        data.insert(QStringLiteral("password"), *command.password);
    }
    return requestEnvelope(
      QStringLiteral("room_join"), command.requestId, std::move(data));
}

auto
generationData(QString roomId,
               qint64 roomGeneration,
               qint64 connectionGeneration) -> QJsonObject
{
    validateOpaqueId(roomId);
    validatePositiveGeneration(roomGeneration);
    validatePositiveGeneration(connectionGeneration);
    return { { QStringLiteral("roomId"), std::move(roomId) },
             { QStringLiteral("roomGeneration"), roomGeneration },
             { QStringLiteral("connectionGeneration"), connectionGeneration } };
}

auto
encodeRoomLeave(const RoomLeave& command) -> QJsonObject
{
    return requestEnvelope(QStringLiteral("room_leave"),
                           command.requestId,
                           generationData(command.roomId,
                                          command.roomGeneration,
                                          command.connectionGeneration));
}

auto
encodeRoomKick(const RoomKick& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    validateOpaqueId(command.targetMemberId);
    data.insert(QStringLiteral("targetMemberId"), command.targetMemberId);
    return requestEnvelope(
      QStringLiteral("room_kick"), command.requestId, std::move(data));
}

auto
encodeChatSend(const ChatSend& command) -> QJsonObject
{
    auto data = generationData(
      command.roomId, command.roomGeneration, command.connectionGeneration);
    auto text = ecmaTrim(command.text);
    if (!validCodePointString(text, MaxChatCodePoints)) {
        fail();
    }
    data.insert(QStringLiteral("text"), std::move(text));
    return requestEnvelope(
      QStringLiteral("chat_send"), command.requestId, std::move(data));
}

auto
encodeHeartbeatReply(const HeartbeatReply& command) -> QJsonObject
{
    validateOpaqueId(command.nonce);
    return { { QStringLiteral("type"), QStringLiteral("heartbeat_reply") },
             { QStringLiteral("data"),
               QJsonObject{ { QStringLiteral("nonce"), command.nonce } } } };
}

auto
parseServerHello(const QJsonObject& data) -> ServerHello
{
    requireExactKeys(
      data,
      { "protocolMajor", "protocolMinor", "capabilities", "resume" },
      { "identity" });
    const auto major = requiredSafeInteger(data, "protocolMajor");
    const auto minor = requiredSafeInteger(data, "protocolMinor");
    if (major != ProtocolMajor || minor != ProtocolMinor) {
        fail(ProtocolFailureCode::ProtocolIncompatible);
    }
    ServerHello result;
    result.protocolMajor = static_cast<int>(major);
    result.protocolMinor = static_cast<int>(minor);
    result.capabilities =
      parseCapabilities(requiredArray(data, "capabilities"), true);
    if (data.contains(QStringLiteral("identity"))) {
        result.identity = parsePublicIdentity(requiredObject(data, "identity"));
    }
    const auto resume = requiredObject(data, "resume");
    const auto status = requiredString(resume, "status");
    if (status == QStringLiteral("not_requested")) {
        requireExactKeys(resume, { "status" });
        result.resume = ResumeNotRequested{};
    } else if (status == QStringLiteral("succeeded")) {
        requireExactKeys(resume, { "status", "room" });
        result.resume =
          ResumeSucceeded{ parseRoomSnapshot(requiredObject(resume, "room")) };
    } else if (status == QStringLiteral("failed")) {
        requireExactKeys(resume, { "status", "code", "displayMessageKey" });
        if (requiredString(resume, "code") !=
              QStringLiteral("room_resume_failed") ||
            requiredString(resume, "displayMessageKey") !=
              QStringLiteral("arena.error.resumeFailed")) {
            fail();
        }
        result.resume = ResumeFailed{};
    } else {
        fail();
    }
    return result;
}

auto
parseDirectorySnapshot(const QJsonObject& data) -> DirectorySnapshot
{
    requireExactKeys(data, { "revision", "rooms" });
    DirectorySnapshot result;
    result.revision = requiredSafeInteger(data, "revision");
    QSet<QString> ids;
    for (const auto& value : requiredArray(data, "rooms")) {
        if (!value.isObject()) {
            fail();
        }
        auto room = parseRoomSummary(value.toObject());
        if (ids.contains(room.roomId)) {
            fail();
        }
        ids.insert(room.roomId);
        result.rooms.push_back(std::move(room));
    }
    return result;
}

auto
parseRoomDirectoryUpdated(const QJsonObject& data) -> RoomDirectoryUpdated
{
    requireExactKeys(data, { "revision", "upserts", "removedRoomIds" });
    RoomDirectoryUpdated result;
    result.revision = requiredSafeInteger(data, "revision");
    QSet<QString> upsertIds;
    for (const auto& value : requiredArray(data, "upserts")) {
        if (!value.isObject()) {
            fail();
        }
        auto room = parseRoomSummary(value.toObject());
        if (upsertIds.contains(room.roomId)) {
            fail();
        }
        upsertIds.insert(room.roomId);
        result.upserts.push_back(std::move(room));
    }
    QSet<QString> removedIds;
    for (const auto& value : requiredArray(data, "removedRoomIds")) {
        if (!value.isString()) {
            fail();
        }
        auto roomId = value.toString();
        validateOpaqueId(roomId);
        if (removedIds.contains(roomId) || upsertIds.contains(roomId)) {
            fail();
        }
        removedIds.insert(roomId);
        result.removedRoomIds.push_back(std::move(roomId));
    }
    return result;
}

auto
parseFatalError(const QJsonObject& data) -> FatalError
{
    requireExactKeys(data, { "code", "displayMessageKey" });
    const auto displayKey = requiredString(data, "displayMessageKey");
    if (!isDisplayMessageKey(displayKey)) {
        fail();
    }
    return FatalError{ .code =
                         parseFatalErrorCode(requiredString(data, "code")),
                       .displayMessageKey = displayKey };
}

struct RoomEventHeader
{
    QString roomId;
    qint64 roomGeneration{};
};

auto
parseRoomEventHeader(const QJsonObject& data) -> RoomEventHeader
{
    auto roomId = requiredString(data, "roomId");
    validateOpaqueId(roomId);
    return { .roomId = std::move(roomId),
             .roomGeneration =
               requiredSafeInteger(data, "roomGeneration", true) };
}

auto
parseRoomMemberJoined(const QJsonObject& data) -> RoomMemberJoined
{
    requireExactKeys(data, { "roomId", "roomGeneration", "member" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .member = parseMember(requiredObject(data, "member")) };
}

auto
parseRoomMemberUpdated(const QJsonObject& data) -> RoomMemberUpdated
{
    requireExactKeys(data, { "roomId", "roomGeneration", "member" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .member = parseMember(requiredObject(data, "member")) };
}

auto
parseRoomMemberLeft(const QJsonObject& data) -> RoomMemberLeft
{
    requireExactKeys(data,
                     { "roomId", "roomGeneration", "memberId", "reason" });
    auto header = parseRoomEventHeader(data);
    auto memberId = requiredString(data, "memberId");
    validateOpaqueId(memberId);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .memberId = std::move(memberId),
             .reason = parseMemberLeftReason(requiredString(data, "reason")) };
}

auto
parseRoomOwnerChanged(const QJsonObject& data) -> RoomOwnerChanged
{
    requireExactKeys(data, { "roomId", "roomGeneration", "ownerMemberId" });
    auto header = parseRoomEventHeader(data);
    auto owner = nullableString(data, "ownerMemberId");
    if (owner) {
        validateOpaqueId(*owner);
    }
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .ownerMemberId = std::move(owner) };
}

auto
parseChatMessageEvent(const QJsonObject& data) -> ChatMessageEvent
{
    requireExactKeys(data, { "roomId", "roomGeneration", "message" });
    auto header = parseRoomEventHeader(data);
    return { .roomId = std::move(header.roomId),
             .roomGeneration = header.roomGeneration,
             .message = parseChatMessage(requiredObject(data, "message")) };
}

auto
parseServerHeartbeat(const QJsonObject& data) -> ServerHeartbeat
{
    requireExactKeys(data, { "nonce", "sentAtMs" });
    auto nonce = requiredString(data, "nonce");
    validateOpaqueId(nonce);
    return { .nonce = std::move(nonce),
             .sentAtMs = requiredSafeInteger(data, "sentAtMs") };
}

auto
parseServerGoingAway(const QJsonObject& data) -> ServerGoingAway
{
    requireExactKeys(data, { "displayMessageKey" }, { "retryAfterMs" });
    if (requiredString(data, "displayMessageKey") !=
        QStringLiteral("arena.serverGoingAway")) {
        fail();
    }
    ServerGoingAway result;
    if (data.contains(QStringLiteral("retryAfterMs"))) {
        result.retryAfterMs = requiredSafeInteger(data, "retryAfterMs");
    }
    return result;
}

auto
parseCommandError(QString requestId, const QJsonObject& data) -> CommandError
{
    validateRequestId(requestId);
    requireExactKeys(data, { "code", "displayMessageKey" });
    const auto displayKey = requiredString(data, "displayMessageKey");
    if (!isDisplayMessageKey(displayKey)) {
        fail();
    }
    return { .requestId = std::move(requestId),
             .code = parseCommandErrorCode(requiredString(data, "code")),
             .displayMessageKey = displayKey };
}

} // namespace

auto
encodeClientMessage(const ClientMessage& message) -> EncodeClientResult
{
    try {
        const auto object = std::visit(
          [](const auto& value) -> QJsonObject {
              using Value = std::decay_t<decltype(value)>;
              if constexpr (std::is_same_v<Value, ClientHello>) {
                  return encodeHello(value);
              } else if constexpr (std::is_same_v<Value, DirectorySubscribe>) {
                  return encodeDirectorySubscribe(value);
              } else if constexpr (std::is_same_v<Value, RoomCreate>) {
                  return encodeRoomCreate(value);
              } else if constexpr (std::is_same_v<Value, RoomJoin>) {
                  return encodeRoomJoin(value);
              } else if constexpr (std::is_same_v<Value, RoomLeave>) {
                  return encodeRoomLeave(value);
              } else if constexpr (std::is_same_v<Value, RoomKick>) {
                  return encodeRoomKick(value);
              } else if constexpr (std::is_same_v<Value, ChatSend>) {
                  return encodeChatSend(value);
              } else if constexpr (std::is_same_v<Value, HeartbeatReply>) {
                  return encodeHeartbeatReply(value);
              } else {
                  fail();
              }
          },
          message);
        const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
        if (bytes.size() > MaxClientMessageBytes) {
            fail(ProtocolFailureCode::FrameTooLarge);
        }
        return QString::fromUtf8(bytes);
    } catch (const DecodeFailure& failure) {
        return ProtocolFailure{ failure.code };
    }
}

auto
decodeServerMessage(QStringView text) -> DecodeServerResult
{
    // UTF-8 cannot contain fewer bytes than the number of UTF-16 code units.
    // Reject obviously oversized frames without allocating a second copy.
    if (text.size() > MaxServerMessageBytes) {
        return ProtocolFailure{ ProtocolFailureCode::FrameTooLarge };
    }
    const auto bytes = text.toString().toUtf8();
    if (bytes.size() > MaxServerMessageBytes) {
        return ProtocolFailure{ ProtocolFailureCode::FrameTooLarge };
    }
    try {
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(bytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || document.isNull() ||
            !document.isObject()) {
            fail();
        }
        const auto envelope = document.object();
        const auto typeValue = envelope.value(QStringLiteral("type"));
        const auto dataValue = envelope.value(QStringLiteral("data"));
        if (!typeValue.isString() || !dataValue.isObject()) {
            fail();
        }
        const auto type = typeValue.toString();
        if (type == QStringLiteral("server_hello")) {
            // Version/capability negotiation failures take precedence over
            // unrelated schema errors in a recognizable hello.
            const auto data = dataValue.toObject();
            const auto major = data.value(QStringLiteral("protocolMajor"));
            const auto minor = data.value(QStringLiteral("protocolMinor"));
            if ((major.isDouble() && major.toDouble() != ProtocolMajor) ||
                (minor.isDouble() && minor.toDouble() != ProtocolMinor)) {
                fail(ProtocolFailureCode::ProtocolIncompatible);
            }
            const auto capabilities =
              data.value(QStringLiteral("capabilities"));
            if (capabilities.isArray()) {
                bool allStrings = true;
                bool foundRequired = false;
                for (const auto& value : capabilities.toArray()) {
                    allStrings = allStrings && value.isString();
                    foundRequired = foundRequired ||
                                    value.toString() ==
                                      QString::fromLatin1(RequiredCapability);
                }
                if (allStrings && !foundRequired) {
                    fail(ProtocolFailureCode::CapabilityRequired);
                }
            }
            requireExactKeys(envelope, { "type", "data" });
            return ServerMessage{ parseServerHello(data) };
        }
        const auto data = dataValue.toObject();
        if (type == QStringLiteral("room_snapshot")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            auto requestId = requiredString(envelope, "requestId");
            validateRequestId(requestId);
            return ServerMessage{ RoomSnapshotEvent{
              .requestId = std::move(requestId),
              .room = parseRoomSnapshot(data),
            } };
        }
        if (type == QStringLiteral("command_error")) {
            requireExactKeys(envelope, { "type", "requestId", "data" });
            return ServerMessage{ parseCommandError(
              requiredString(envelope, "requestId"), data) };
        }

        requireExactKeys(envelope, { "type", "data" });
        if (type == QStringLiteral("fatal_error")) {
            return ServerMessage{ parseFatalError(data) };
        }
        if (type == QStringLiteral("directory_snapshot")) {
            return ServerMessage{ parseDirectorySnapshot(data) };
        }
        if (type == QStringLiteral("room_directory_updated")) {
            return ServerMessage{ parseRoomDirectoryUpdated(data) };
        }
        if (type == QStringLiteral("room_member_joined")) {
            return ServerMessage{ parseRoomMemberJoined(data) };
        }
        if (type == QStringLiteral("room_member_updated")) {
            return ServerMessage{ parseRoomMemberUpdated(data) };
        }
        if (type == QStringLiteral("room_member_left")) {
            return ServerMessage{ parseRoomMemberLeft(data) };
        }
        if (type == QStringLiteral("room_owner_changed")) {
            return ServerMessage{ parseRoomOwnerChanged(data) };
        }
        if (type == QStringLiteral("chat_message")) {
            return ServerMessage{ parseChatMessageEvent(data) };
        }
        if (type == QStringLiteral("server_heartbeat")) {
            return ServerMessage{ parseServerHeartbeat(data) };
        }
        if (type == QStringLiteral("server_going_away")) {
            return ServerMessage{ parseServerGoingAway(data) };
        }
        fail();
    } catch (const DecodeFailure& failure) {
        return ProtocolFailure{ failure.code };
    }
}

auto
displayMessageKey(ProtocolFailureCode code) -> QString
{
    switch (code) {
        case ProtocolFailureCode::MalformedMessage:
            return QStringLiteral("arena.error.malformedMessage");
        case ProtocolFailureCode::FrameTooLarge:
            return QStringLiteral("arena.error.frameTooLarge");
        case ProtocolFailureCode::ProtocolIncompatible:
            return QStringLiteral("arena.error.protocolIncompatible");
        case ProtocolFailureCode::CapabilityRequired:
            return QStringLiteral("arena.error.capabilityRequired");
    }
    return {};
}

} // namespace arena
