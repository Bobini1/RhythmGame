#include "arena/ArenaProtocol.h"

#include <catch2/catch_test_macros.hpp>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QFile>
#include <QHash>
#include <QSet>

#include <concepts>
#include <limits>
#include <type_traits>
#include <variant>

namespace {

auto
objectFrom(const QString& text) -> QJsonObject
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    REQUIRE(parseError.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());
    return document.object();
}

auto
decodedMessage(const arena::DecodeServerResult& result)
  -> const arena::ServerMessage*
{
    return std::get_if<arena::ServerMessage>(&result);
}

auto
failureCode(const arena::DecodeServerResult& result)
  -> std::optional<arena::ProtocolFailureCode>
{
    const auto* failure = std::get_if<arena::ProtocolFailure>(&result);
    if (failure == nullptr) {
        return std::nullopt;
    }
    return failure->code;
}

auto
encodeFailureCode(const arena::EncodeClientResult& result)
  -> std::optional<arena::ProtocolFailureCode>
{
    const auto* failure = std::get_if<arena::ProtocolFailure>(&result);
    return failure == nullptr ? std::nullopt : std::optional{ failure->code };
}

template<typename T>
auto
messageAs(const arena::DecodeServerResult& result) -> const T*
{
    const auto* message = decodedMessage(result);
    return message == nullptr ? nullptr : std::get_if<T>(message);
}

auto
compact(const QJsonObject& object) -> QString
{
    return QString::fromUtf8(
      QJsonDocument(object).toJson(QJsonDocument::Compact));
}

auto
identityObject() -> QJsonObject
{
    return { { QStringLiteral("userId"), QStringLiteral("user-1") },
             { QStringLiteral("displayName"), QStringLiteral("Alice") },
             { QStringLiteral("avatarUrl"), QJsonValue::Null } };
}

auto
memberObject(QString id = QStringLiteral("member-1")) -> QJsonObject
{
    return { { QStringLiteral("memberId"), std::move(id) },
             { QStringLiteral("identity"), identityObject() },
             { QStringLiteral("status"), QStringLiteral("connected") },
             { QStringLiteral("lobbyWins"), 0 } };
}

auto
chatObject(QString id = QStringLiteral("message-1")) -> QJsonObject
{
    return { { QStringLiteral("messageId"), std::move(id) },
             { QStringLiteral("authorMemberId"), QStringLiteral("member-1") },
             { QStringLiteral("authorDisplayName"), QStringLiteral("Alice") },
             { QStringLiteral("sentAtMs"), 1'752'172'800'000.0 },
             { QStringLiteral("text"), QStringLiteral("Hello") } };
}

auto
roomSummaryObject(QString id = QStringLiteral("room-123")) -> QJsonObject
{
    return { { QStringLiteral("roomId"), std::move(id) },
             { QStringLiteral("name"), QStringLiteral("Arena room") },
             { QStringLiteral("phase"), QStringLiteral("selecting") },
             { QStringLiteral("hasPassword"), true },
             { QStringLiteral("connectedCount"), 1 },
             { QStringLiteral("reservedCount"), 0 },
             { QStringLiteral("maxCount"), arena::RoomCapacity },
             { QStringLiteral("members"),
               QJsonArray{ QJsonObject{
                 { QStringLiteral("displayName"), QStringLiteral("Alice") },
                 { QStringLiteral("avatarUrl"), QJsonValue::Null },
                 { QStringLiteral("connected"), true },
               } } } };
}

auto
roomSnapshotObject() -> QJsonObject
{
    return {
        { QStringLiteral("roomId"), QStringLiteral("room-123") },
        { QStringLiteral("roomGeneration"), 3 },
        { QStringLiteral("name"), QStringLiteral("Arena room") },
        { QStringLiteral("phase"), QStringLiteral("selecting") },
        { QStringLiteral("hasPassword"), true },
        { QStringLiteral("maxCount"), arena::RoomCapacity },
        { QStringLiteral("ownerMemberId"), QStringLiteral("member-1") },
        { QStringLiteral("self"),
          QJsonObject{
            { QStringLiteral("memberId"), QStringLiteral("member-1") },
            { QStringLiteral("connectionGeneration"), 2 },
            { QStringLiteral("resumeToken"),
              QStringLiteral("resume_token-123") } } },
        { QStringLiteral("members"), QJsonArray{ memberObject() } },
        { QStringLiteral("chat"), QJsonArray{ chatObject() } },
    };
}

auto
envelope(QString type,
         QJsonObject data,
         std::optional<QString> requestId = std::nullopt) -> QString
{
    QJsonObject object{ { QStringLiteral("type"), std::move(type) },
                        { QStringLiteral("data"), std::move(data) } };
    if (requestId) {
        object.insert(QStringLiteral("requestId"), *requestId);
    }
    return compact(object);
}

auto
keysOf(const QJsonObject& object) -> QSet<QString>
{
    QSet<QString> keys;
    for (const auto& key : object.keys()) {
        keys.insert(key);
    }
    return keys;
}

