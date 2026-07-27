#pragma once

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QStringView>
#include <QtTypes>

#include <chrono>
#include <functional>

class QAudioOutput;
class QMediaDevices;
class QMediaPlayer;
class QTimer;
class QVideoFrame;
class QVideoSink;

class MediaProbe final : public QObject
{
    Q_OBJECT

public:
    using EventCallback =
        std::function<void(QStringView, QJsonObject)>;
    using CompletionCallback =
        std::function<void(QStringView, QJsonObject)>;
    using FailureCallback =
        std::function<void(QStringView, QStringView)>;

    explicit MediaProbe(
        EventCallback eventCallback,
        CompletionCallback completionCallback,
        FailureCallback failureCallback,
        QObject *parent = nullptr);

    [[nodiscard]] bool armDeviceObservation(quint32 runNonce);
    [[nodiscard]] bool attachVideoSink(QVideoSink *videoSink);
    void start(quint32 runNonce);
    [[nodiscard]] QJsonObject acknowledgeVisualCapture(
        quint32 runNonce,
        const QStringList &requestIds);

private:
    void createPlayer();
    void mediaStatusChanged(int status);
    void playbackStateChanged(int state);
    void videoFrameChanged(const QVideoFrame &frame);
    void positionChanged(qint64 position);
    void publishDeviceSnapshot(QStringView reason);
    void publishDeviceBatchSettled();
    void pollSeekProof();
    void handleNaturalEnd();
    void armBackendRemovalAndDestroy();
    void finishBackendRemoval();
    void finishObjectTeardownIfReady();
    void append(QStringView type, QJsonObject payload = {});
    void fail(QStringView code, QStringView detail);

    static constexpr qint64 requiredPositionAdvanceMilliseconds = 500;
    static constexpr qint64 requiredSeekPositionMilliseconds = 1000;
    static constexpr qint64 minimumSeekJumpMilliseconds = 100;
    static constexpr qint64 maximumSeekTargetErrorMilliseconds = 125;
    static constexpr qint64 maximumSeekArmPositionSkewMilliseconds = 125;
    static constexpr qint64 requiredPostSeekAdvanceMilliseconds = 100;
    static constexpr qint64 maximumEndPositionErrorMilliseconds = 125;
    static constexpr qsizetype maximumFramePositionSamples = 8;
    static constexpr qsizetype maximumPostSeekFramePositionSamples = 32;
    static constexpr qsizetype maximumDeviceSnapshots = 16;
    static constexpr qsizetype maximumDeviceSettlements = 16;
    static constexpr auto seekResponseTimeout =
        std::chrono::seconds{1};
    static constexpr auto seekProofPollInterval =
        std::chrono::milliseconds{10};
    static constexpr auto backendRemovalPollInterval =
        std::chrono::milliseconds{10};
    static constexpr auto backendRemovalStabilityWindow =
        std::chrono::milliseconds{250};
    static constexpr auto backendRemovalResponseTimeout =
        std::chrono::seconds{1};
    static constexpr auto mediaTimeout =
        std::chrono::seconds{30};

    EventCallback m_eventCallback;
    CompletionCallback m_completionCallback;
    FailureCallback m_failureCallback;
    QPointer<QVideoSink> m_videoSink;
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QMediaDevices *m_mediaDevices = nullptr;
    QTimer *m_timeout = nullptr;
    QTimer *m_deviceSettleTimer = nullptr;
    QTimer *m_seekProofPoll = nullptr;
    QTimer *m_seekTimeout = nullptr;
    QTimer *m_backendRemovalPoll = nullptr;
    QElapsedTimer m_elapsed;
    QJsonArray m_framePositionSamples;
    QJsonArray m_postSeekFramePositionSamples;
    QJsonObject m_backendRemovalArmRecord;
    QJsonObject m_seekProof;
    QStringList m_requestIds;
    QString m_seekElementId;
    quint32 m_runNonce = 0;
    qint64 m_lastFramePosition = -1;
    qint64 m_capturePausedPositionMilliseconds = -1;
    qint64 m_resumePositionMilliseconds = -1;
    qint64 m_seekRequestPositionMilliseconds = -1;
    qint64 m_seekRequestedAtMilliseconds = -1;
    qint64 m_backendRemovalRequestedAtMilliseconds = -1;
    qint64 m_backendRemovalStableSinceMilliseconds = -1;
    qsizetype m_deviceSnapshotCount = 0;
    qsizetype m_deviceSettlementCount = 0;
    qsizetype m_audioInputSignalCount = 0;
    qsizetype m_audioOutputSignalCount = 0;
    double m_browserSeekRequestMonotonicMilliseconds = -1.0;
    bool m_started = false;
    bool m_failed = false;
    bool m_metadataLoaded = false;
    bool m_playingObserved = false;
    bool m_capturePauseRequested = false;
    bool m_capturePaused = false;
    bool m_captureReady = false;
    bool m_captureAcknowledged = false;
    bool m_seekRequested = false;
    bool m_seekObserved = false;
    bool m_resumeRequested = false;
    bool m_resumeObserved = false;
    bool m_naturalEndObserved = false;
    bool m_backendRemovalRecorded = false;
    bool m_playerDestructionRecorded = false;
    bool m_audioOutputDestructionRecorded = false;
    bool m_teardownCompleted = false;
};
