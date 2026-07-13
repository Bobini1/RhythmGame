#pragma once

#include "ArenaChatModel.h"
#include "ArenaAvailabilityIndex.h"
#include "ArenaGameplaySource.h"
#include "ArenaIdentityProvider.h"
#include "ArenaInventorySource.h"
#include "ArenaMemberListModel.h"
#include "ArenaOpponentTarget.h"
#include "ArenaProtocol.h"
#include "ArenaResultModel.h"
#include "ArenaRoomListModel.h"
#include "ArenaRoundLoader.h"
#include "ArenaScheduler.h"
#include "ArenaStandingsModel.h"
#include "ArenaTransport.h"
#include "ArenaTypes.h"
#include "gameplay_logic/ChartRunner.h"

#include <QHash>
#include <QObject>
#include <QPointer>
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
    Q_PROPERTY(QString roundLaunchCancellationStatusKey READ
                 getRoundLaunchCancellationStatusKey NOTIFY
                   roundLaunchCancellationStatusKeyChanged FINAL)

    Q_PROPERTY(QString roomId READ getRoomId NOTIFY roomChanged FINAL)
    Q_PROPERTY(QString roomName READ getRoomName NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      qint64 roomGeneration READ getRoomGeneration NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      QString selfMemberId READ getSelfMemberId NOTIFY roomChanged FINAL)
    Q_PROPERTY(
      QString ownerMemberId READ getOwnerMemberId NOTIFY ownerChanged FINAL)
    Q_PROPERTY(bool isOwner READ getIsOwner NOTIFY ownerChanged FINAL)
    Q_PROPERTY(bool roundsAvailable READ getRoundsAvailable NOTIFY
                 capabilitiesChanged FINAL)
    Q_PROPERTY(bool competitionAvailable READ competitionAvailable NOTIFY
                 capabilitiesChanged FINAL)
    Q_PROPERTY(bool availabilitySyncing READ getAvailabilitySyncing NOTIFY
                 availabilityChanged FINAL)
    Q_PROPERTY(arena::ArenaAvailabilityIndex* availability READ getAvailability
                 CONSTANT FINAL)
    Q_PROPERTY(
      arena::RoomPhase roomPhase READ getRoomPhase NOTIFY roundChanged FINAL)
    Q_PROPERTY(bool canSelect READ getCanSelect NOTIFY selectionChanged FINAL)
    Q_PROPERTY(bool canReady READ getCanReady NOTIFY readyChanged FINAL)
    Q_PROPERTY(bool ready READ getReady NOTIFY readyChanged FINAL)
    Q_PROPERTY(
      QString selectedTitle READ getSelectedTitle NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QString selectedSubtitle READ getSelectedSubtitle NOTIFY
                 selectionChanged FINAL)
    Q_PROPERTY(
      QString selectedMd5 READ getSelectedMd5 NOTIFY selectionChanged FINAL)
    Q_PROPERTY(QString selectedByMemberId READ getSelectedByMemberId NOTIFY
                 selectionChanged FINAL)
    Q_PROPERTY(QString selectedByDisplayName READ getSelectedByDisplayName
                 NOTIFY selectedByDisplayNameChanged FINAL)
    Q_PROPERTY(qint64 selectionRevision READ getSelectionRevision NOTIFY
                 selectionChanged FINAL)
    Q_PROPERTY(
      QString currentRoundId READ getCurrentRoundId NOTIFY roundChanged FINAL)

    Q_PROPERTY(arena::ArenaRoomListModel* rooms READ getRooms CONSTANT FINAL)
    Q_PROPERTY(
      arena::ArenaMemberListModel* members READ getMembers CONSTANT FINAL)
    Q_PROPERTY(arena::ArenaChatModel* chat READ getChat CONSTANT FINAL)
    Q_PROPERTY(arena::ArenaStandingsModel* liveStandings READ liveStandings
                 CONSTANT FINAL)
    Q_PROPERTY(
      arena::ArenaResultModel* lastResult READ lastResult CONSTANT FINAL)
    Q_PROPERTY(arena::ArenaResultModel* presentedResult READ presentedResult
                 CONSTANT FINAL)
    Q_PROPERTY(arena::ArenaOpponentTarget* opponentTarget READ opponentTarget
                 CONSTANT FINAL)
    Q_PROPERTY(gameplay_logic::ChartRunner* arenaRunner READ arenaRunner NOTIFY
                 competitionChanged FINAL)
    Q_PROPERTY(bool arenaGameplayActive READ arenaGameplayActive NOTIFY
                 competitionChanged FINAL)
    Q_PROPERTY(bool resultPresentationActive READ resultPresentationActive
                 NOTIFY competitionChanged FINAL)
    Q_PROPERTY(bool gameplayChatOpen READ gameplayChatOpen WRITE
                 setGameplayChatOpen NOTIFY gameplayChatOpenChanged FINAL)
    Q_PROPERTY(bool overlayCustomizationActive READ overlayCustomizationActive
                 NOTIFY overlayCustomizationActiveChanged FINAL)
    Q_PROPERTY(QString arenaOptionsSummary READ arenaOptionsSummary NOTIFY
                 competitionChanged FINAL)

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
                          ArenaInventorySource* inventorySource = nullptr,
                          ArenaRoundLoader* roundLoader = nullptr,
                          ArenaGameplaySource* gameplaySource = nullptr,
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
    [[nodiscard]] auto getRoundLaunchCancellationStatusKey() const -> QString;

    [[nodiscard]] auto getRoomId() const -> QString;
    [[nodiscard]] auto getRoomName() const -> QString;
    [[nodiscard]] auto getRoomGeneration() const -> qint64;
    [[nodiscard]] auto getSelfMemberId() const -> QString;
    [[nodiscard]] auto getOwnerMemberId() const -> QString;
    [[nodiscard]] auto getIsOwner() const -> bool;
    [[nodiscard]] auto getRoundsAvailable() const -> bool;
    [[nodiscard]] auto competitionAvailable() const -> bool;
    [[nodiscard]] auto getAvailabilitySyncing() const -> bool;
    [[nodiscard]] auto getAvailability() -> ArenaAvailabilityIndex*;
    [[nodiscard]] auto getRoomPhase() const -> RoomPhase;
    [[nodiscard]] auto getCanSelect() const -> bool;
    [[nodiscard]] auto getCanReady() const -> bool;
    [[nodiscard]] auto getReady() const -> bool;
    [[nodiscard]] auto getSelectedTitle() const -> QString;
    [[nodiscard]] auto getSelectedSubtitle() const -> QString;
    [[nodiscard]] auto getSelectedMd5() const -> QString;
    [[nodiscard]] auto getSelectedByMemberId() const -> QString;
    [[nodiscard]] auto getSelectedByDisplayName() const -> QString;
    [[nodiscard]] auto getSelectionRevision() const -> qint64;
    [[nodiscard]] auto getCurrentRoundId() const -> QString;

    [[nodiscard]] auto getRooms() -> ArenaRoomListModel*;
    [[nodiscard]] auto getMembers() -> ArenaMemberListModel*;
    [[nodiscard]] auto getChat() -> ArenaChatModel*;
    [[nodiscard]] auto liveStandings() -> ArenaStandingsModel*;
    [[nodiscard]] auto lastResult() -> ArenaResultModel*;
    [[nodiscard]] auto presentedResult() -> ArenaResultModel*;
    [[nodiscard]] auto opponentTarget() -> ArenaOpponentTarget*;
    [[nodiscard]] auto arenaRunner() const -> gameplay_logic::ChartRunner*;
    [[nodiscard]] auto arenaGameplayActive() const -> bool;
    [[nodiscard]] auto resultPresentationActive() const -> bool;
    [[nodiscard]] auto gameplayChatOpen() const -> bool;
    [[nodiscard]] auto overlayCustomizationActive() const -> bool;
    [[nodiscard]] auto arenaOptionsSummary() const -> QString;

    Q_INVOKABLE void connectForBrowsing();
    Q_INVOKABLE void exitArena();
    Q_INVOKABLE void createRoom(const QString& name, const QString& password);
    Q_INVOKABLE void joinRoom(const QString& roomId, const QString& password);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void kickMember(const QString& memberId);
    Q_INVOKABLE void sendChat(const QString& text);
    Q_INVOKABLE void retry();
    Q_INVOKABLE void selectChart(gameplay_logic::ChartData* chart);
    Q_INVOKABLE void setReady(bool ready);
    Q_INVOKABLE bool submitLocalResult(gameplay_logic::BmsScore* score);
    Q_INVOKABLE void releasePreparedGameplay(
      gameplay_logic::ChartRunner* runner);
    Q_INVOKABLE void abandonCurrentRound();
    Q_INVOKABLE void setGameplayChatOpen(bool open);
    Q_INVOKABLE void toggleGameplayChat();
    Q_INVOKABLE void setOverlayCustomizationActive(bool active);
    Q_INVOKABLE void endResultPresentation(const QString& roundId);

  signals:
    void stateChanged();
    void activeChanged();
    void authenticatedChanged();
    void loginRequiredChanged();
    void directoryReadyChanged();
    void admissionPendingChanged();
    void errorChanged();
    void roundLaunchCancellationStatusKeyChanged();
    void roomChanged();
    void ownerChanged();
    void capabilitiesChanged();
    void availabilityChanged();
    void selectionChanged();
    void selectedByDisplayNameChanged();
    void readyChanged();
    void roundChanged();
    void preparedGameplayChanged(gameplay_logic::ChartRunner* runner);
    void roundRunnerStarted(const QString& roundId,
                            gameplay_logic::ChartRunner* runner);
    void roundLaunchCancelled();
    void competitionChanged();
    void gameplayChatOpenChanged();
    void overlayCustomizationActiveChanged();

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
        Chat,
        InventoryBegin,
        InventoryCommit,
        AvailabilityApplied,
        AvailabilityResync,
        Selection,
        Ready,
        ProbeResult,
        LoadResult,
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
    ArenaInventorySource* m_inventorySource;
    ArenaRoundLoader* m_roundLoader;
    ArenaGameplaySource* m_gameplaySource;
    QUrl m_endpoint;
    QString m_clientVersion;

    ArenaRoomListModel m_rooms;
    ArenaMemberListModel m_members;
    ArenaChatModel m_chat;
    ArenaAvailabilityIndex m_availability;
    ArenaStandingsModel m_liveStandings;
    ArenaResultModel m_lastResult;
    ArenaResultModel m_presentedResult;
    ArenaOpponentTarget m_opponentTarget;

    State m_state{ State::Disconnected };
    bool m_active{};
    bool m_authenticated{};
    bool m_loginRequired{};
    bool m_directoryReady{};
    bool m_roundsAvailable{};
    bool m_competitionAvailable{};
    QString m_errorCode;
    QString m_errorMessageKey;
    QString m_roundLaunchCancellationStatusKey;

    QString m_roomId;
    QString m_roomName;
    qint64 m_roomGeneration{};
    qint64 m_connectionGeneration{};
    QString m_selfMemberId;
    std::optional<QString> m_ownerMemberId{ std::nullopt };
    QString m_resumeToken;
    RoomPhase m_roomPhase{ RoomPhase::Selecting };
    std::optional<SelectionSnapshot> m_selection;
    QString m_selectedByMemberId;
    qint64 m_selectionRevision{};
    qint64 m_roomAvailabilityRevision{};
    std::optional<FrozenRound> m_round;
    bool m_selfReady{};
    InventoryState m_selfInventoryState{ InventoryState::Missing };
    qint64 m_selfInventoryRevision{};
    qint64 m_selfAvailabilityAppliedRevision{};

    PendingAdmission m_pendingAdmission{};
    QString m_pendingAdmissionRequestId;
    QHash<QString, PendingCommand> m_pendingCommands;
    QVector<QString> m_pendingChatCommandIds;
    std::optional<qint64> m_directoryRevision{ std::nullopt };
    bool m_directoryResyncPending{};
    bool m_legacyFallbackAvailable{};
    bool m_legacyBrowseOnly{};

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

    struct PendingInventoryUpload
    {
        ArenaInventorySnapshot snapshot;
        QString beginRequestId;
        QString commitRequestId;
        QString uploadId;
        QString digest;
    };

    struct PendingAvailabilityTransfer
    {
        AvailabilityTransferBegin declaration;
        QByteArray transferId;
        QByteArray reset;
        QByteArray added;
        QByteArray removed;
        quint32 resetChunks{};
        quint32 addedChunks{};
        quint32 removedChunks{};
    };

    quint64 m_nextInventorySourceRequestId{ 1 };
    quint64 m_inventorySourceRequestId{};
    qint64 m_inventorySourceRequestGeneration{};
    qint64 m_lastPublishedLibraryGeneration{};
    qint64 m_uncertainCommittedGeneration{};
    qint64 m_uncertainBaseInventoryRevision{};
    QString m_availabilityResyncRequestId;
    QString m_availabilityAppliedRequestId;
    QString m_readyRequestId;
    std::optional<bool> m_requestedReady;
    QString m_selectionRequestId;
    std::optional<SelectionSnapshot> m_requestedSelection;

    enum class RoundLoaderOperation
    {
        None,
        Probe,
        Load,
    };
    quint64 m_nextRoundLoaderRequestId{ 1 };
    quint64 m_roundLoaderRequestId{};
    RoundLoaderOperation m_roundLoaderOperation{ RoundLoaderOperation::None };
    std::optional<RoundProbeRequested> m_probeRequest;
    std::optional<FrozenRound> m_loadRequestRound;
    QPointer<gameplay_logic::ChartRunner> m_preparedRunner;
    QPointer<gameplay_logic::ChartRunner> m_retainedGameplayRunner;
    quint64 m_retainedGameplayLoadRequestId{};
    bool m_preparedGameplayExposed{};
    bool m_roundRunnerStartedEmitted{};
    ArenaScheduler::TaskId m_roundStartTask{};
    QString m_probeResultRequestId;
    QString m_loadResultRequestId;
    std::optional<PendingInventoryUpload> m_pendingInventoryUpload;
    std::optional<PendingAvailabilityTransfer> m_pendingAvailabilityTransfer;

    struct PendingTerminal
    {
        std::variant<RoundResultSubmit, RoundAbandon> message;
    };

    QHash<QString, PublicIdentity> m_competitionIdentities;
    QPointer<gameplay_logic::ChartRunner> m_arenaRunner;
    QMetaObject::Connection m_arenaRunnerStatusConnection;
    QPointer<gameplay_logic::ChartRunner> m_customizationRunner;
    QMetaObject::Connection m_customizationRunnerDestroyedConnection;
    bool m_gameplaySourceAttached{};
    bool m_arenaGameplayActive{};
    bool m_resultPresentationActive{};
    bool m_gameplayChatOpen{};
    bool m_overlayCustomizationActive{};
    bool m_localRoundAbandoned{};
    bool m_localTerminalSubmitted{};
    QString m_arenaOptionsSummary;
    QString m_expectedScoreGuid;
    ArenaScheduler::TaskId m_telemetryTask{};
    qint64 m_nextTelemetryDueMs{};
    quint32 m_nextTelemetrySequence{ 1 };
    std::optional<RoundTelemetry> m_pendingTelemetry;
    std::optional<PendingTerminal> m_pendingTerminal;

    void setState(State state);
    void setActive(bool active);
    void setAuthenticated(bool authenticated);
    void setLoginRequired(bool required);
    void setDirectoryReady(bool ready);
    void setRoundsAvailable(bool available);
    void setCompetitionAvailable(bool available);
    void clearError();
    void setError(QString code, QString messageKey);
    void setRoundLaunchCancellationStatusKey(QString statusKey);
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
    void requestInventorySnapshot();
    void cancelInventorySnapshot();
    void handleInventoryGenerationChanged(qint64 generation);
    void handleInventorySnapshotReady(quint64 requestId,
                                      ArenaInventorySnapshot snapshot);
    void handleInventorySnapshotFailed(quint64 requestId,
                                       ArenaInventoryFailure failure);
    void handleInventoryUploadReady(const InventoryUploadReady& ready);
    void handleInventoryCommitted(const InventoryCommitted& committed);
    void handleAvailabilityTransferBegin(
      const AvailabilityTransferBegin& begin);
    void handleAvailabilityTransferCommit(
      const AvailabilityTransferCommit& commit);
    void applySelfMember(const Member& member);
    void applySelection(std::optional<SelectionSnapshot> selection,
                        qint64 selectionRevision,
                        qint64 availabilityRevision,
                        std::optional<QString> selectedByMemberId);
    void requestAvailabilityResync();
    void clearRoundTransfers(bool abandonSeat = true,
                             bool preservePreparedRound = false,
                             bool preserveCompetitionScore = false);
    void cancelRoundLoader();
    void handleProbeRequested(const RoundProbeRequested& requested);
    void handleLoadRequested(const RoundLoadRequested& requested);
    void handleProbeFinished(quint64 requestId, ArenaProbeResult result);
    void handleLoadFinished(quint64 requestId,
                            gameplay_logic::ChartRunner* runner);
    void handleLoadFailed(quint64 requestId, ArenaLoadFailure failure);
    void handleRoundStartScheduled(const RoundStartScheduled& scheduled);
    void cancelPreparedRound(bool notify, bool preserveScoreGuid = false);
    void retainPreparedGameplayUntilReleased();
    void cacheCompetitionIdentities(const FrozenRound& round);
    void beginCompetitionRound(const FrozenRound& round);
    void clearCompetitionState(bool clearLastResult = true);
    void clearActiveCompetitionRound(bool preservePresentedResult = false);
    [[nodiscard]] auto attachGameplaySource(gameplay_logic::ChartRunner* runner)
      -> bool;
    void detachGameplaySource(bool preserveScoreGuid = false,
                              bool preserveCustomization = false);
    void synchronizeCustomizationRunner();
    void releaseCustomizationRunner();
    void handleArenaRunnerStatusChanged();
    void startTelemetrySampling();
    void scheduleTelemetryTick();
    void stopTelemetrySampling();
    void sampleTelemetry();
    void sendOrQueueTelemetry(TelemetrySnapshot telemetry);
    void flushCompetitionMessages();
    void sendTerminal(std::variant<RoundResultSubmit, RoundAbandon> message);
    void sendAbandon(DnfReason reason);
    void showPendingResult(bool localDnf);
    void handleStandings(const LiveStandingsSnapshot& snapshot);
    void handleTerminalAccepted(const RoundTerminalAccepted& accepted);
    void handleRoundFinalized(const RoundFinalized& finalized);
    [[nodiscard]] auto currentCompetitionRound(
      QStringView roundId,
      QStringView launchAttemptId) const -> bool;
    void failProtocol(ProtocolFailureCode code);
    void failProtocol(QString code, QString messageKey);
};

} // namespace arena