auto
loadProtocolFixture() -> QJsonObject
{
    QFile file(QString::fromUtf8(ARENA_PROTOCOL_FIXTURE_PATH));
    REQUIRE(file.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    REQUIRE(parseError.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());
    return document.object();
}

auto
loadPhase2ProtocolFixture() -> QJsonObject
{
    QFile file(QString::fromUtf8(ARENA_PHASE2_PROTOCOL_FIXTURE_PATH));
    REQUIRE(file.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    REQUIRE(parseError.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());
    return document.object();
}

auto
loadPhase3ProtocolFixture() -> QJsonObject
{
    QFile file(QString::fromUtf8(ARENA_PHASE3_PROTOCOL_FIXTURE_PATH));
    REQUIRE(file.open(QIODevice::ReadOnly));
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    REQUIRE(parseError.error == QJsonParseError::NoError);
    REQUIRE(document.isObject());
    return document.object();
}

auto
fixtureMessage(const QJsonObject& fixture,
               QStringView listKey,
               QStringView type) -> QJsonObject
{
    for (const auto& value : fixture.value(listKey.toString()).toArray()) {
        const auto message =
          value.toObject().value(QStringLiteral("message")).toObject();
        if (message.value(QStringLiteral("type")).toString() == type) {
            return message;
        }
    }
    FAIL_CHECK("missing fixture message");
    return {};
}

auto
clientMessageFromFixture(const QJsonObject& object)
  -> std::optional<arena::ClientMessage>
{
    using namespace arena;
    const auto type = object.value(QStringLiteral("type")).toString();
    const auto data = object.value(QStringLiteral("data")).toObject();
    const auto requestId = object.value(QStringLiteral("requestId")).toString();
    if (type == QStringLiteral("client_hello")) {
        ClientHello hello{
            .protocolMajor =
              data.value(QStringLiteral("protocolMajor")).toInt(),
            .protocolMinor =
              data.value(QStringLiteral("protocolMinor")).toInt(),
            .clientVersion =
              data.value(QStringLiteral("clientVersion")).toString(),
        };
        hello.capabilities.clear();
        for (const auto& value :
             data.value(QStringLiteral("capabilities")).toArray()) {
            hello.capabilities.push_back(value.toString());
        }
        if (data.contains(QStringLiteral("ticket"))) {
            hello.ticket = data.value(QStringLiteral("ticket")).toString();
        }
        if (data.contains(QStringLiteral("resume"))) {
            const auto resume = data.value(QStringLiteral("resume")).toObject();
            hello.resume = ResumeRequest{
                .roomId = resume.value(QStringLiteral("roomId")).toString(),
                .seatToken =
                  resume.value(QStringLiteral("seatToken")).toString(),
            };
        }
        return ClientMessage{ std::move(hello) };
    }
    if (type == QStringLiteral("directory_subscribe")) {
        return ClientMessage{ DirectorySubscribe{} };
    }
    if (type == QStringLiteral("room_create")) {
        auto message = RoomCreate{
            .requestId = requestId,
            .name = data.value(QStringLiteral("name")).toString(),
        };
        if (data.contains(QStringLiteral("password"))) {
            message.password =
              data.value(QStringLiteral("password")).toString();
        }
        return ClientMessage{ std::move(message) };
    }
    if (type == QStringLiteral("room_join")) {
        auto message = RoomJoin{
            .requestId = requestId,
            .roomId = data.value(QStringLiteral("roomId")).toString(),
        };
        if (data.contains(QStringLiteral("password"))) {
            message.password =
              data.value(QStringLiteral("password")).toString();
        }
        return ClientMessage{ std::move(message) };
    }
    if (type == QStringLiteral("room_leave")) {
        return ClientMessage{ RoomLeave{
          .requestId = requestId,
          .roomId = data.value(QStringLiteral("roomId")).toString(),
          .roomGeneration =
            data.value(QStringLiteral("roomGeneration")).toInteger(),
          .connectionGeneration =
            data.value(QStringLiteral("connectionGeneration")).toInteger(),
        } };
    }
    if (type == QStringLiteral("room_kick")) {
        return ClientMessage{ RoomKick{
          .requestId = requestId,
          .roomId = data.value(QStringLiteral("roomId")).toString(),
          .roomGeneration =
            data.value(QStringLiteral("roomGeneration")).toInteger(),
          .connectionGeneration =
            data.value(QStringLiteral("connectionGeneration")).toInteger(),
          .targetMemberId =
            data.value(QStringLiteral("targetMemberId")).toString(),
        } };
    }
    if (type == QStringLiteral("chat_send")) {
        return ClientMessage{ ChatSend{
          .requestId = requestId,
          .roomId = data.value(QStringLiteral("roomId")).toString(),
          .roomGeneration =
            data.value(QStringLiteral("roomGeneration")).toInteger(),
          .connectionGeneration =
            data.value(QStringLiteral("connectionGeneration")).toInteger(),
          .text = data.value(QStringLiteral("text")).toString(),
        } };
    }
    if (type == QStringLiteral("heartbeat_reply")) {
        return ClientMessage{ HeartbeatReply{
          .nonce = data.value(QStringLiteral("nonce")).toString(),
        } };
    }
    return std::nullopt;
}

auto
selectionFromPhase2Fixture(const QJsonObject& object)
  -> arena::SelectionSnapshot
{
    using namespace arena;
    REQUIRE(object.value(QStringLiteral("noteOrderP1")).toString() ==
            QStringLiteral("s_random_plus"));
    REQUIRE(object.value(QStringLiteral("noteOrderP2")).toString() ==
            QStringLiteral("lr2_random_ex"));
    REQUIRE(object.value(QStringLiteral("dpMode")).toString() ==
            QStringLiteral("lr2_flip"));

    QVector<qint64> sequence;
    for (const auto& value :
         object.value(QStringLiteral("randomSequence")).toArray()) {
        sequence.push_back(value.toInteger());
    }
    SelectionSnapshot selection{
        .sha256 = object.value(QStringLiteral("sha256")).toString(),
        .title = object.value(QStringLiteral("title")).toString(),
        .subtitle = object.value(QStringLiteral("subtitle")).toString(),
        .artist = object.value(QStringLiteral("artist")).toString(),
        .keyMode = object.value(QStringLiteral("keyMode")).toInt(),
        .randomSequence = std::move(sequence),
        .noteOrderP1 = NoteOrder::SRandomPlus,
        .noteOrderP2 = NoteOrder::Lr2RandomEx,
        .dpMode = DpMode::Lr2Flip,
        .laneSeed = object.value(QStringLiteral("laneSeed")).toString(),
        .randomizationVersion =
          object.value(QStringLiteral("randomizationVersion")).toInt(),
    };
    if (object.contains(QStringLiteral("md5"))) {
        selection.md5 = object.value(QStringLiteral("md5")).toString();
    }
    return selection;
}

auto
validSelection() -> arena::SelectionSnapshot
{
    return {
        .sha256 = QString(64, QChar(u'a')),
        .md5 = QString(32, QChar(u'b')),
        .title = QStringLiteral("Chart"),
        .subtitle = {},
        .artist = QStringLiteral("Artist"),
        .keyMode = 7,
        .randomSequence = { 1, 2, 3 },
        .noteOrderP1 = arena::NoteOrder::Random,
        .noteOrderP2 = arena::NoteOrder::Mirror,
        .dpMode = arena::DpMode::Off,
        .laneSeed = QStringLiteral("0123456789abcdef"),
        .randomizationVersion = 1,
    };
}

auto
validJudgements() -> arena::ArenaJudgements
{
    return { .perfect = 40,
             .great = 20,
             .good = 4,
             .bad = 1,
             .poor = 2,
             .emptyPoor = 3 };
}

auto
validTelemetry() -> arena::TelemetrySnapshot
{
    return { .sequence = 7,
             .exScore = 100,
             .progressPermille = 500,
             .maxCombo = 64,
             .badPoorCount = 6,
             .judgements = validJudgements(),
             .gauge = { .type = arena::GaugeType::Normal,
                        .valueMilli = 55'000 } };
}

auto
validFinalResult() -> arena::FinalResult
{
    return { .exScore = 100,
             .maxCombo = 64,
             .badPoorCount = 6,
             .judgements = validJudgements(),
             .clearType = arena::ClearType::Normal,
             .finalGauge = { .type = arena::GaugeType::Normal,
                             .valueMilli = 75'000 } };
}

auto
phase3ClientMessageFromFixture(const QJsonObject& object)
  -> std::optional<arena::ClientMessage>
{
    using namespace arena;
    if (const auto phase1 = clientMessageFromFixture(object)) {
        return phase1;
    }
    const auto type = object.value(QStringLiteral("type")).toString();
    const auto requestId = object.value(QStringLiteral("requestId")).toString();
    const auto data = object.value(QStringLiteral("data")).toObject();
    const auto roomId = data.value(QStringLiteral("roomId")).toString();
    const auto roomGeneration =
      data.value(QStringLiteral("roomGeneration")).toInteger();
    const auto connectionGeneration =
      data.value(QStringLiteral("connectionGeneration")).toInteger();
    const auto roundId = data.value(QStringLiteral("roundId")).toString();
    const auto launchAttemptId =
      data.value(QStringLiteral("launchAttemptId")).toString();

    if (type == QStringLiteral("round_load_result")) {
        return ClientMessage{ RoundLoadResult{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .roundId = roundId,
          .launchAttemptId = launchAttemptId,
          .selectionRevision =
            data.value(QStringLiteral("selectionRevision")).toInteger(),
          .availabilityRevision =
            data.value(QStringLiteral("availabilityRevision")).toInteger(),
          .inventoryRevision =
            data.value(QStringLiteral("inventoryRevision")).toInteger(),
          .ok = true,
          .chartLengthMs =
            data.value(QStringLiteral("chartLengthMs")).toInteger(),
        } };
    }
    if (type == QStringLiteral("round_telemetry")) {
        return ClientMessage{ RoundTelemetry{
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .roundId = roundId,
          .launchAttemptId = launchAttemptId,
          .telemetry = validTelemetry(),
        } };
    }
    if (type == QStringLiteral("round_result_submit")) {
        return ClientMessage{ RoundResultSubmit{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .roundId = roundId,
          .launchAttemptId = launchAttemptId,
          .result = validFinalResult(),
        } };
    }
    if (type == QStringLiteral("round_abandon")) {
        return ClientMessage{ RoundAbandon{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .roundId = roundId,
          .launchAttemptId = launchAttemptId,
          .reason = DnfReason::Aborted,
        } };
    }
    return std::nullopt;
}

auto
phase2ClientMessageFromFixture(const QJsonObject& object)
  -> std::optional<arena::ClientMessage>
{
    using namespace arena;
    const auto type = object.value(QStringLiteral("type")).toString();
    const auto requestId = object.value(QStringLiteral("requestId")).toString();
    const auto data = object.value(QStringLiteral("data")).toObject();
    const auto roomId = data.value(QStringLiteral("roomId")).toString();
    const auto roomGeneration =
      data.value(QStringLiteral("roomGeneration")).toInteger();
    const auto connectionGeneration =
      data.value(QStringLiteral("connectionGeneration")).toInteger();

    if (type == QStringLiteral("inventory_upload_begin")) {
        return ClientMessage{ InventoryUploadBegin{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .libraryGeneration =
            data.value(QStringLiteral("libraryGeneration")).toInteger(),
          .hashCount = data.value(QStringLiteral("hashCount")).toInteger(),
          .byteCount = data.value(QStringLiteral("byteCount")).toInteger(),
          .chunkCount = data.value(QStringLiteral("chunkCount")).toInteger(),
          .vectorDigest = data.value(QStringLiteral("vectorDigest")).toString(),
        } };
    }
    if (type == QStringLiteral("inventory_upload_commit")) {
        return ClientMessage{ InventoryUploadCommit{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .uploadId = data.value(QStringLiteral("uploadId")).toString(),
          .libraryGeneration =
            data.value(QStringLiteral("libraryGeneration")).toInteger(),
          .hashCount = data.value(QStringLiteral("hashCount")).toInteger(),
          .byteCount = data.value(QStringLiteral("byteCount")).toInteger(),
          .chunkCount = data.value(QStringLiteral("chunkCount")).toInteger(),
          .vectorDigest = data.value(QStringLiteral("vectorDigest")).toString(),
        } };
    }
    if (type == QStringLiteral("inventory_upload_abort")) {
        return ClientMessage{ InventoryUploadAbort{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .uploadId = data.value(QStringLiteral("uploadId")).toString(),
          .libraryGeneration =
            data.value(QStringLiteral("libraryGeneration")).toInteger(),
        } };
    }
    if (type == QStringLiteral("availability_applied")) {
        return ClientMessage{ AvailabilityApplied{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .availabilityRevision =
            data.value(QStringLiteral("availabilityRevision")).toInteger(),
        } };
    }
    if (type == QStringLiteral("availability_resync")) {
        return ClientMessage{ AvailabilityResync{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .currentRevision =
            data.value(QStringLiteral("currentRevision")).toInteger(),
        } };
    }
    if (type == QStringLiteral("selection_set")) {
        return ClientMessage{ SelectionSet{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .availabilityRevision =
            data.value(QStringLiteral("availabilityRevision")).toInteger(),
          .inventoryRevision =
            data.value(QStringLiteral("inventoryRevision")).toInteger(),
          .selection = selectionFromPhase2Fixture(
            data.value(QStringLiteral("selection")).toObject()),
        } };
    }
    if (type == QStringLiteral("ready_set")) {
        return ClientMessage{ ReadySet{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .ready = data.value(QStringLiteral("ready")).toBool(),
          .selectionRevision =
            data.value(QStringLiteral("selectionRevision")).toInteger(),
          .availabilityRevision =
            data.value(QStringLiteral("availabilityRevision")).toInteger(),
          .inventoryRevision =
            data.value(QStringLiteral("inventoryRevision")).toInteger(),
        } };
    }
    if (type == QStringLiteral("round_probe_result")) {
        RoundProbeResult result{
            .requestId = requestId,
            .roomId = roomId,
            .roomGeneration = roomGeneration,
            .connectionGeneration = connectionGeneration,
            .roundId = data.value(QStringLiteral("roundId")).toString(),
            .launchAttemptId =
              data.value(QStringLiteral("launchAttemptId")).toString(),
            .selectionRevision =
              data.value(QStringLiteral("selectionRevision")).toInteger(),
            .availabilityRevision =
              data.value(QStringLiteral("availabilityRevision")).toInteger(),
            .inventoryRevision =
              data.value(QStringLiteral("inventoryRevision")).toInteger(),
            .nonce = data.value(QStringLiteral("nonce")).toString(),
            .ok = data.value(QStringLiteral("ok")).toBool(),
        };
        if (result.ok) {
            result.sha256 = data.value(QStringLiteral("sha256")).toString();
        }
        return ClientMessage{ std::move(result) };
    }
    if (type == QStringLiteral("round_load_result")) {
        return ClientMessage{ RoundLoadResult{
          .requestId = requestId,
          .roomId = roomId,
          .roomGeneration = roomGeneration,
          .connectionGeneration = connectionGeneration,
          .roundId = data.value(QStringLiteral("roundId")).toString(),
          .launchAttemptId =
            data.value(QStringLiteral("launchAttemptId")).toString(),
          .selectionRevision =
            data.value(QStringLiteral("selectionRevision")).toInteger(),
          .availabilityRevision =
            data.value(QStringLiteral("availabilityRevision")).toInteger(),
          .inventoryRevision =
            data.value(QStringLiteral("inventoryRevision")).toInteger(),
          .ok = data.value(QStringLiteral("ok")).toBool(),
          .failureReason = RoundLoadFailureReason::ResourceFailed,
        } };
    }
    return std::nullopt;
}

auto
serverMessageType(const arena::ServerMessage& message) -> QString
{
    return std::visit(
      []<typename T>(const T&) -> QString {
          if constexpr (std::same_as<T, arena::ServerHello>) {
              return QStringLiteral("server_hello");
          } else if constexpr (std::same_as<T, arena::FatalError>) {
              return QStringLiteral("fatal_error");
          } else if constexpr (std::same_as<T, arena::DirectorySnapshot>) {
              return QStringLiteral("directory_snapshot");
          } else if constexpr (std::same_as<T, arena::RoomDirectoryUpdated>) {
              return QStringLiteral("room_directory_updated");
          } else if constexpr (std::same_as<T, arena::RoomSnapshotEvent>) {
              return QStringLiteral("room_snapshot");
          } else if constexpr (std::same_as<T, arena::RoomMemberJoined>) {
              return QStringLiteral("room_member_joined");
          } else if constexpr (std::same_as<T, arena::RoomMemberUpdated>) {
              return QStringLiteral("room_member_updated");
          } else if constexpr (std::same_as<T, arena::RoomMemberLeft>) {
              return QStringLiteral("room_member_left");
          } else if constexpr (std::same_as<T, arena::RoomOwnerChanged>) {
              return QStringLiteral("room_owner_changed");
          } else if constexpr (std::same_as<T, arena::ChatMessageEvent>) {
              return QStringLiteral("chat_message");
          } else if constexpr (std::same_as<T, arena::ServerHeartbeat>) {
              return QStringLiteral("server_heartbeat");
          } else if constexpr (std::same_as<T, arena::ServerGoingAway>) {
              return QStringLiteral("server_going_away");
          } else if constexpr (std::same_as<T, arena::CommandError>) {
              return QStringLiteral("command_error");
          } else if constexpr (std::same_as<T, arena::InventoryUploadReady>) {
              return QStringLiteral("inventory_upload_ready");
          } else if constexpr (std::same_as<T, arena::InventoryCommitted>) {
              return QStringLiteral("inventory_committed");
          } else if constexpr (std::same_as<T,
                                            arena::AvailabilityTransferBegin>) {
              return QStringLiteral("availability_transfer_begin");
          } else if constexpr (std::same_as<
                                 T,
                                 arena::AvailabilityTransferCommit>) {
              return QStringLiteral("availability_transfer_commit");
          } else if constexpr (std::same_as<T, arena::SelectionChanged>) {
              return QStringLiteral("selection_changed");
          } else if constexpr (std::same_as<T, arena::SelectionRejected>) {
              return QStringLiteral("selection_rejected");
          } else if constexpr (std::same_as<T, arena::RoundLoadingStarted>) {
              return QStringLiteral("round_loading_started");
          } else if constexpr (std::same_as<T, arena::RoundProbeRequested>) {
              return QStringLiteral("round_probe_requested");
          } else if constexpr (std::same_as<T, arena::RoundLoadRequested>) {
              return QStringLiteral("round_load_requested");
          } else if constexpr (std::same_as<T, arena::RoundStartScheduled>) {
              return QStringLiteral("round_start_scheduled");
          } else if constexpr (std::same_as<T, arena::RoundStarted>) {
              return QStringLiteral("round_started");
          } else if constexpr (std::same_as<T, arena::LiveStandingsSnapshot>) {
              return QStringLiteral("round_standings");
          } else if constexpr (std::same_as<T, arena::RoundTerminalAccepted>) {
              return QStringLiteral("round_terminal_accepted");
          } else if constexpr (std::same_as<T, arena::RoundFinalized>) {
              return QStringLiteral("round_finalized");
          } else if constexpr (std::same_as<T, arena::RoundLaunchCancelled>) {
              return QStringLiteral("round_launch_cancelled");
          }
      },
      message);
}

auto
fixtureFailureCode(const QString& code)
  -> std::optional<arena::ProtocolFailureCode>
{
    if (code == QStringLiteral("malformed_message")) {
        return arena::ProtocolFailureCode::MalformedMessage;
    }
    if (code == QStringLiteral("protocol_incompatible")) {
        return arena::ProtocolFailureCode::ProtocolIncompatible;
    }
    if (code == QStringLiteral("capability_required")) {
        return arena::ProtocolFailureCode::CapabilityRequired;
    }
    return std::nullopt;
}

} // namespace

TEST_CASE("ArenaProtocol uses exact 1.0 with 32 public member previews",
          "[arena][protocol][directory-members]")
{
    using namespace arena;
    STATIC_REQUIRE(ProtocolMajor == 1);
    STATIC_REQUIRE(ProtocolMinor == 0);
    STATIC_REQUIRE(RoomCapacity == 32);

    QJsonArray members;
    for (int index = 0; index < RoomCapacity; ++index) {
        members.push_back(QJsonObject{
          { QStringLiteral("displayName"),
            QStringLiteral("Player %1").arg(index + 1) },
          { QStringLiteral("avatarUrl"),
            index == 1 ? QJsonValue{ QStringLiteral(
                           "https://example.test/player-2.png") }
                       : QJsonValue{ QJsonValue::Null } },
          { QStringLiteral("connected"), index != 2 },
        });
    }
    auto summary = roomSummaryObject();
    summary.insert(QStringLiteral("connectedCount"), RoomCapacity - 1);
    summary.insert(QStringLiteral("reservedCount"), 1);
    summary.insert(QStringLiteral("members"), members);
    const auto decoded = decodeServerMessage(
      envelope(QStringLiteral("directory_snapshot"),
               { { QStringLiteral("revision"), 1 },
                 { QStringLiteral("rooms"), QJsonArray{ summary } } }));
    const auto* snapshot = messageAs<DirectorySnapshot>(decoded);
    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->rooms.front().members.size() == RoomCapacity);
    CHECK(snapshot->rooms.front().members[1].displayName ==
          QStringLiteral("Player 2"));
    CHECK(snapshot->rooms.front().members[1].avatarUrl ==
          QStringLiteral("https://example.test/player-2.png"));
    CHECK_FALSE(snapshot->rooms.front().members[2].connected);

    members.push_back(QJsonObject{
      { QStringLiteral("displayName"), QStringLiteral("Player 33") },
      { QStringLiteral("avatarUrl"), QJsonValue::Null },
      { QStringLiteral("connected"), true } });
    summary.insert(QStringLiteral("members"), members);
    CHECK(failureCode(decodeServerMessage(envelope(
            QStringLiteral("directory_snapshot"),
            { { QStringLiteral("revision"), 1 },
              { QStringLiteral("rooms"), QJsonArray{ summary } } }))) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol consumes the canonical cross-language fixture",
          "[arena][protocol][fixture]")
{
    using namespace arena;
    const auto fixture = loadProtocolFixture();
    CHECK(keysOf(fixture) ==
          QSet<QString>{ QStringLiteral("fixtureSchema"),
                         QStringLiteral("protocolMajor"),
                         QStringLiteral("protocolMinor"),
                         QStringLiteral("clientMessages"),
                         QStringLiteral("serverMessages"),
                         QStringLiteral("invalidServerMessages") });
    CHECK(fixture.value(QStringLiteral("fixtureSchema")).toInt() == 1);
    CHECK(fixture.value(QStringLiteral("protocolMajor")).toInt() ==
          ProtocolMajor);
    CHECK(fixture.value(QStringLiteral("protocolMinor")).toInt() ==
          ProtocolMinor);

    QSet<QString> caseNames;
    for (const auto& value :
         fixture.value(QStringLiteral("clientMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto name = fixtureCase.value(QStringLiteral("name")).toString();
        CAPTURE(name);
        REQUIRE_FALSE(name.isEmpty());
        REQUIRE_FALSE(caseNames.contains(name));
        caseNames.insert(name);
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto message = clientMessageFromFixture(expected);
        REQUIRE(message.has_value());
        const auto encoded = encodeClientMessage(*message);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded)) == expected);
    }

    for (const auto& value :
         fixture.value(QStringLiteral("serverMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto name = fixtureCase.value(QStringLiteral("name")).toString();
        CAPTURE(name);
        REQUIRE_FALSE(name.isEmpty());
        REQUIRE_FALSE(caseNames.contains(name));
        caseNames.insert(name);
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto decoded = decodeServerMessage(compact(expected));
        REQUIRE(decodedMessage(decoded) != nullptr);
        CHECK(serverMessageType(*decodedMessage(decoded)) ==
              expected.value(QStringLiteral("type")).toString());
    }

    for (const auto& value :
         fixture.value(QStringLiteral("invalidServerMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(keysOf(fixtureCase) ==
                QSet<QString>{ QStringLiteral("name"),
                               QStringLiteral("typescriptFailure"),
                               QStringLiteral("cppFailure"),
                               QStringLiteral("message") });
        const auto name = fixtureCase.value(QStringLiteral("name")).toString();
        CAPTURE(name);
        REQUIRE_FALSE(name.isEmpty());
        REQUIRE_FALSE(caseNames.contains(name));
        caseNames.insert(name);
        CHECK(
          fixtureCase.value(QStringLiteral("typescriptFailure")).toString() ==
          QStringLiteral("malformed_message"));
        const auto expectedFailure = fixtureFailureCode(
          fixtureCase.value(QStringLiteral("cppFailure")).toString());
        REQUIRE(expectedFailure.has_value());
        const auto decoded = decodeServerMessage(
          compact(fixtureCase.value(QStringLiteral("message")).toObject()));
        CHECK(failureCode(decoded) == expectedFailure);
    }
}

TEST_CASE("ArenaProtocol consumes every canonical Phase 2 text golden",
          "[arena][protocol][fixture][phase2]")
{
    using namespace arena;
    const auto fixture = loadPhase2ProtocolFixture();
    CHECK(keysOf(fixture) ==
          QSet<QString>{ QStringLiteral("fixtureSchema"),
                         QStringLiteral("protocolMajor"),
                         QStringLiteral("protocolMinor"),
                         QStringLiteral("clientMessages"),
                         QStringLiteral("serverMessages"),
                         QStringLiteral("strictInvalidClientTypes"),
                         QStringLiteral("strictInvalidServerTypes") });
    CHECK(fixture.value(QStringLiteral("fixtureSchema")).toInt() == 1);
    CHECK(fixture.value(QStringLiteral("protocolMajor")).toInt() ==
          ProtocolMajor);
    CHECK(fixture.value(QStringLiteral("protocolMinor")).toInt() ==
          ProtocolMinor);

    QSet<QString> clientTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("clientMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto type = expected.value(QStringLiteral("type")).toString();
        CAPTURE(type);
        REQUIRE_FALSE(clientTypes.contains(type));
        clientTypes.insert(type);
        const auto message = phase2ClientMessageFromFixture(expected);
        REQUIRE(message.has_value());
        const auto encoded = encodeClientMessage(*message);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded)) == expected);
    }

    QSet<QString> serverTypes;
    QHash<QString, QJsonObject> serverGoldens;
    for (const auto& value :
         fixture.value(QStringLiteral("serverMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto type = expected.value(QStringLiteral("type")).toString();
        CAPTURE(type);
        REQUIRE_FALSE(serverTypes.contains(type));
        serverTypes.insert(type);
        serverGoldens.insert(type, expected);
        const auto decoded = decodeServerMessage(compact(expected));
        REQUIRE(decodedMessage(decoded) != nullptr);
        CHECK(serverMessageType(*decodedMessage(decoded)) == type);
    }

    QSet<QString> strictClientTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("strictInvalidClientTypes")).toArray()) {
        REQUIRE(value.isString());
        strictClientTypes.insert(value.toString());
    }
    CHECK(strictClientTypes == clientTypes);

    QSet<QString> strictServerTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("strictInvalidServerTypes")).toArray()) {
        REQUIRE(value.isString());
        const auto type = value.toString();
        strictServerTypes.insert(type);
        REQUIRE(serverGoldens.contains(type));
        auto invalid = serverGoldens.value(type);
        auto data = invalid.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("unexpected"), true);
        invalid.insert(QStringLiteral("data"), data);
        CAPTURE(type);
        CHECK(failureCode(decodeServerMessage(compact(invalid))) ==
              ProtocolFailureCode::MalformedMessage);
    }
    CHECK(strictServerTypes == serverTypes);
}

TEST_CASE("ArenaProtocol consumes every canonical Phase 3 text golden",
          "[arena][protocol][fixture][phase3]")
{
    using namespace arena;
    const auto fixture = loadPhase3ProtocolFixture();
    CHECK(keysOf(fixture) ==
          QSet<QString>{ QStringLiteral("fixtureSchema"),
                         QStringLiteral("protocolMajor"),
                         QStringLiteral("protocolMinor"),
                         QStringLiteral("clientMessages"),
                         QStringLiteral("serverMessages"),
                         QStringLiteral("strictInvalidClientTypes"),
                         QStringLiteral("strictInvalidServerTypes") });
    CHECK(fixture.value(QStringLiteral("fixtureSchema")).toInt() == 1);
    CHECK(fixture.value(QStringLiteral("protocolMajor")).toInt() ==
          ProtocolMajor);
    CHECK(fixture.value(QStringLiteral("protocolMinor")).toInt() ==
          ProtocolMinor);

    QSet<QString> clientTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("clientMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto type = expected.value(QStringLiteral("type")).toString();
        CAPTURE(type);
        REQUIRE_FALSE(clientTypes.contains(type));
        clientTypes.insert(type);
        const auto message = phase3ClientMessageFromFixture(expected);
        REQUIRE(message.has_value());
        const auto encoded = encodeClientMessage(*message);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded)) == expected);
    }

    QSet<QString> serverTypes;
    QHash<QString, QJsonObject> serverGoldens;
    for (const auto& value :
         fixture.value(QStringLiteral("serverMessages")).toArray()) {
        const auto fixtureCase = value.toObject();
        REQUIRE(
          keysOf(fixtureCase) ==
          QSet<QString>{ QStringLiteral("name"), QStringLiteral("message") });
        const auto expected =
          fixtureCase.value(QStringLiteral("message")).toObject();
        const auto type = expected.value(QStringLiteral("type")).toString();
        CAPTURE(type);
        REQUIRE_FALSE(serverTypes.contains(type));
        serverTypes.insert(type);
        serverGoldens.insert(type, expected);
        const auto decoded = decodeServerMessage(compact(expected));
        REQUIRE(decodedMessage(decoded) != nullptr);
        CHECK(serverMessageType(*decodedMessage(decoded)) == type);
    }

    QSet<QString> strictClientTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("strictInvalidClientTypes")).toArray()) {
        REQUIRE(value.isString());
        strictClientTypes.insert(value.toString());
    }
    CHECK(strictClientTypes == clientTypes);

    QSet<QString> strictServerTypes;
    for (const auto& value :
         fixture.value(QStringLiteral("strictInvalidServerTypes")).toArray()) {
        REQUIRE(value.isString());
        const auto type = value.toString();
        strictServerTypes.insert(type);
        REQUIRE(serverGoldens.contains(type));
        auto invalid = serverGoldens.value(type);
        auto data = invalid.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("unexpected"), true);
        invalid.insert(QStringLiteral("data"), data);
        CAPTURE(type);
        CHECK(failureCode(decodeServerMessage(compact(invalid))) ==
              ProtocolFailureCode::MalformedMessage);
    }
    CHECK(strictServerTypes == serverTypes);

    const auto standings = decodeServerMessage(
      compact(serverGoldens.value(QStringLiteral("round_standings"))));
    const auto* snapshot = messageAs<LiveStandingsSnapshot>(standings);
    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->entries.size() == 1);
    const auto* active =
      std::get_if<LiveActiveStanding>(&snapshot->entries.front().state);
    REQUIRE(active != nullptr);
    REQUIRE(active->telemetry.has_value());
    CHECK(active->telemetry->exScore == 100);

    const auto finalized = decodeServerMessage(
      compact(serverGoldens.value(QStringLiteral("round_finalized"))));
    const auto* result = messageAs<RoundFinalized>(finalized);
    REQUIRE(result != nullptr);
    REQUIRE(result->result.entries.size() == 1);
    CHECK(result->result.winnerMemberIds ==
          QVector<QString>{ QStringLiteral("member-phase3") });
}

