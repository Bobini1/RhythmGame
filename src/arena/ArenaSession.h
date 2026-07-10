#pragma once

#include "ArenaChatModel.h"
#include "ArenaIdentityProvider.h"
#include "ArenaMemberListModel.h"
#include "ArenaProtocol.h"
#include "ArenaRoomListModel.h"
#include "ArenaScheduler.h"
#include "ArenaTransport.h"
#include "ArenaTypes.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>
#include <QtTypes>

#include <optional>
#include <variant>

namespace arena {

class ArenaSession final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
      arena::ArenaSession::State state READ getState NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool active READ getActive NOTIFY activeChanged FINAL)
    Q_PROPERTY(bool authenticated READ getAuthenticated NOTIFY
                 authenticatedChanged FINAL)
    Q_PROPERTY(bool loginRequired READ getLoginRequired NOTIFY
                 loginRequiredChanged FINAL)
    Q_PROPERTY(bool reconnecting READ getReconnecting NOTIFY stateChanged FINAL)
    Q_PROPERTY(bool directoryReady READ getDirectoryReady NOTIFY
                 directoryReadyChanged FINAL)
    Q_PROPERTY(bool admissionPending READ getAdmissionPending NOTIFY
                 admissionPendingChanged FINAL)
    Q_PROPERTY(QString errorCode READ getErrorCode NOTIFY errorChanged FINAL)
    Q_PROPERTY(
      QString errorMessageKey READ getErrorMessageKey NOTIFY errorChanged FINAL)

    Q_PROPERTY(QString roomId READ getRoomId NOTIFY roomChanged FINAL)
    Q_PROPERTY(QString roomName READ getRoomName NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      qint64 roomGeneration READ getRoomGeneration NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      QString selfMemberId READ getSelfMemberId NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      QString ownerMemberId READ getOwnerMemberId NOTIFY ownerChanged FINAL)
    Q_PROPERTY(bool isOwner READ getIsOwner NOTIFY ownerChanged FINAL)

    Q_PROPERTY(arena::ArenaRoomListModel* rooms READ getRooms CONSTANT FINAL)
    Q_PROPERTY(
      arena::ArenaMemberListModel* members READ getMembers CONSTANT FINAL)
    Q_PROPERTY(arena::ArenaChatModel* chat READ getChat CONSTANT FINAL)

  public:
    enum class State
    {
        Disconnected,
        Browsing,
        ConnectingAuthenticated,
        InRoom,
        Reconnecting,
        Error,
    };
    Q_ENUM(State)

    explicit ArenaSession(ArenaTransport* transport,
                          ArenaIdentityProvider* identityProvider,
                          ArenaScheduler* scheduler,
                          QUrl endpoint,
                          QString clientVersion,
                          QObject* parent = nullptr);
    ~ArenaSession() override;

    [[nodiscard]] auto getState() const -> State;
    [[nodiscard]] auto getActive() const -> bool;
    [[nodiscard]] auto getAuthenticated() const -> bool;
    [[nodiscard]] auto getLoginRequired() const -> bool;
    [[nodiscard]] auto getReconnecting() const -> bool;
    [[nodiscard]] auto getDirectoryReady() const -> bool;
    [[nodiscard]] auto getAdmissionPending() const -> bool;
    [[nodiscard]] auto getErrorCode() const -> QString;
    [[nodiscard]] auto getErrorMessageKey() const -> QString;

    [[nodiscard]] auto getRoomId() const -> QString;
    [[nodiscard]] auto getRoomName() const -> QString;
    [[nodiscard]] auto getRoomGeneration() const -> qint64;
    [[nodiscard]] auto getSelfMemberId() const -> QString;
    [[nodiscard]] auto getOwnerMemberId() const -> QString;
    [[nodiscard]] auto getIsOwner() const -> bool;

    [[nodiscard]] auto getRooms() -> ArenaRoomListModel*;
    [[nodiscard]] auto getMembers() -> ArenaMemberListModel*;
    [[nodiscard]] auto getChat() -> ArenaChatModel*;

    Q_INVOKABLE void connectForBrowsing();
    Q_INVOKABLE void exitArena();
    Q_INVOKABLE void createRoom(const QString& name, const QString& password);
    Q_INVOKABLE void joinRoom(const QString& roomId, const QString& password);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void kickMember(const QString& memberId);
    Q_INVOKABLE void sendChat(const QString& text);
    Q_INVOKABLE void retry();

  signals:
    void stateChanged();
    void activeChanged();
    void authenticatedChanged();
    void loginRequiredChanged();
    void directoryReadyChanged();
    void admissionPendingChanged();
    void errorChanged();
    void roomChanged();
    void ownerChanged();

  private:
    enum class HandshakeKind
    {
        None,
        AnonymousBrowse,
        AuthenticatedAdmission,
        Resume
    };
    enum class PendingCommandKind
    {
        Admission,
        Leave,
        Kick,
        Chat
    };

    struct PendingCreate
    {
        QString name;
        std::optional<QString> password{ std::nullopt };
    };
    struct PendingJoin
    {
        QString roomId;
        std::optional<QString> password{ std::nullopt };
    };
    using PendingAdmission =
      std::variant<std::monostate, PendingCreate, PendingJoin>;

    struct PendingCommand
    {
        PendingCommandKind kind;
        quint64 lifecycleGeneration{};
        QString relatedMemberId;
    };

    static constexpr qint64 ReconnectGraceMs = 60'000;
    static constexpr qint64 AttemptTimeoutMs = 10'000;
    static constexpr qint64 InitialBackoffMs = 500;
    static constexpr qint64 MaximumBackoffMs = 8'000;

    ArenaTransport* m_transport;
    ArenaIdentityProvider* m_identityProvider;
    ArenaScheduler* m_scheduler;
    QUrl m_endpoint;
    QString m_clientVersion;

    ArenaRoomListModel m_rooms;
    ArenaMemberListModel m_members;
    ArenaChatModel m_chat;

    State m_state{ State::Disconnected };
    bool m_active{};
    bool m_authenticated{};
    bool m_loginRequired{};
    bool m_directoryReady{};
    QString m_errorCode;
    QString m_errorMessageKey;

    QString m_roomId;
    QString m_roomName;
    qint64 m_roomGeneration{};
    qint64 m_connectionGeneration{};
    QString m_selfMemberId;
    std::optional<QString> m_ownerMemberId{ std::nullopt };
    QString m_resumeToken;

    PendingAdmission m_pendingAdmission{};
    QString m_pendingAdmissionRequestId;
    QHash<QString, PendingCommand> m_pendingCommands;
    QVector<QString> m_pendingChatCommandIds;
    std::optional<qint64> m_directoryRevision{ std::nullopt };
    bool m_directoryResyncPending{};

    HandshakeKind m_handshakeKind{ HandshakeKind::None };
    bool m_protocolReady{};
    quint64 m_lifecycleGeneration{ 1 };
    ArenaTransport::Generation m_currentTransportGeneration{};
    ArenaTransport::Generation m_nextTransportGeneration{ 1 };
    quint64 m_nextRequestId{ 1 };
    quint64 m_nextTicketRequestId{ 1 };
    quint64 m_currentTicketRequestId{};
    QString m_pendingTicket;
    bool m_lastLoggedIn{};

    ArenaScheduler::TaskId m_graceTask{};
    ArenaScheduler::TaskId m_attemptTimeoutTask{};
    ArenaScheduler::TaskId m_retryTask{};
    qint64 m_reconnectDeadlineMs{};
    qint64 m_nextBackoffMs{ InitialBackoffMs };

    void setState(State state);
    void setActive(bool active);
    void setAuthenticated(bool authenticated);
    void setLoginRequired(bool required);
    void setDirectoryReady(bool ready);
    void clearError();
    void setError(QString code, QString messageKey);
    void setPendingAdmission(PendingAdmission admission);
    void clearPendingAdmission();
    void restoreAnonymousAfterAdmissionFailure(QString code,
                                               QString messageKey);

    void startTransport(HandshakeKind kind);
    void invalidateTransport();
    void openAnonymousBrowsing();
    [[nodiscard]] auto sendMessage(const ClientMessage& message) -> bool;
    void sendDirectorySubscribe();
    [[nodiscard]] auto nextRequestId() -> QString;

    void handleConnected(ArenaTransport::Generation generation);
    void handleDisconnected(ArenaTransport::Generation generation);
    void handleTransportError(ArenaTransport::Generation generation,
                              ArenaTransport::Error error);
    void handleText(ArenaTransport::Generation generation, const QString& text);
    void handleBinary(ArenaTransport::Generation generation,
                      const QByteArray& bytes);
    void handleServerMessage(const ServerMessage& message);
    void handleServerHello(const ServerHello& hello);
    void handleDirectorySnapshot(const DirectorySnapshot& snapshot);
    void handleDirectoryDelta(const RoomDirectoryUpdated& delta);
    void handleCommandError(const CommandError& error);

    void beginAuthenticatedAdmission();
    void requestTicket();
    void handleTicketReady(quint64 requestId, const QString& ticket);
    void handleTicketFailure(quint64 requestId,
                             ArenaIdentityProvider::TicketFailure failure);
    void sendPendingAdmission();

    void applyRoomSnapshot(const RoomSnapshot& snapshot);
    void clearRoom();
    [[nodiscard]] auto acceptsRoomEvent(QStringView roomId, qint64 generation)
      -> bool;
    void returnToAuthenticatedBrowser();

    void beginReconnect();
    void startResumeAttempt();
    void scheduleReconnect();
    [[nodiscard]] auto resumeDeadlineReached() const -> bool;
    void failResumeAtDeadline();
    void cancelReconnectTasks();
    void cancelTask(ArenaScheduler::TaskId& taskId);

    void handleActiveProfileChanged();
    void handleLoginStateChanged();
    void cleanupForIdentityChange();
    void bestEffortLeave();
    void invalidateAsyncWork();
    void failProtocol(ProtocolFailureCode code);
    void failProtocol(QString code, QString messageKey);
};

} // namespace arena