TEST_CASE("ArenaProtocol validates Phase 2 inventory declarations exactly",
          "[arena][protocol][phase2]")
{
    using namespace arena;
    auto begin = InventoryUploadBegin{
        .requestId = QStringLiteral("inventory-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .libraryGeneration = 1,
        .hashCount = 2,
        .byteCount = 64,
        .chunkCount = 1,
        .vectorDigest = QString(64, QChar(u'a')),
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(begin)));

    begin.byteCount = 63;
    CHECK(encodeFailureCode(encodeClientMessage(begin)) ==
          ProtocolFailureCode::MalformedMessage);
    begin.byteCount = 64;
    begin.chunkCount = 0;
    CHECK(encodeFailureCode(encodeClientMessage(begin)) ==
          ProtocolFailureCode::MalformedMessage);
    begin.chunkCount = 1;
    begin.hashCount = 250'001;
    begin.byteCount = 8'000'032;
    CHECK(encodeFailureCode(encodeClientMessage(begin)) ==
          ProtocolFailureCode::MalformedMessage);

    auto commit = InventoryUploadCommit{
        .requestId = QStringLiteral("inventory-2"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .uploadId = QStringLiteral("AAAAAAAAAAAAAAAAAAAAAA"),
        .libraryGeneration = 1,
        .hashCount = 0,
        .byteCount = 0,
        .chunkCount = 0,
        .vectorDigest = QString(64, QChar(u'b')),
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(commit)));
    commit.uploadId.chop(1);
    CHECK(encodeFailureCode(encodeClientMessage(commit)) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol validates deterministic selection and result unions",
          "[arena][protocol][phase2]")
{
    using namespace arena;
    auto command = SelectionSet{
        .requestId = QStringLiteral("selection-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .availabilityRevision = 1,
        .inventoryRevision = 1,
        .selection = validSelection(),
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(command)));

    command.selection.randomSequence.fill(1, MaxRandomSequenceEntries + 1);
    CHECK(encodeFailureCode(encodeClientMessage(command)) ==
          ProtocolFailureCode::MalformedMessage);
    command.selection = validSelection();
    command.selection.randomSequence = { 0 };
    CHECK(encodeFailureCode(encodeClientMessage(command)) ==
          ProtocolFailureCode::MalformedMessage);
    command.selection = validSelection();
    command.selection.title = QString(201, QChar(u'x'));
    CHECK(encodeFailureCode(encodeClientMessage(command)) ==
          ProtocolFailureCode::MalformedMessage);
    command.selection = validSelection();
    command.selection.laneSeed = QStringLiteral("0123456789abcdeF");
    CHECK(encodeFailureCode(encodeClientMessage(command)) ==
          ProtocolFailureCode::MalformedMessage);

    auto probe = RoundProbeResult{
        .requestId = QStringLiteral("probe-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .selectionRevision = 1,
        .availabilityRevision = 1,
        .inventoryRevision = 1,
        .nonce = QStringLiteral("nonce-1"),
        .ok = true,
        .sha256 = QString(64, QChar(u'a')),
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(probe)));
    probe.failureReason = RoundProbeFailureReason::Cancelled;
    CHECK(encodeFailureCode(encodeClientMessage(probe)) ==
          ProtocolFailureCode::MalformedMessage);
    probe.ok = false;
    probe.sha256.reset();
    CHECK(std::holds_alternative<QString>(encodeClientMessage(probe)));
    probe.failureReason.reset();
    CHECK(encodeFailureCode(encodeClientMessage(probe)) ==
          ProtocolFailureCode::MalformedMessage);

    auto loaded = RoundLoadResult{
        .requestId = QStringLiteral("load-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .selectionRevision = 1,
        .availabilityRevision = 1,
        .inventoryRevision = 1,
        .ok = true,
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(loaded)));
    loaded.failureReason = RoundLoadFailureReason::Cancelled;
    CHECK(encodeFailureCode(encodeClientMessage(loaded)) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE(
  "ArenaProtocol encodes bounded competition telemetry and terminal unions",
  "[arena][protocol][phase3]")
{
    using namespace arena;

    auto telemetry = RoundTelemetry{
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 2,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .telemetry = validTelemetry(),
    };
    const auto encodedTelemetry = encodeClientMessage(telemetry);
    REQUIRE(std::holds_alternative<QString>(encodedTelemetry));
    const auto telemetryObject =
      objectFrom(std::get<QString>(encodedTelemetry));
    CHECK_FALSE(telemetryObject.contains(QStringLiteral("requestId")));
    CHECK(telemetryObject.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("telemetry"))
            .toObject()
            .value(QStringLiteral("playStatus"))
            .toString() == QStringLiteral("playing"));

    const auto expectTelemetryFailure = [&telemetry] {
        CHECK(encodeFailureCode(encodeClientMessage(telemetry)) ==
              ProtocolFailureCode::MalformedMessage);
    };
    telemetry.telemetry.sequence = 0;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    telemetry.telemetry.sequence = MaxUInt32 + 1;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    telemetry.telemetry.exScore = MaxScoreCounter + 1;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    telemetry.telemetry.progressPermille = 1001;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    telemetry.telemetry.gauge.valueMilli = 100'001;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    telemetry.telemetry.judgements.perfect = MaxScoreCounter + 1;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    ++telemetry.telemetry.exScore;
    expectTelemetryFailure();
    telemetry.telemetry = validTelemetry();
    ++telemetry.telemetry.badPoorCount;
    expectTelemetryFailure();

    auto result = RoundResultSubmit{
        .requestId = QStringLiteral("result-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 2,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .result = validFinalResult(),
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(result)));
    ++result.result.exScore;
    CHECK(encodeFailureCode(encodeClientMessage(result)) ==
          ProtocolFailureCode::MalformedMessage);
    result.result = validFinalResult();
    ++result.result.badPoorCount;
    CHECK(encodeFailureCode(encodeClientMessage(result)) ==
          ProtocolFailureCode::MalformedMessage);

    auto abandon = RoundAbandon{
        .requestId = QStringLiteral("abandon-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 2,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .reason = DnfReason::ResultUnavailable,
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(abandon)));
    abandon.reason = DnfReason::Left;
    CHECK(encodeFailureCode(encodeClientMessage(abandon)) ==
          ProtocolFailureCode::MalformedMessage);

    auto loaded = RoundLoadResult{
        .requestId = QStringLiteral("load-phase3"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 2,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .selectionRevision = 1,
        .availabilityRevision = 1,
        .inventoryRevision = 1,
        .ok = true,
        .chartLengthMs = 0,
    };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(loaded)));
    loaded.chartLengthMs = MaxChartLengthMs;
    CHECK(std::holds_alternative<QString>(encodeClientMessage(loaded)));
    loaded.chartLengthMs = MaxChartLengthMs + 1;
    CHECK(encodeFailureCode(encodeClientMessage(loaded)) ==
          ProtocolFailureCode::MalformedMessage);
    loaded.chartLengthMs.reset();
    CHECK(std::holds_alternative<QString>(encodeClientMessage(loaded)));
}

TEST_CASE(
  "ArenaProtocol maps every competition gauge clear and abandon literal",
  "[arena][protocol][phase3]")
{
    using namespace arena;
    auto telemetry = RoundTelemetry{
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .telemetry = validTelemetry(),
    };
    const std::pair<GaugeType, QStringView> gaugeTypes[]{
        { GaugeType::Fc, u"fc" },     { GaugeType::ExHard, u"exhard" },
        { GaugeType::Hard, u"hard" }, { GaugeType::Normal, u"normal" },
        { GaugeType::Easy, u"easy" }, { GaugeType::AssistEasy, u"aeasy" },
    };
    for (const auto& [type, spelling] : gaugeTypes) {
        telemetry.telemetry.gauge.type = type;
        const auto encoded = encodeClientMessage(telemetry);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded))
                .value(QStringLiteral("data"))
                .toObject()
                .value(QStringLiteral("telemetry"))
                .toObject()
                .value(QStringLiteral("gauge"))
                .toObject()
                .value(QStringLiteral("type"))
                .toString() == spelling);
    }

    auto submit = RoundResultSubmit{
        .requestId = QStringLiteral("result-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
        .result = validFinalResult(),
    };
    const std::pair<ClearType, QStringView> clearTypes[]{
        { ClearType::Max, u"max" },       { ClearType::Perfect, u"perfect" },
        { ClearType::FullCombo, u"fc" },  { ClearType::ExHard, u"exhard" },
        { ClearType::Hard, u"hard" },     { ClearType::Normal, u"normal" },
        { ClearType::Easy, u"easy" },     { ClearType::AssistEasy, u"aeasy" },
        { ClearType::Failed, u"failed" },
    };
    for (const auto& [type, spelling] : clearTypes) {
        submit.result.clearType = type;
        const auto encoded = encodeClientMessage(submit);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded))
                .value(QStringLiteral("data"))
                .toObject()
                .value(QStringLiteral("result"))
                .toObject()
                .value(QStringLiteral("clearType"))
                .toString() == spelling);
    }

    auto abandon = RoundAbandon{
        .requestId = QStringLiteral("abandon-1"),
        .roomId = QStringLiteral("room-1"),
        .roomGeneration = 1,
        .connectionGeneration = 1,
        .roundId = QStringLiteral("round-1"),
        .launchAttemptId = QStringLiteral("attempt-1"),
    };
    for (const auto& [reason, spelling] :
         { std::pair{ DnfReason::Aborted, QStringView{ u"aborted" } },
           std::pair{ DnfReason::ResultUnavailable,
                      QStringView{ u"result_unavailable" } } }) {
        abandon.reason = reason;
        const auto encoded = encodeClientMessage(abandon);
        REQUIRE(std::holds_alternative<QString>(encoded));
        CHECK(objectFrom(std::get<QString>(encoded))
                .value(QStringLiteral("data"))
                .toObject()
                .value(QStringLiteral("reason"))
                .toString() == spelling);
    }
}

TEST_CASE("ArenaProtocol encodes anonymous, ticket, and resume hellos",
          "[arena][protocol]")
{
    using namespace arena;

    const auto anonymous = encodeClientMessage(ClientHello{
      .clientVersion = QStringLiteral("2026.7.10"),
    });
    REQUIRE(std::holds_alternative<QString>(anonymous));
    CHECK(
      objectFrom(std::get<QString>(anonymous)) ==
      objectFrom(QStringLiteral(
        R"({"type":"client_hello","data":{"protocolMajor":1,"protocolMinor":0,"clientVersion":"2026.7.10","capabilities":["rooms-v1","rounds-v1","competition-v1"]}})")));

    const auto ticket = encodeClientMessage(ClientHello{
      .clientVersion = QStringLiteral("2026.7.10"),
      .ticket = QStringLiteral("header.payload.signature"),
    });
    REQUIRE(std::holds_alternative<QString>(ticket));
    CHECK(
      objectFrom(std::get<QString>(ticket)) ==
      objectFrom(QStringLiteral(
        R"({"type":"client_hello","data":{"protocolMajor":1,"protocolMinor":0,"clientVersion":"2026.7.10","capabilities":["rooms-v1","rounds-v1","competition-v1"],"ticket":"header.payload.signature"}})")));

    const auto resume = encodeClientMessage(ClientHello{
      .clientVersion = QStringLiteral("2026.7.10"),
      .capabilities = { QStringLiteral("rooms-v1"),
                        QStringLiteral("rounds-v1"),
                        QStringLiteral("competition-v1"),
                        QStringLiteral("future-extension") },
      .ticket = QStringLiteral("header.payload.signature"),
      .resume =
        ResumeRequest{ .roomId = QStringLiteral("room-123"),
                       .seatToken = QStringLiteral("resume_token-123") },
    });
    REQUIRE(std::holds_alternative<QString>(resume));
    CHECK(
      objectFrom(std::get<QString>(resume)) ==
      objectFrom(QStringLiteral(
        R"({"type":"client_hello","data":{"protocolMajor":1,"protocolMinor":0,"clientVersion":"2026.7.10","capabilities":["rooms-v1","rounds-v1","competition-v1","future-extension"],"ticket":"header.payload.signature","resume":{"roomId":"room-123","seatToken":"resume_token-123"}}})")));

    const auto legacy = encodeClientMessage(ClientHello{
      .protocolMinor = ProtocolMinor,
      .clientVersion = QStringLiteral("2026.7.10"),
      .capabilities = { QStringLiteral("rooms-v1") },
    });
    REQUIRE(std::holds_alternative<QString>(legacy));
    CHECK(
      objectFrom(std::get<QString>(legacy)) ==
      objectFrom(QStringLiteral(
        R"({"type":"client_hello","data":{"protocolMajor":1,"protocolMinor":0,"clientVersion":"2026.7.10","capabilities":["rooms-v1"]}})")));
}

TEST_CASE(
  "ArenaProtocol enforces protocol 1.0 capability dependencies and ceilings",
  "[arena][protocol][phase3]")
{
    using namespace arena;

    STATIC_REQUIRE(ProtocolMajor == 1);
    STATIC_REQUIRE(ProtocolMinor == 0);
    STATIC_REQUIRE(RoomCapacity == 32);
    CHECK(QString::fromLatin1(CompetitionCapability) ==
          QStringLiteral("competition-v1"));

    const auto roundsHello = encodeClientMessage(ClientHello{
      .protocolMinor = ProtocolMinor,
      .clientVersion = QStringLiteral("2026.7.10"),
      .capabilities = { QStringLiteral("rooms-v1"),
                        QStringLiteral("rounds-v1") },
    });
    REQUIRE(std::holds_alternative<QString>(roundsHello));
    CHECK(objectFrom(std::get<QString>(roundsHello))
            .value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("protocolMinor"))
            .toInt() == ProtocolMinor);

    const auto invalidHello = [](int minor, QStringList capabilities) {
        return encodeFailureCode(encodeClientMessage(ClientHello{
          .protocolMinor = minor,
          .clientVersion = QStringLiteral("2026.7.10"),
          .capabilities = std::move(capabilities),
        }));
    };
    CHECK_FALSE(invalidHello(
      0, { QStringLiteral("rooms-v1"), QStringLiteral("rounds-v1") }));
    CHECK(invalidHello(1,
                       { QStringLiteral("rooms-v1"),
                         QStringLiteral("rounds-v1"),
                         QStringLiteral("competition-v1") }) ==
          ProtocolFailureCode::ProtocolIncompatible);
    CHECK(
      invalidHello(
        0, { QStringLiteral("rooms-v1"), QStringLiteral("competition-v1") }) ==
      ProtocolFailureCode::MalformedMessage);
    CHECK(invalidHello(0, { QStringLiteral("rounds-v1") }) ==
          ProtocolFailureCode::CapabilityRequired);

    const auto serverHello = [](int minor, QJsonArray capabilities) {
        return decodeServerMessage(envelope(
          QStringLiteral("server_hello"),
          { { QStringLiteral("protocolMajor"), 1 },
            { QStringLiteral("protocolMinor"), minor },
            { QStringLiteral("capabilities"), std::move(capabilities) },
            { QStringLiteral("resume"),
              QJsonObject{ { QStringLiteral("status"),
                             QStringLiteral("not_requested") } } } }));
    };
    CHECK(messageAs<ServerHello>(serverHello(
            0, QJsonArray{ QStringLiteral("rooms-v1") })) != nullptr);
    CHECK(messageAs<ServerHello>(
            serverHello(0,
                        QJsonArray{ QStringLiteral("rooms-v1"),
                                    QStringLiteral("rounds-v1") })) != nullptr);
    CHECK(messageAs<ServerHello>(serverHello(
            0,
            QJsonArray{ QStringLiteral("rooms-v1"),
                        QStringLiteral("rounds-v1"),
                        QStringLiteral("competition-v1") })) != nullptr);
    CHECK(messageAs<ServerHello>(serverHello(
            0, QJsonArray{ QStringLiteral("rooms-v1") })) != nullptr);

    CHECK(failureCode(
            serverHello(1,
                        QJsonArray{ QStringLiteral("rooms-v1"),
                                    QStringLiteral("rounds-v1"),
                                    QStringLiteral("competition-v1") })) ==
          ProtocolFailureCode::ProtocolIncompatible);
    for (auto result : {
           serverHello(0,
                       QJsonArray{ QStringLiteral("rooms-v1"),
                                   QStringLiteral("competition-v1") }),
           serverHello(0,
                       QJsonArray{ QStringLiteral("rounds-v1"),
                                   QStringLiteral("rooms-v1") }),
         }) {
        CHECK(failureCode(result) == ProtocolFailureCode::MalformedMessage);
    }

    const auto failedResume = decodeServerMessage(
      envelope(QStringLiteral("server_hello"),
               { { QStringLiteral("protocolMajor"), 1 },
                 { QStringLiteral("protocolMinor"), 0 },
                 { QStringLiteral("capabilities"),
                   QJsonArray{ QStringLiteral("rooms-v1"),
                               QStringLiteral("rounds-v1") } },
                 { QStringLiteral("resume"),
                   QJsonObject{
                     { QStringLiteral("status"), QStringLiteral("failed") },
                     { QStringLiteral("code"),
                       QStringLiteral("competition_capability_required") },
                     { QStringLiteral("displayMessageKey"),
                       QStringLiteral(
                         "arena.error.competitionCapabilityRequired") } } } }));
    const auto* hello = messageAs<ServerHello>(failedResume);
    REQUIRE(hello != nullptr);
    const auto* failed = std::get_if<ResumeFailed>(&hello->resume);
    REQUIRE(failed != nullptr);
    CHECK(failed->code == ResumeFailureCode::CompetitionCapabilityRequired);
}

TEST_CASE("ArenaProtocol decodes strict competition standing and result unions",
          "[arena][protocol][phase3]")
{
    using namespace arena;
    const auto fixture = loadPhase3ProtocolFixture();
    const auto standingsGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_standings");
    const auto finalizedGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_finalized");

    auto decodeStandingsEntry = [&standingsGolden](QJsonObject entry) {
        auto message = standingsGolden;
        auto data = message.value(QStringLiteral("data")).toObject();
        data.insert(QStringLiteral("entries"), QJsonArray{ std::move(entry) });
        message.insert(QStringLiteral("data"), data);
        return decodeServerMessage(compact(message));
    };

    const auto canonicalEntry = standingsGolden.value(QStringLiteral("data"))
                                  .toObject()
                                  .value(QStringLiteral("entries"))
                                  .toArray()
                                  .at(0)
                                  .toObject();
    CHECK(messageAs<LiveStandingsSnapshot>(
            decodeStandingsEntry(canonicalEntry)) != nullptr);

    auto loading = canonicalEntry;
    loading.insert(QStringLiteral("competitionState"),
                   QStringLiteral("loading"));
    CHECK(messageAs<LiveStandingsSnapshot>(decodeStandingsEntry(loading)) !=
          nullptr);

    auto noData = canonicalEntry;
    noData.insert(QStringLiteral("rank"), QJsonValue::Null);
    noData.insert(QStringLiteral("telemetry"), QJsonValue::Null);
    CHECK(messageAs<LiveStandingsSnapshot>(decodeStandingsEntry(noData)) !=
          nullptr);

    const auto finalEntry = finalizedGolden.value(QStringLiteral("data"))
                              .toObject()
                              .value(QStringLiteral("result"))
                              .toObject()
                              .value(QStringLiteral("entries"))
                              .toArray()
                              .at(0)
                              .toObject();
    auto finished = QJsonObject{
        { QStringLiteral("memberId"),
          canonicalEntry.value(QStringLiteral("memberId")) },
        { QStringLiteral("connectionStatus"), QStringLiteral("connected") },
        { QStringLiteral("competitionState"), QStringLiteral("finished") },
        { QStringLiteral("rank"), 1 },
        { QStringLiteral("result"),
          finalEntry.value(QStringLiteral("result")) },
    };
    CHECK(messageAs<LiveStandingsSnapshot>(decodeStandingsEntry(finished)) !=
          nullptr);

    auto dnf = QJsonObject{
        { QStringLiteral("memberId"),
          canonicalEntry.value(QStringLiteral("memberId")) },
        { QStringLiteral("connectionStatus"), QStringLiteral("reserved") },
        { QStringLiteral("competitionState"), QStringLiteral("dnf") },
        { QStringLiteral("rank"), QJsonValue::Null },
        { QStringLiteral("dnfReason"), QStringLiteral("play_deadline") },
    };
    CHECK(messageAs<LiveStandingsSnapshot>(decodeStandingsEntry(dnf)) !=
          nullptr);
    for (const auto& reason : { QStringLiteral("aborted"),
                                QStringLiteral("result_unavailable"),
                                QStringLiteral("left"),
                                QStringLiteral("kicked"),
                                QStringLiteral("grace_expired"),
                                QStringLiteral("play_deadline") }) {
        dnf.insert(QStringLiteral("dnfReason"), reason);
        CAPTURE(reason);
        CHECK(messageAs<LiveStandingsSnapshot>(decodeStandingsEntry(dnf)) !=
              nullptr);
    }

    auto activeRankWithoutData = noData;
    activeRankWithoutData.insert(QStringLiteral("rank"), 1);
    CHECK(failureCode(decodeStandingsEntry(activeRankWithoutData)) ==
          ProtocolFailureCode::MalformedMessage);
    auto activeDataWithoutRank = canonicalEntry;
    activeDataWithoutRank.insert(QStringLiteral("rank"), QJsonValue::Null);
    CHECK(failureCode(decodeStandingsEntry(activeDataWithoutRank)) ==
          ProtocolFailureCode::MalformedMessage);
    auto invalidFinished = finished;
    invalidFinished.insert(QStringLiteral("rank"), 0);
    CHECK(failureCode(decodeStandingsEntry(invalidFinished)) ==
          ProtocolFailureCode::MalformedMessage);
    invalidFinished = finished;
    invalidFinished.insert(QStringLiteral("telemetry"), QJsonValue::Null);
    CHECK(failureCode(decodeStandingsEntry(invalidFinished)) ==
          ProtocolFailureCode::MalformedMessage);
    auto invalidDnf = dnf;
    invalidDnf.insert(QStringLiteral("rank"), 1);
    CHECK(failureCode(decodeStandingsEntry(invalidDnf)) ==
          ProtocolFailureCode::MalformedMessage);

    for (const auto key :
         { QStringLiteral("judgements"), QStringLiteral("gauge") }) {
        auto invalid = canonicalEntry;
        auto telemetry = invalid.value(QStringLiteral("telemetry")).toObject();
        auto nested = telemetry.value(key).toObject();
        nested.insert(QStringLiteral("unexpected"), true);
        telemetry.insert(key, nested);
        invalid.insert(QStringLiteral("telemetry"), telemetry);
        CAPTURE(key);
        CHECK(failureCode(decodeStandingsEntry(invalid)) ==
              ProtocolFailureCode::MalformedMessage);
    }
    auto invalidTelemetry = canonicalEntry;
    auto telemetry =
      invalidTelemetry.value(QStringLiteral("telemetry")).toObject();
    telemetry.insert(QStringLiteral("exScore"), 101);
    invalidTelemetry.insert(QStringLiteral("telemetry"), telemetry);
    CHECK(failureCode(decodeStandingsEntry(invalidTelemetry)) ==
          ProtocolFailureCode::MalformedMessage);

    auto duplicateStandings = standingsGolden;
    auto duplicateData =
      duplicateStandings.value(QStringLiteral("data")).toObject();
    duplicateData.insert(QStringLiteral("entries"),
                         QJsonArray{ canonicalEntry, canonicalEntry });
    duplicateStandings.insert(QStringLiteral("data"), duplicateData);
    CHECK(failureCode(decodeServerMessage(compact(duplicateStandings))) ==
          ProtocolFailureCode::MalformedMessage);

    auto invalidFinalized = finalizedGolden;
    auto finalizedData =
      invalidFinalized.value(QStringLiteral("data")).toObject();
    auto result = finalizedData.value(QStringLiteral("result")).toObject();
    result.insert(QStringLiteral("participantCount"), 2);
    finalizedData.insert(QStringLiteral("result"), result);
    invalidFinalized.insert(QStringLiteral("data"), finalizedData);
    CHECK(failureCode(decodeServerMessage(compact(invalidFinalized))) ==
          ProtocolFailureCode::MalformedMessage);

    invalidFinalized = finalizedGolden;
    finalizedData = invalidFinalized.value(QStringLiteral("data")).toObject();
    result = finalizedData.value(QStringLiteral("result")).toObject();
    result.insert(QStringLiteral("winnerMemberIds"), QJsonArray{});
    finalizedData.insert(QStringLiteral("result"), result);
    invalidFinalized.insert(QStringLiteral("data"), finalizedData);
    CHECK(failureCode(decodeServerMessage(compact(invalidFinalized))) ==
          ProtocolFailureCode::MalformedMessage);

    auto dnfFinalized = finalizedGolden;
    finalizedData = dnfFinalized.value(QStringLiteral("data")).toObject();
    result = finalizedData.value(QStringLiteral("result")).toObject();
    auto entries = result.value(QStringLiteral("entries")).toArray();
    auto finalDnf = entries.at(0).toObject();
    finalDnf.insert(QStringLiteral("competitionState"), QStringLiteral("dnf"));
    finalDnf.insert(QStringLiteral("rank"), QJsonValue::Null);
    finalDnf.remove(QStringLiteral("result"));
    finalDnf.insert(QStringLiteral("dnfReason"),
                    QStringLiteral("result_unavailable"));
    entries[0] = finalDnf;
    result.insert(QStringLiteral("entries"), entries);
    result.insert(QStringLiteral("winnerMemberIds"), QJsonArray{});
    finalizedData.insert(QStringLiteral("result"), result);
    dnfFinalized.insert(QStringLiteral("data"), finalizedData);
    CHECK(messageAs<RoundFinalized>(
            decodeServerMessage(compact(dnfFinalized))) != nullptr);

    invalidFinalized = finalizedGolden;
    finalizedData = invalidFinalized.value(QStringLiteral("data")).toObject();
    finalizedData.insert(QStringLiteral("roundId"),
                         QStringLiteral("different-round"));
    invalidFinalized.insert(QStringLiteral("data"), finalizedData);
    CHECK(failureCode(decodeServerMessage(compact(invalidFinalized))) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE(
  "ArenaProtocol decodes competition identities deadlines and room state",
  "[arena][protocol][phase3]")
{
    using namespace arena;
    const auto fixture = loadPhase3ProtocolFixture();

    const auto loadingGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_loading_started");
    const auto loadingDecoded = decodeServerMessage(compact(loadingGolden));
    const auto* loading = messageAs<RoundLoadingStarted>(loadingDecoded);
    REQUIRE(loading != nullptr);
    REQUIRE(loading->round.participants.size() == 1);
    REQUIRE(loading->round.participants.front().identity.has_value());
    CHECK(loading->round.participants.front().identity->displayName ==
          QStringLiteral("Phase 3 Player"));
    CHECK_FALSE(loading->round.playDeadlineAtServerMs.has_value());

    auto scheduledRound = loadingGolden;
    auto scheduledRoundData =
      scheduledRound.value(QStringLiteral("data")).toObject();
    auto scheduledRoundValue =
      scheduledRoundData.value(QStringLiteral("round")).toObject();
    scheduledRoundValue.insert(QStringLiteral("stage"),
                               QStringLiteral("scheduled"));
    scheduledRoundValue.insert(QStringLiteral("playDeadlineAtServerMs"),
                               400'000);
    scheduledRoundData.insert(QStringLiteral("round"), scheduledRoundValue);
    scheduledRound.insert(QStringLiteral("data"), scheduledRoundData);
    const auto scheduledRoundDecoded =
      decodeServerMessage(compact(scheduledRound));
    const auto* scheduledFrozen =
      messageAs<RoundLoadingStarted>(scheduledRoundDecoded);
    REQUIRE(scheduledFrozen != nullptr);
    CHECK(scheduledFrozen->round.playDeadlineAtServerMs == 400'000);

    auto invalidLoading = loadingGolden;
    auto loadingData = invalidLoading.value(QStringLiteral("data")).toObject();
    auto round = loadingData.value(QStringLiteral("round")).toObject();
    round.insert(QStringLiteral("playDeadlineAtServerMs"), 400'000);
    loadingData.insert(QStringLiteral("round"), round);
    invalidLoading.insert(QStringLiteral("data"), loadingData);
    CHECK(failureCode(decodeServerMessage(compact(invalidLoading))) ==
          ProtocolFailureCode::MalformedMessage);

    const auto scheduleGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_start_scheduled");
    const auto scheduleDecoded = decodeServerMessage(compact(scheduleGolden));
    const auto* schedule = messageAs<RoundStartScheduled>(scheduleDecoded);
    REQUIRE(schedule != nullptr);
    CHECK(schedule->playDeadlineAtServerMs == 400'000);

    auto invalidSchedule = scheduleGolden;
    auto scheduleData =
      invalidSchedule.value(QStringLiteral("data")).toObject();
    scheduleData.insert(QStringLiteral("playDeadlineAtServerMs"), 99'999);
    invalidSchedule.insert(QStringLiteral("data"), scheduleData);
    CHECK(failureCode(decodeServerMessage(compact(invalidSchedule))) ==
          ProtocolFailureCode::MalformedMessage);

    const auto startedGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_started");
    const auto startedDecoded = decodeServerMessage(compact(startedGolden));
    const auto* started = messageAs<RoundStarted>(startedDecoded);
    REQUIRE(started != nullptr);
    CHECK(started->playDeadlineAtServerMs == 400'000);

    auto terminalGolden =
      fixtureMessage(fixture, u"serverMessages", u"round_terminal_accepted");
    const auto finishedTerminalDecoded =
      decodeServerMessage(compact(terminalGolden));
    const auto* finishedTerminal =
      messageAs<RoundTerminalAccepted>(finishedTerminalDecoded);
    REQUIRE(finishedTerminal != nullptr);
    CHECK(finishedTerminal->terminal == TerminalKind::Finished);
    auto terminalData = terminalGolden.value(QStringLiteral("data")).toObject();
    terminalData.insert(QStringLiteral("terminal"), QStringLiteral("dnf"));
    terminalGolden.insert(QStringLiteral("data"), terminalData);
    const auto dnfTerminalDecoded =
      decodeServerMessage(compact(terminalGolden));
    const auto* dnfTerminal =
      messageAs<RoundTerminalAccepted>(dnfTerminalDecoded);
    REQUIRE(dnfTerminal != nullptr);
    CHECK(dnfTerminal->terminal == TerminalKind::Dnf);
    terminalData.insert(QStringLiteral("terminal"), QStringLiteral("future"));
    terminalGolden.insert(QStringLiteral("data"), terminalData);
    CHECK(failureCode(decodeServerMessage(compact(terminalGolden))) ==
          ProtocolFailureCode::MalformedMessage);

    const auto roomGolden =
      fixtureMessage(fixture, u"serverMessages", u"room_snapshot");
    const auto roomDecoded = decodeServerMessage(compact(roomGolden));
    const auto* room = messageAs<RoomSnapshotEvent>(roomDecoded);
    REQUIRE(room != nullptr);
    CHECK(room->room.competitionShape);
    CHECK_FALSE(room->room.liveStandings.has_value());
    CHECK_FALSE(room->room.lastRoundResult.has_value());

    auto invalidRoom = roomGolden;
    auto roomData = invalidRoom.value(QStringLiteral("data")).toObject();
    roomData.remove(QStringLiteral("lastRoundResult"));
    invalidRoom.insert(QStringLiteral("data"), roomData);
    CHECK(failureCode(decodeServerMessage(compact(invalidRoom))) ==
          ProtocolFailureCode::MalformedMessage);
    roomData = roomGolden.value(QStringLiteral("data")).toObject();
    roomData.insert(QStringLiteral("phase"), QStringLiteral("playing"));
    invalidRoom.insert(QStringLiteral("data"), roomData);
    CHECK(failureCode(decodeServerMessage(compact(invalidRoom))) ==
          ProtocolFailureCode::MalformedMessage);

    roomData = roomGolden.value(QStringLiteral("data")).toObject();
    auto members = roomData.value(QStringLiteral("members")).toArray();
    auto member = members.at(0).toObject();
    member.insert(QStringLiteral("lobbyWins"), static_cast<double>(MaxUInt32));
    members[0] = member;
    roomData.insert(QStringLiteral("members"), members);
    invalidRoom.insert(QStringLiteral("data"), roomData);
    CHECK(messageAs<RoomSnapshotEvent>(
            decodeServerMessage(compact(invalidRoom))) != nullptr);
    member.insert(QStringLiteral("lobbyWins"),
                  static_cast<double>(MaxUInt32) + 1.0);
    members[0] = member;
    roomData.insert(QStringLiteral("members"), members);
    invalidRoom.insert(QStringLiteral("data"), roomData);
    CHECK(failureCode(decodeServerMessage(compact(invalidRoom))) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol decodes server hello and directory snapshot",
          "[arena][protocol]")
{
    using namespace arena;

    const auto hello = decodeServerMessage(QStringLiteral(
      R"({"type":"server_hello","data":{"protocolMajor":1,"protocolMinor":0,"capabilities":["rooms-v1"],"resume":{"status":"not_requested"}}})"));
    REQUIRE(decodedMessage(hello) != nullptr);
    const auto* serverHello = std::get_if<ServerHello>(decodedMessage(hello));
    REQUIRE(serverHello != nullptr);
    CHECK(serverHello->protocolMajor == 1);
    CHECK(serverHello->protocolMinor == 0);
    CHECK(serverHello->capabilities ==
          QStringList{ QStringLiteral("rooms-v1") });
    CHECK_FALSE(serverHello->identity.has_value());

    const auto currentHello = decodeServerMessage(QStringLiteral(
      R"({"type":"server_hello","data":{"protocolMajor":1,"protocolMinor":0,"capabilities":["rooms-v1","rounds-v1"],"resume":{"status":"not_requested"}}})"));
    const auto* current = messageAs<ServerHello>(currentHello);
    REQUIRE(current != nullptr);
    CHECK(current->protocolMinor == ProtocolMinor);
    CHECK(current->capabilities == QStringList{ QStringLiteral("rooms-v1"),
                                                QStringLiteral("rounds-v1") });

    const auto directory = decodeServerMessage(QStringLiteral(
      R"({"type":"directory_snapshot","data":{"revision":4,"rooms":[{"roomId":"room-123","name":"Arena room","phase":"selecting","hasPassword":true,"connectedCount":1,"reservedCount":0,"maxCount":32,"members":[{"displayName":"Alice","avatarUrl":null,"connected":true}]}]}})"));
    REQUIRE(decodedMessage(directory) != nullptr);
    const auto* snapshot =
      std::get_if<DirectorySnapshot>(decodedMessage(directory));
    REQUIRE(snapshot != nullptr);
    REQUIRE(snapshot->rooms.size() == 1);
    CHECK(snapshot->revision == 4);
    CHECK(snapshot->rooms.front().roomId == QStringLiteral("room-123"));
    CHECK(snapshot->rooms.front().hasPassword);
}

TEST_CASE(
  "ArenaProtocol rejects malformed unknown incompatible and oversized frames",
  "[arena][protocol]")
{
    using namespace arena;

    CHECK(failureCode(decodeServerMessage(QStringLiteral("{"))) ==
          ProtocolFailureCode::MalformedMessage);
    CHECK(failureCode(decodeServerMessage(
            QStringLiteral(R"({"type":"future_event","data":{}})"))) ==
          ProtocolFailureCode::MalformedMessage);
    CHECK(
      failureCode(decodeServerMessage(QStringLiteral(
        R"({"type":"directory_snapshot","extra":true,"data":{"revision":0,"rooms":[]}})"))) ==
      ProtocolFailureCode::MalformedMessage);
    CHECK(
      failureCode(decodeServerMessage(QStringLiteral(
        R"({"type":"server_hello","data":{"protocolMajor":2,"protocolMinor":0,"capabilities":["rooms-v1"],"resume":{"status":"not_requested"}}})"))) ==
      ProtocolFailureCode::ProtocolIncompatible);
    CHECK(
      failureCode(decodeServerMessage(QStringLiteral(
        R"({"type":"server_hello","data":{"protocolMajor":1,"protocolMinor":3,"capabilities":["rooms-v1"],"resume":{"status":"not_requested"}}})"))) ==
      ProtocolFailureCode::ProtocolIncompatible);
    CHECK(
      failureCode(decodeServerMessage(QStringLiteral(
        R"({"type":"server_hello","data":{"protocolMajor":1,"protocolMinor":0,"capabilities":["other"],"resume":{"status":"not_requested"}}})"))) ==
      ProtocolFailureCode::CapabilityRequired);

    QString oversized(static_cast<qsizetype>(MaxServerMessageBytes / 2 + 1),
                      QChar(0x00e9));
    CHECK(oversized.toUtf8().size() > MaxServerMessageBytes);
    CHECK(failureCode(decodeServerMessage(oversized)) ==
          ProtocolFailureCode::FrameTooLarge);
}

TEST_CASE("ArenaProtocol encodes every Phase 1 client command",
          "[arena][protocol]")
{
    using namespace arena;

    const QVector<ClientMessage> messages{
        DirectorySubscribe{},
        RoomCreate{ .requestId = QStringLiteral("create-1"),
                    .name = QStringLiteral("  Arena room  "),
                    .password = QStringLiteral(" keep spaces ") },
        RoomJoin{ .requestId = QStringLiteral("join-1"),
                  .roomId = QStringLiteral("room-123"),
                  .password = QStringLiteral("password") },
        RoomLeave{ .requestId = QStringLiteral("leave-1"),
                   .roomId = QStringLiteral("room-123"),
                   .roomGeneration = 3,
                   .connectionGeneration = 2 },
        RoomKick{ .requestId = QStringLiteral("kick-1"),
                  .roomId = QStringLiteral("room-123"),
                  .roomGeneration = 3,
                  .connectionGeneration = 2,
                  .targetMemberId = QStringLiteral("member-2") },
        ChatSend{ .requestId = QStringLiteral("chat-1"),
                  .roomId = QStringLiteral("room-123"),
                  .roomGeneration = 3,
                  .connectionGeneration = 2,
                  .text = QStringLiteral("  Hello Arena!  ") },
        HeartbeatReply{ .nonce = QStringLiteral("heartbeat-123") },
    };
    const QStringList expectedTypes{
        QStringLiteral("directory_subscribe"), QStringLiteral("room_create"),
        QStringLiteral("room_join"),           QStringLiteral("room_leave"),
        QStringLiteral("room_kick"),           QStringLiteral("chat_send"),
        QStringLiteral("heartbeat_reply")
    };

    for (qsizetype i = 0; i < messages.size(); ++i) {
        const auto encoded = encodeClientMessage(messages[i]);
        REQUIRE(std::holds_alternative<QString>(encoded));
        const auto object = objectFrom(std::get<QString>(encoded));
        CHECK(object.value(QStringLiteral("type")).toString() ==
              expectedTypes[i]);
        CHECK(object.keys().size() == (i >= 1 && i <= 5 ? 3 : 2));
    }

    const auto create =
      objectFrom(std::get<QString>(encodeClientMessage(messages[1])));
    CHECK(create.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("name"))
            .toString() == QStringLiteral("Arena room"));
    CHECK(create.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("password"))
            .toString() == QStringLiteral(" keep spaces "));
    const auto chat =
      objectFrom(std::get<QString>(encodeClientMessage(messages[5])));
    CHECK(chat.value(QStringLiteral("data"))
            .toObject()
            .value(QStringLiteral("text"))
            .toString() == QStringLiteral("Hello Arena!"));
}

TEST_CASE("ArenaProtocol decodes all server hello resume alternatives",
          "[arena][protocol]")
{
    using namespace arena;

    const auto anonymous = decodeServerMessage(
      envelope(QStringLiteral("server_hello"),
               { { QStringLiteral("protocolMajor"), 1 },
                 { QStringLiteral("protocolMinor"), 0 },
                 { QStringLiteral("capabilities"),
                   QJsonArray{ QStringLiteral("rooms-v1") } },
                 { QStringLiteral("resume"),
                   QJsonObject{ { QStringLiteral("status"),
                                  QStringLiteral("not_requested") } } } }));
    REQUIRE(messageAs<ServerHello>(anonymous) != nullptr);
    CHECK(std::holds_alternative<ResumeNotRequested>(
      messageAs<ServerHello>(anonymous)->resume));

    QJsonObject authenticatedData{
        { QStringLiteral("protocolMajor"), 1 },
        { QStringLiteral("protocolMinor"), 0 },
        { QStringLiteral("capabilities"),
          QJsonArray{ QStringLiteral("rooms-v1") } },
        { QStringLiteral("identity"), identityObject() },
        { QStringLiteral("resume"),
          QJsonObject{
            { QStringLiteral("status"), QStringLiteral("not_requested") } } },
    };
    const auto authenticated = decodeServerMessage(
      envelope(QStringLiteral("server_hello"), authenticatedData));
    REQUIRE(messageAs<ServerHello>(authenticated) != nullptr);
    REQUIRE(messageAs<ServerHello>(authenticated)->identity.has_value());
    CHECK(messageAs<ServerHello>(authenticated)->identity->displayName ==
          QStringLiteral("Alice"));

    authenticatedData.insert(
      QStringLiteral("resume"),
      QJsonObject{ { QStringLiteral("status"), QStringLiteral("succeeded") },
                   { QStringLiteral("room"), roomSnapshotObject() } });
    const auto succeeded = decodeServerMessage(
      envelope(QStringLiteral("server_hello"), authenticatedData));
    REQUIRE(messageAs<ServerHello>(succeeded) != nullptr);
    const auto* resumed =
      std::get_if<ResumeSucceeded>(&messageAs<ServerHello>(succeeded)->resume);
    REQUIRE(resumed != nullptr);
    CHECK(resumed->room.self.resumeToken == QStringLiteral("resume_token-123"));

    authenticatedData.insert(
      QStringLiteral("resume"),
      QJsonObject{
        { QStringLiteral("status"), QStringLiteral("failed") },
        { QStringLiteral("code"), QStringLiteral("room_resume_failed") },
        { QStringLiteral("displayMessageKey"),
          QStringLiteral("arena.error.resumeFailed") } });
    const auto failed = decodeServerMessage(
      envelope(QStringLiteral("server_hello"), authenticatedData));
    REQUIRE(messageAs<ServerHello>(failed) != nullptr);
    CHECK(std::holds_alternative<ResumeFailed>(
      messageAs<ServerHello>(failed)->resume));
}

TEST_CASE("ArenaProtocol decodes every Phase 1 server event",
          "[arena][protocol]")
{
    using namespace arena;

    const QVector<std::pair<QString, std::size_t>> messages{
        { envelope(QStringLiteral("fatal_error"),
                   { { QStringLiteral("code"),
                       QStringLiteral("server_shutting_down") },
                     { QStringLiteral("displayMessageKey"),
                       QStringLiteral("arena.error.serverShuttingDown") } }),
          ServerMessage{ FatalError{} }.index() },
        { envelope(
            QStringLiteral("directory_snapshot"),
            { { QStringLiteral("revision"), 4 },
              { QStringLiteral("rooms"), QJsonArray{ roomSummaryObject() } } }),
          ServerMessage{ DirectorySnapshot{} }.index() },
        { envelope(QStringLiteral("room_directory_updated"),
                   { { QStringLiteral("revision"), 5 },
                     { QStringLiteral("upserts"), QJsonArray{} },
                     { QStringLiteral("removedRoomIds"),
                       QJsonArray{ QStringLiteral("room-removed") } } }),
          ServerMessage{ RoomDirectoryUpdated{} }.index() },
        { envelope(QStringLiteral("room_snapshot"),
                   roomSnapshotObject(),
                   QStringLiteral("join-1")),
          ServerMessage{ RoomSnapshotEvent{} }.index() },
        { envelope(QStringLiteral("room_member_joined"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("member"), memberObject() } }),
          ServerMessage{ RoomMemberJoined{} }.index() },
        { envelope(QStringLiteral("room_member_updated"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("member"), memberObject() } }),
          ServerMessage{ RoomMemberUpdated{} }.index() },
        { envelope(QStringLiteral("room_member_left"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("memberId"), QStringLiteral("member-2") },
                     { QStringLiteral("reason"), QStringLiteral("kicked") } }),
          ServerMessage{ RoomMemberLeft{} }.index() },
        { envelope(QStringLiteral("room_owner_changed"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("ownerMemberId"), QJsonValue::Null } }),
          ServerMessage{ RoomOwnerChanged{} }.index() },
        { envelope(QStringLiteral("chat_message"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("message"), chatObject() } }),
          ServerMessage{ ChatMessageEvent{} }.index() },
        { envelope(
            QStringLiteral("server_heartbeat"),
            { { QStringLiteral("nonce"), QStringLiteral("heartbeat-123") },
              { QStringLiteral("sentAtMs"), 1000 } }),
          ServerMessage{ ServerHeartbeat{} }.index() },
        { envelope(QStringLiteral("server_going_away"),
                   { { QStringLiteral("displayMessageKey"),
                       QStringLiteral("arena.serverGoingAway") },
                     { QStringLiteral("retryAfterMs"), 1000 } }),
          ServerMessage{ ServerGoingAway{} }.index() },
        { envelope(QStringLiteral("command_error"),
                   { { QStringLiteral("code"),
                       QStringLiteral("room_password_invalid") },
                     { QStringLiteral("displayMessageKey"),
                       QStringLiteral("arena.error.roomPasswordInvalid") } },
                   QStringLiteral("join-1")),
          ServerMessage{ CommandError{} }.index() },
    };

    for (const auto& [text, expectedIndex] : messages) {
        const auto decoded = decodeServerMessage(text);
        REQUIRE(decodedMessage(decoded) != nullptr);
        CHECK(decodedMessage(decoded)->index() == expectedIndex);
    }

    for (const auto reason : { QStringLiteral("left"),
                               QStringLiteral("kicked"),
                               QStringLiteral("grace_expired") }) {
        const auto decoded = decodeServerMessage(
          envelope(QStringLiteral("room_member_left"),
                   { { QStringLiteral("roomId"), QStringLiteral("room-123") },
                     { QStringLiteral("roomGeneration"), 3 },
                     { QStringLiteral("memberId"), QStringLiteral("member-2") },
                     { QStringLiteral("reason"), reason } }));
        REQUIRE(messageAs<RoomMemberLeft>(decoded) != nullptr);
    }

    const QStringList commandCodes{
        QStringLiteral("auth_required"),
        QStringLiteral("already_in_room"),
        QStringLiteral("not_in_room"),
        QStringLiteral("room_not_found"),
        QStringLiteral("room_password_invalid"),
        QStringLiteral("room_full"),
        QStringLiteral("room_banned"),
        QStringLiteral("room_duplicate_identity"),
        QStringLiteral("room_generation_stale"),
        QStringLiteral("connection_generation_stale"),
        QStringLiteral("permission_denied"),
        QStringLiteral("target_not_found"),
        QStringLiteral("cannot_kick_self"),
        QStringLiteral("chat_empty"),
        QStringLiteral("chat_too_long"),
        QStringLiteral("rate_limited"),
        QStringLiteral("rounds_capability_required"),
        QStringLiteral("competition_capability_required"),
        QStringLiteral("inventory_busy"),
        QStringLiteral("inventory_invalid"),
        QStringLiteral("inventory_stale"),
        QStringLiteral("inventory_capacity_exceeded"),
        QStringLiteral("availability_stale"),
        QStringLiteral("selection_not_common"),
        QStringLiteral("selection_stale"),
        QStringLiteral("ready_not_allowed"),
        QStringLiteral("round_stale"),
        QStringLiteral("launch_stage_stale"),
        QStringLiteral("result_invalid"),
        QStringLiteral("round_already_terminal"),
        QStringLiteral("server_capacity"),
    };
    for (const auto& code : commandCodes) {
        CHECK(messageAs<CommandError>(decodeServerMessage(
                envelope(QStringLiteral("command_error"),
                         { { QStringLiteral("code"), code },
                           { QStringLiteral("displayMessageKey"),
                             QStringLiteral("arena.error.any") } },
                         QStringLiteral("request-1")))) != nullptr);
    }

    const QStringList fatalCodes{ QStringLiteral("malformed_message"),
                                  QStringLiteral("frame_too_large"),
                                  QStringLiteral("unexpected_binary"),
                                  QStringLiteral("hello_required"),
                                  QStringLiteral("hello_repeated"),
                                  QStringLiteral("protocol_incompatible"),
                                  QStringLiteral("capability_required"),
                                  QStringLiteral("invalid_ticket"),
                                  QStringLiteral("ticket_replayed"),
                                  QStringLiteral("server_shutting_down") };
    for (const auto& code : fatalCodes) {
        CHECK(messageAs<FatalError>(decodeServerMessage(envelope(
                QStringLiteral("fatal_error"),
                { { QStringLiteral("code"), code },
                  { QStringLiteral("displayMessageKey"),
                    QStringLiteral("arena.error.any") } }))) != nullptr);
    }
}

TEST_CASE("ArenaProtocol rejects malformed strict envelopes atomically",
          "[arena][protocol]")
{
    using namespace arena;

    const QStringList malformed{
        QStringLiteral("null"),
        QStringLiteral("[]"),
        QStringLiteral(R"({"data":{}})"),
        QStringLiteral(R"({"type":1,"data":{}})"),
        QStringLiteral(R"({"type":"directory_snapshot"})"),
        QStringLiteral(
          R"({"type":"directory_snapshot","requestId":"extra","data":{"revision":0,"rooms":[]}})"),
        QStringLiteral(
          R"({"type":"directory_snapshot","data":{"revision":0,"rooms":[],"extra":true}})"),
        QStringLiteral(
          R"({"type":"directory_snapshot","data":{"revision":"0","rooms":[]}})"),
        QStringLiteral(
          R"({"type":"room_snapshot","data":{"roomId":"room-1"}})"),
        QStringLiteral(
          R"({"type":"fatal_error","data":{"code":"future_error","displayMessageKey":"arena.error.any"}})"),
        QStringLiteral(
          R"({"type":"server_going_away","data":{"displayMessageKey":"wrong"}})"),
        QStringLiteral(
          R"({"type":"server_hello","data":{"protocolMajor":1,"protocolMinor":0,"capabilities":["rooms-v1"],"identity":null,"resume":{"status":"not_requested"}}})"),
    };

    DirectorySnapshot alreadyDecoded{ .revision = 9,
                                      .rooms = { RoomSummary{
                                        .roomId = QStringLiteral("keep") } } };
    for (const auto& text : malformed) {
        const auto result = decodeServerMessage(text);
        CHECK(failureCode(result) == ProtocolFailureCode::MalformedMessage);
        CHECK(alreadyDecoded.revision == 9);
        CHECK(alreadyDecoded.rooms.front().roomId == QStringLiteral("keep"));
    }

    const auto invalidUrl = roomSnapshotObject();
    auto members = invalidUrl.value(QStringLiteral("members")).toArray();
    auto member = members.at(0).toObject();
    auto identity = member.value(QStringLiteral("identity")).toObject();
    identity.insert(QStringLiteral("avatarUrl"), QStringLiteral("not a url"));
    member.insert(QStringLiteral("identity"), identity);
    members[0] = member;
    auto invalidUrlSnapshot = invalidUrl;
    invalidUrlSnapshot.insert(QStringLiteral("members"), members);
    CHECK(
      failureCode(decodeServerMessage(envelope(QStringLiteral("room_snapshot"),
                                               invalidUrlSnapshot,
                                               QStringLiteral("join-1")))) ==
      ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol reports version and capability failures precisely",
          "[arena][protocol]")
{
    using namespace arena;

    auto helloData = [] {
        return QJsonObject{
            { QStringLiteral("protocolMajor"), 1 },
            { QStringLiteral("protocolMinor"), 0 },
            { QStringLiteral("capabilities"),
              QJsonArray{ QStringLiteral("rooms-v1") } },
            { QStringLiteral("resume"),
              QJsonObject{ { QStringLiteral("status"),
                             QStringLiteral("not_requested") } } },
        };
    };

    for (const auto [major, minor] : { std::pair{ 2, 0 }, std::pair{ 1, 3 } }) {
        auto data = helloData();
        data.insert(QStringLiteral("protocolMajor"), major);
        data.insert(QStringLiteral("protocolMinor"), minor);
        data.insert(QStringLiteral("resume"), QStringLiteral("also-bad"));
        CHECK(failureCode(decodeServerMessage(
                envelope(QStringLiteral("server_hello"), data))) ==
              ProtocolFailureCode::ProtocolIncompatible);
    }

    auto missingCapability = helloData();
    missingCapability.insert(QStringLiteral("capabilities"),
                             QJsonArray{ QStringLiteral("future") });
    missingCapability.insert(QStringLiteral("resume"),
                             QStringLiteral("also-bad"));
    CHECK(failureCode(decodeServerMessage(
            envelope(QStringLiteral("server_hello"), missingCapability))) ==
          ProtocolFailureCode::CapabilityRequired);

    for (const auto capabilities : {
           QJsonArray{ QStringLiteral("rooms-v1"), QStringLiteral("rooms-v1") },
           QJsonArray{ QStringLiteral("rooms-v1"), QStringLiteral("future") },
           QJsonArray{ QStringLiteral("rounds-v1"),
                       QStringLiteral("rooms-v1") },
           QJsonArray{ QStringLiteral("rooms-v1"), 1 },
         }) {
        auto data = helloData();
        data.insert(QStringLiteral("capabilities"), capabilities);
        CHECK(failureCode(decodeServerMessage(
                envelope(QStringLiteral("server_hello"), data))) ==
              ProtocolFailureCode::MalformedMessage);
    }

    auto roomsWithRounds = helloData();
    roomsWithRounds.insert(
      QStringLiteral("capabilities"),
      QJsonArray{ QStringLiteral("rooms-v1"), QStringLiteral("rounds-v1") });
    CHECK(messageAs<ServerHello>(decodeServerMessage(envelope(
            QStringLiteral("server_hello"), roomsWithRounds))) != nullptr);

    auto currentRoomsOnly = helloData();
    currentRoomsOnly.insert(QStringLiteral("protocolMinor"), ProtocolMinor);
    CHECK(messageAs<ServerHello>(decodeServerMessage(envelope(
            QStringLiteral("server_hello"), currentRoomsOnly))) != nullptr);

    auto tooMany = ClientHello{ .clientVersion = QStringLiteral("1.0") };
    tooMany.capabilities.clear();
    tooMany.capabilities.push_back(QStringLiteral("rooms-v1"));
    for (int i = 1; i < 17; ++i) {
        tooMany.capabilities.push_back(QStringLiteral("cap-%1").arg(i));
    }
    CHECK(encodeFailureCode(encodeClientMessage(tooMany)) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol enforces Unicode and byte limits", "[arena][protocol]")
{
    using namespace arena;

    const QString emoji = QString::fromUtf8("\xF0\x9F\x92\xA5");
    auto hello = ClientHello{ .clientVersion = emoji.repeated(64) };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(hello)));
    hello.clientVersion.append(emoji);
    CHECK(encodeFailureCode(encodeClientMessage(hello)) ==
          ProtocolFailureCode::MalformedMessage);

    auto room = RoomCreate{ .requestId = QStringLiteral("create-1"),
                            .name = emoji.repeated(80) };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(room)));
    room.name.append(emoji);
    CHECK(encodeFailureCode(encodeClientMessage(room)) ==
          ProtocolFailureCode::MalformedMessage);

    room.name = QStringLiteral("Room");
    room.password = QString(64, QChar(0x00e9));
    CHECK(std::holds_alternative<QString>(encodeClientMessage(room)));
    room.password->append(QChar(0x00e9));
    CHECK(encodeFailureCode(encodeClientMessage(room)) ==
          ProtocolFailureCode::MalformedMessage);

    auto chat = ChatSend{ .requestId = QStringLiteral("chat-1"),
                          .roomId = QStringLiteral("room-1"),
                          .roomGeneration = 1,
                          .connectionGeneration = 1,
                          .text = emoji.repeated(500) };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(chat)));
    chat.text.append(emoji);
    CHECK(encodeFailureCode(encodeClientMessage(chat)) ==
          ProtocolFailureCode::MalformedMessage);

    auto ticket =
      ClientHello{ .clientVersion = QStringLiteral("1.0"),
                   .ticket = QString(MaxTicketCharacters, QChar(u'x')) };
    CHECK(std::holds_alternative<QString>(encodeClientMessage(ticket)));
    ticket.ticket->append(QChar(u'x'));
    CHECK(encodeFailureCode(encodeClientMessage(ticket)) ==
          ProtocolFailureCode::MalformedMessage);

    auto badSurrogate = QStringLiteral("ok");
    badSurrogate.append(QChar(0xd800));
    CHECK(encodeFailureCode(encodeClientMessage(
            ClientHello{ .clientVersion = badSurrogate })) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol enforces numeric and collection limits",
          "[arena][protocol]")
{
    using namespace arena;

    auto directory = QJsonObject{
        { QStringLiteral("revision"), static_cast<double>(MaxJsonSafeInteger) },
        { QStringLiteral("rooms"), QJsonArray{} },
    };
    CHECK(messageAs<DirectorySnapshot>(decodeServerMessage(envelope(
            QStringLiteral("directory_snapshot"), directory))) != nullptr);

    for (const auto invalid :
         { -1.0, 1.5, static_cast<double>(MaxJsonSafeInteger) + 2.0 }) {
        directory.insert(QStringLiteral("revision"), invalid);
        CHECK(failureCode(decodeServerMessage(
                envelope(QStringLiteral("directory_snapshot"), directory))) ==
              ProtocolFailureCode::MalformedMessage);
    }

    auto snapshot = roomSnapshotObject();
    snapshot.insert(QStringLiteral("roomGeneration"), 0);
    CHECK(
      failureCode(decodeServerMessage(envelope(QStringLiteral("room_snapshot"),
                                               snapshot,
                                               QStringLiteral("join-1")))) ==
      ProtocolFailureCode::MalformedMessage);

    snapshot = roomSnapshotObject();
    QJsonArray members;
    for (int i = 0; i < RoomCapacity + 1; ++i) {
        members.append(memberObject(QStringLiteral("member-%1").arg(i)));
    }
    snapshot.insert(QStringLiteral("members"), members);
    CHECK(
      failureCode(decodeServerMessage(envelope(QStringLiteral("room_snapshot"),
                                               snapshot,
                                               QStringLiteral("join-1")))) ==
      ProtocolFailureCode::MalformedMessage);

    auto summary = roomSummaryObject();
    summary.insert(QStringLiteral("connectedCount"), 16);
    summary.insert(QStringLiteral("reservedCount"), 1);
    directory = { { QStringLiteral("revision"), 1 },
                  { QStringLiteral("rooms"), QJsonArray{ summary } } };
    CHECK(failureCode(decodeServerMessage(
            envelope(QStringLiteral("directory_snapshot"), directory))) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol enforces the four MiB server frame cap before parsing",
          "[arena][protocol]")
{
    using namespace arena;

    const auto small = envelope(QStringLiteral("directory_snapshot"),
                                { { QStringLiteral("revision"), 0 },
                                  { QStringLiteral("rooms"), QJsonArray{} } });
    const auto smallBytes = small.toUtf8().size();
    auto exact = small;
    exact.append(QString(MaxServerMessageBytes - smallBytes, QChar(u' ')));
    REQUIRE(exact.toUtf8().size() == MaxServerMessageBytes);
    CHECK(messageAs<DirectorySnapshot>(decodeServerMessage(exact)) != nullptr);

    exact.append(QChar(u' '));
    CHECK(failureCode(decodeServerMessage(exact)) ==
          ProtocolFailureCode::FrameTooLarge);

    QString multibyte(static_cast<qsizetype>(MaxServerMessageBytes / 2 + 1),
                      QChar(0x00e9));
    REQUIRE(multibyte.size() < MaxServerMessageBytes);
    REQUIRE(multibyte.toUtf8().size() > MaxServerMessageBytes);
    CHECK(failureCode(decodeServerMessage(multibyte)) ==
          ProtocolFailureCode::FrameTooLarge);

    const QString obviouslyOversized(MaxServerMessageBytes + 1, QChar(u'x'));
    CHECK(failureCode(decodeServerMessage(obviouslyOversized)) ==
          ProtocolFailureCode::FrameTooLarge);
}

TEST_CASE("ArenaProtocol accepts maximum legal competition snapshots within "
          "dedicated caps",
          "[arena][protocol][phase3]")
{
    using namespace arena;
    STATIC_REQUIRE(MaxStandingsMessageBytes == 65'536);
    STATIC_REQUIRE(MaxResultSnapshotBytes == 262'144);
    STATIC_REQUIRE(MaxFinalizationMessageBytes == 524'288);
    STATIC_REQUIRE(MaxFinalizationMessageBytes < MaxServerMessageBytes);

    const auto fixture = loadPhase3ProtocolFixture();
    auto standings =
      fixtureMessage(fixture, u"serverMessages", u"round_standings");
    auto standingsData = standings.value(QStringLiteral("data")).toObject();
    const auto standingTemplate =
      standingsData.value(QStringLiteral("entries")).toArray().at(0).toObject();
    QJsonArray standingEntries;
    for (int i = 0; i < RoomCapacity; ++i) {
        auto entry = standingTemplate;
        entry.insert(QStringLiteral("memberId"),
                     QStringLiteral("member-%1").arg(i));
        standingEntries.append(entry);
    }
    standingsData.insert(QStringLiteral("entries"), standingEntries);
    standings.insert(QStringLiteral("data"), standingsData);
    const auto standingsText = compact(standings);
    REQUIRE(standingsText.toUtf8().size() <= MaxStandingsMessageBytes);
    CHECK(messageAs<LiveStandingsSnapshot>(
            decodeServerMessage(standingsText)) != nullptr);

    auto finalized =
      fixtureMessage(fixture, u"serverMessages", u"round_finalized");
    auto finalizedData = finalized.value(QStringLiteral("data")).toObject();
    auto result = finalizedData.value(QStringLiteral("result")).toObject();
    auto selection = result.value(QStringLiteral("selection")).toObject();
    QJsonArray randomSequence;
    for (int i = 0; i < MaxRandomSequenceEntries; ++i) {
        randomSequence.append(static_cast<double>(MaxJsonSafeInteger));
    }
    selection.insert(QStringLiteral("randomSequence"), randomSequence);
    const auto emoji = QString::fromUtf8("\xF0\x9F\x92\xA5");
    selection.insert(QStringLiteral("title"), emoji.repeated(200));
    selection.insert(QStringLiteral("subtitle"), emoji.repeated(200));
    selection.insert(QStringLiteral("artist"), emoji.repeated(200));
    result.insert(QStringLiteral("selection"), selection);
    result.insert(QStringLiteral("participantCount"), RoomCapacity);

    const auto finalTemplate =
      result.value(QStringLiteral("entries")).toArray().at(0).toObject();
    const auto memberTemplate =
      finalizedData.value(QStringLiteral("members")).toArray().at(0).toObject();
    QJsonArray finalEntries;
    QJsonArray members;
    QJsonArray winners;
    for (int i = 0; i < RoomCapacity; ++i) {
        const auto suffix = QString::number(i);
        const auto memberPrefix = QStringLiteral("member-");
        const auto memberId =
          memberPrefix +
          QString(MaxOpaqueIdCharacters - memberPrefix.size() - suffix.size(),
                  QChar(u'm')) +
          suffix;
        const auto userPrefix = QStringLiteral("user-");
        const auto userId =
          userPrefix +
          QString(MaxOpaqueIdCharacters - userPrefix.size() - suffix.size(),
                  QChar(u'u')) +
          suffix;
        const auto avatarPrefix = QStringLiteral("https://example.com/");
        const auto avatar =
          avatarPrefix +
          QString(MaxAvatarUrlCharacters - avatarPrefix.size(), QChar(u'a'));
        const auto identity = QJsonObject{
            { QStringLiteral("userId"), userId },
            { QStringLiteral("displayName"), emoji.repeated(80) },
            { QStringLiteral("avatarUrl"), avatar },
        };

        auto entry = finalTemplate;
        entry.insert(QStringLiteral("memberId"), memberId);
        entry.insert(QStringLiteral("identity"), identity);
        entry.insert(QStringLiteral("lobbyWinsAfter"),
                     static_cast<double>(MaxUInt32));
        finalEntries.append(entry);
        winners.append(memberId);

        auto member = memberTemplate;
        member.insert(QStringLiteral("memberId"), memberId);
        member.insert(QStringLiteral("identity"), identity);
        member.insert(QStringLiteral("lobbyWins"),
                      static_cast<double>(MaxUInt32));
        members.append(member);
    }
    result.insert(QStringLiteral("entries"), finalEntries);
    result.insert(QStringLiteral("winnerMemberIds"), winners);
    finalizedData.insert(QStringLiteral("result"), result);
    finalizedData.insert(QStringLiteral("members"), members);
    finalized.insert(QStringLiteral("data"), finalizedData);

    const auto resultBytes =
      QJsonDocument(result).toJson(QJsonDocument::Compact).size();
    const auto finalizedText = compact(finalized);
    REQUIRE(resultBytes <= MaxResultSnapshotBytes);
    REQUIRE(finalizedText.toUtf8().size() <= MaxFinalizationMessageBytes);
    REQUIRE(finalizedText.toUtf8().size() <= MaxServerMessageBytes);
    CHECK(messageAs<RoundFinalized>(decodeServerMessage(finalizedText)) !=
          nullptr);
}

TEST_CASE(
  "ArenaProtocol applies competition message caps below the transport ceiling",
  "[arena][protocol][phase3]")
{
    using namespace arena;
    const auto fixture = loadPhase3ProtocolFixture();

    auto standingsText =
      compact(fixtureMessage(fixture, u"serverMessages", u"round_standings"));
    standingsText.append(QString(
      MaxStandingsMessageBytes - standingsText.toUtf8().size(), QChar(u' ')));
    REQUIRE(standingsText.toUtf8().size() == MaxStandingsMessageBytes);
    CHECK(messageAs<LiveStandingsSnapshot>(
            decodeServerMessage(standingsText)) != nullptr);
    standingsText.append(QChar(u' '));
    CHECK(failureCode(decodeServerMessage(standingsText)) ==
          ProtocolFailureCode::FrameTooLarge);

    auto finalizedText =
      compact(fixtureMessage(fixture, u"serverMessages", u"round_finalized"));
    finalizedText.append(
      QString(MaxFinalizationMessageBytes - finalizedText.toUtf8().size(),
              QChar(u' ')));
    REQUIRE(finalizedText.toUtf8().size() == MaxFinalizationMessageBytes);
    CHECK(messageAs<RoundFinalized>(decodeServerMessage(finalizedText)) !=
          nullptr);
    finalizedText.append(QChar(u' '));
    CHECK(failureCode(decodeServerMessage(finalizedText)) ==
          ProtocolFailureCode::FrameTooLarge);
}

TEST_CASE("ArenaProtocol rejects duplicate wire IDs atomically",
          "[arena][protocol]")
{
    using namespace arena;

    const auto duplicateRooms =
      envelope(QStringLiteral("directory_snapshot"),
               { { QStringLiteral("revision"), 1 },
                 { QStringLiteral("rooms"),
                   QJsonArray{ roomSummaryObject(), roomSummaryObject() } } });
    CHECK(failureCode(decodeServerMessage(duplicateRooms)) ==
          ProtocolFailureCode::MalformedMessage);

    auto snapshot = roomSnapshotObject();
    snapshot.insert(QStringLiteral("members"),
                    QJsonArray{ memberObject(), memberObject() });
    CHECK(
      failureCode(decodeServerMessage(envelope(QStringLiteral("room_snapshot"),
                                               snapshot,
                                               QStringLiteral("join-1")))) ==
      ProtocolFailureCode::MalformedMessage);

    snapshot = roomSnapshotObject();
    snapshot.insert(QStringLiteral("chat"),
                    QJsonArray{ chatObject(), chatObject() });
    CHECK(
      failureCode(decodeServerMessage(envelope(QStringLiteral("room_snapshot"),
                                               snapshot,
                                               QStringLiteral("join-1")))) ==
      ProtocolFailureCode::MalformedMessage);

    const auto duplicateDelta =
      envelope(QStringLiteral("room_directory_updated"),
               { { QStringLiteral("revision"), 2 },
                 { QStringLiteral("upserts"),
                   QJsonArray{ roomSummaryObject(QStringLiteral("room-1")),
                               roomSummaryObject(QStringLiteral("room-1")) } },
                 { QStringLiteral("removedRoomIds"), QJsonArray{} } });
    CHECK(failureCode(decodeServerMessage(duplicateDelta)) ==
          ProtocolFailureCode::MalformedMessage);

    const auto overlapDelta =
      envelope(QStringLiteral("room_directory_updated"),
               { { QStringLiteral("revision"), 2 },
                 { QStringLiteral("upserts"),
                   QJsonArray{ roomSummaryObject(QStringLiteral("room-1")) } },
                 { QStringLiteral("removedRoomIds"),
                   QJsonArray{ QStringLiteral("room-1") } } });
    CHECK(failureCode(decodeServerMessage(overlapDelta)) ==
          ProtocolFailureCode::MalformedMessage);
}

TEST_CASE("ArenaProtocol failures never retain sensitive input",
          "[arena][protocol]")
{
    using namespace arena;

    static_assert(sizeof(ProtocolFailure) == sizeof(ProtocolFailureCode));
    CHECK(displayMessageKey(ProtocolFailureCode::MalformedMessage) ==
          QStringLiteral("arena.error.malformedMessage"));
    CHECK(displayMessageKey(ProtocolFailureCode::FrameTooLarge) ==
          QStringLiteral("arena.error.frameTooLarge"));

    const auto result = decodeServerMessage(QStringLiteral(
      R"({"ticket":"sentinel-ticket","password":"sentinel-password","seatToken":"sentinel-token","text":"sentinel-chat")"));
    REQUIRE(std::holds_alternative<ProtocolFailure>(result));
    CHECK(std::get<ProtocolFailure>(result).code ==
          ProtocolFailureCode::MalformedMessage);
}
