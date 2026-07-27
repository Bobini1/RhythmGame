#include "MediaProbe.h"

#include "BrowserRuntimeBridge.h"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>

#include <cmath>
#include <utility>

namespace
{
const QRegularExpression mediaElementIdPattern{
    QStringLiteral("^media-element-[1-9][0-9]*$")};

bool isValidMediaRemovalArmSnapshot(
    const QJsonObject &snapshot,
    quint32 runNonce)
{
    return snapshot.keys() == QStringList{
            QStringLiteral("cleanupArmed"),
            QStringLiteral("elementId"),
            QStringLiteral("exactSourceMatched"),
            QStringLiteral("hadResourceBeforeDestruction"),
            QStringLiteral("matchingElementCount"),
            QStringLiteral("observerMutatedElement"),
            QStringLiteral("runNonce"),
            QStringLiteral("sourcePathAndQuery"),
            QStringLiteral("trackedMediaElementCount"),
            QStringLiteral("wasConnected")}
        && snapshot.value(u"cleanupArmed").toBool()
        && mediaElementIdPattern.match(
            snapshot.value(u"elementId").toString()).hasMatch()
        && snapshot.value(u"exactSourceMatched").toBool()
        && snapshot.value(u"hadResourceBeforeDestruction").toBool()
        && snapshot.value(u"matchingElementCount").toInt() == 1
        && !snapshot.value(u"observerMutatedElement").toBool()
        && snapshot.value(u"runNonce").toInteger()
            == static_cast<qint64>(runNonce)
        && snapshot.value(u"sourcePathAndQuery").toString()
            == QStringLiteral("/fixtures/probe.webm?nonce=")
                + QString::number(runNonce)
        && snapshot.value(u"trackedMediaElementCount").toInt() >= 1
        && snapshot.value(u"wasConnected").toBool();
}

bool isValidMediaRemovalSnapshot(
    const QJsonObject &snapshot,
    quint32 runNonce,
    bool requireMediaElementResourceReleased)
{
    const bool mediaElementResourceReleased =
        snapshot.value(u"mediaElementResourceReleased").toBool();
    const bool networkStateEmpty =
        snapshot.value(u"networkStateEmpty").toBool();
    const bool readyStateEmpty =
        snapshot.value(u"readyStateEmpty").toBool();
    return snapshot.keys() == QStringList{
            QStringLiteral("cleanupArmed"),
            QStringLiteral("currentSourceCleared"),
            QStringLiteral("currentSourceOwnedOrEmpty"),
            QStringLiteral("currentTimeReset"),
            QStringLiteral("documentOwnedElementCount"),
            QStringLiteral("domRemoved"),
            QStringLiteral("durationCleared"),
            QStringLiteral("elementId"),
            QStringLiteral("emptiedObserved"),
            QStringLiteral("exactSourceMatched"),
            QStringLiteral("hadResourceBeforeDestruction"),
            QStringLiteral("matchingElementCount"),
            QStringLiteral("mediaElementResourceReleased"),
            QStringLiteral("mediaErrorCleared"),
            QStringLiteral("networkStateEmpty"),
            QStringLiteral("observerMutatedElement"),
            QStringLiteral("paused"),
            QStringLiteral("readyStateEmpty"),
            QStringLiteral("runNonce"),
            QStringLiteral("seekingStopped"),
            QStringLiteral("sourceAttributeCleared"),
            QStringLiteral("sourceElementCount"),
            QStringLiteral("sourceObjectCleared"),
            QStringLiteral("sourcePathAndQuery"),
            QStringLiteral("sourcePropertyCleared"),
            QStringLiteral("trackedMediaElementCount"),
            QStringLiteral("wasConnected")}
        && snapshot.value(u"cleanupArmed").toBool()
        && snapshot.value(u"currentSourceCleared").isBool()
        && snapshot.value(u"currentSourceOwnedOrEmpty").toBool()
        && snapshot.value(u"currentTimeReset").toBool()
        && snapshot.value(u"documentOwnedElementCount").toInt() == 0
        && snapshot.value(u"domRemoved").toBool()
        && snapshot.value(u"durationCleared").toBool()
        && mediaElementIdPattern.match(
            snapshot.value(u"elementId").toString()).hasMatch()
        && snapshot.value(u"emptiedObserved").isBool()
        && snapshot.value(u"exactSourceMatched").toBool()
        && snapshot.value(u"hadResourceBeforeDestruction").toBool()
        && snapshot.value(u"matchingElementCount").toInt() == 1
        && snapshot.value(u"mediaElementResourceReleased").isBool()
        && snapshot.value(u"mediaErrorCleared").toBool()
        && snapshot.value(u"networkStateEmpty").isBool()
        && !snapshot.value(u"observerMutatedElement").toBool()
        && snapshot.value(u"paused").toBool()
        && snapshot.value(u"readyStateEmpty").isBool()
        && snapshot.value(u"runNonce").toInteger()
            == static_cast<qint64>(runNonce)
        && snapshot.value(u"seekingStopped").toBool()
        && snapshot.value(u"sourceAttributeCleared").toBool()
        && snapshot.value(u"sourceElementCount").toInt() == 0
        && snapshot.value(u"sourceObjectCleared").toBool()
        && snapshot.value(u"sourcePathAndQuery").toString()
            == QStringLiteral("/fixtures/probe.webm?nonce=")
                + QString::number(runNonce)
        && snapshot.value(u"sourcePropertyCleared").toBool()
        && snapshot.value(u"trackedMediaElementCount").toInt() >= 1
        && snapshot.value(u"wasConnected").toBool()
        && mediaElementResourceReleased
            == (snapshot.value(u"emptiedObserved").toBool()
                && networkStateEmpty
                && readyStateEmpty)
        && (!requireMediaElementResourceReleased
            || mediaElementResourceReleased);
}

bool hasStableMediaRemovalIdentity(
    const QJsonObject &initial,
    const QJsonObject &current)
{
    return initial.value(u"cleanupArmed")
            == current.value(u"cleanupArmed")
        && initial.value(u"elementId")
            == current.value(u"elementId")
        && initial.value(u"exactSourceMatched")
            == current.value(u"exactSourceMatched")
        && initial.value(u"hadResourceBeforeDestruction")
            == current.value(u"hadResourceBeforeDestruction")
        && initial.value(u"matchingElementCount")
            == current.value(u"matchingElementCount")
        && initial.value(u"observerMutatedElement")
            == current.value(u"observerMutatedElement")
        && initial.value(u"runNonce")
            == current.value(u"runNonce")
        && initial.value(u"sourcePathAndQuery")
            == current.value(u"sourcePathAndQuery")
        && initial.value(u"trackedMediaElementCount")
            == current.value(u"trackedMediaElementCount")
        && initial.value(u"wasConnected")
            == current.value(u"wasConnected");
}

QJsonArray encodeAudioDevices(const QList<QAudioDevice> &devices)
{
    QJsonArray encoded;
    for (const QAudioDevice &device : devices) {
        const QByteArray idSha256 =
            QCryptographicHash::hash(
                device.id(),
                QCryptographicHash::Sha256).toHex();
        encoded.append(QJsonObject{
            {QStringLiteral("idSha256"),
             QString::fromLatin1(idSha256)},
            {QStringLiteral("isDefault"), device.isDefault()},
        });
    }
    return encoded;
}
}

MediaProbe::MediaProbe(
    EventCallback eventCallback,
    CompletionCallback completionCallback,
    FailureCallback failureCallback,
    QObject *parent)
    : QObject{parent}
    , m_eventCallback{std::move(eventCallback)}
    , m_completionCallback{std::move(completionCallback)}
    , m_failureCallback{std::move(failureCallback)}
    , m_timeout{new QTimer{this}}
    , m_deviceSettleTimer{new QTimer{this}}
    , m_seekProofPoll{new QTimer{this}}
    , m_seekTimeout{new QTimer{this}}
    , m_backendRemovalPoll{new QTimer{this}}
{
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        fail(
            u"qt-media-timeout",
            u"served media did not reach natural end and clean teardown");
    });
    m_deviceSettleTimer->setSingleShot(true);
    connect(
        m_deviceSettleTimer,
        &QTimer::timeout,
        this,
        &MediaProbe::publishDeviceBatchSettled);
    m_seekProofPoll->setInterval(seekProofPollInterval);
    connect(
        m_seekProofPoll,
        &QTimer::timeout,
        this,
        &MediaProbe::pollSeekProof);
    m_seekTimeout->setSingleShot(true);
    connect(m_seekTimeout, &QTimer::timeout, this, [this] {
        fail(
            u"qt-media-seek-timeout",
            u"nonce-owned media did not emit a bounded seeking/seeked pair");
    });
    m_backendRemovalPoll->setInterval(backendRemovalPollInterval);
    connect(
        m_backendRemovalPoll,
        &QTimer::timeout,
        this,
        &MediaProbe::finishBackendRemoval);
}

bool MediaProbe::armDeviceObservation(quint32 runNonce)
{
    if (m_mediaDevices != nullptr
        || m_started
        || runNonce == 0
        || runNonce == 0xFFFFFFFFU) {
        return false;
    }
    m_runNonce = runNonce;
    m_mediaDevices = new QMediaDevices{this};
    connect(
        m_mediaDevices,
        &QMediaDevices::audioInputsChanged,
        this,
        [this] {
            ++m_audioInputSignalCount;
            publishDeviceSnapshot(u"audio-inputs-changed");
        });
    connect(
        m_mediaDevices,
        &QMediaDevices::audioOutputsChanged,
        this,
        [this] {
            ++m_audioOutputSignalCount;
            publishDeviceSnapshot(u"audio-outputs-changed");
        });
    publishDeviceSnapshot(u"initial");
    return !m_failed;
}

bool MediaProbe::attachVideoSink(QVideoSink *videoSink)
{
    if (videoSink == nullptr
        || !m_videoSink.isNull()
        || m_started) {
        return false;
    }
    m_videoSink = videoSink;
    append(u"qt-media-video-sink-attached");
    return true;
}

void MediaProbe::start(quint32 runNonce)
{
    if (m_started
        || runNonce == 0
        || runNonce == 0xFFFFFFFFU
        || runNonce != m_runNonce
        || m_mediaDevices == nullptr
        || m_videoSink.isNull()) {
        fail(
            u"qt-media-start-invalid",
            u"media must start once with an attached QML VideoOutput sink");
        return;
    }
    m_started = true;
    m_elapsed.start();
    if (!BrowserRuntimeBridge::beginOwnedMediaBackendTracking(
            m_runNonce)) {
        fail(
            u"qt-media-backend-tracker",
            u"browser media backend tracking could not be armed");
        return;
    }

    createPlayer();
    const QUrl source = BrowserRuntimeBridge::sameOriginUrl(
        QStringLiteral("/fixtures/probe.webm?nonce=")
        + QString::number(m_runNonce));
    m_player->setSource(source);
    append(
        u"qt-media-source-set",
        QJsonObject{
            {QStringLiteral("path"), source.path()},
            {QStringLiteral("query"), source.query()},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    m_timeout->start(mediaTimeout);
    m_player->play();
}

QJsonObject MediaProbe::acknowledgeVisualCapture(
    quint32 runNonce,
    const QStringList &requestIds)
{
    if (m_failed
        || !m_captureReady
        || !m_capturePaused
        || m_captureAcknowledged
        || m_naturalEndObserved
        || runNonce != m_runNonce
        || m_player == nullptr
        || m_player->playbackState()
            != QMediaPlayer::PausedState) {
        return {};
    }

    m_seekRequestPositionMilliseconds = m_player->position();
    const QJsonObject seekArm =
        BrowserRuntimeBridge::armOwnedMediaSeekTracking(
            m_runNonce,
            requiredSeekPositionMilliseconds);
    const QStringList expectedSeekArmKeys{
        QStringLiteral("elementId"),
        QStringLiteral("preSeekPositionMilliseconds"),
        QStringLiteral("requestMonotonicMilliseconds"),
        QStringLiteral("runNonce"),
        QStringLiteral("targetPositionMilliseconds"),
    };
    const double browserPreSeekPosition =
        seekArm.value(u"preSeekPositionMilliseconds").toDouble(-1.0);
    const double browserRequestMonotonic =
        seekArm.value(u"requestMonotonicMilliseconds").toDouble(-1.0);
    if (seekArm.keys() != expectedSeekArmKeys
        || !mediaElementIdPattern.match(
            seekArm.value(u"elementId").toString()).hasMatch()
        || !std::isfinite(browserPreSeekPosition)
        || browserPreSeekPosition < 0.0
        || !std::isfinite(browserRequestMonotonic)
        || browserRequestMonotonic < 0.0
        || seekArm.value(u"runNonce").toInteger()
            != static_cast<qint64>(m_runNonce)
        || seekArm.value(u"targetPositionMilliseconds").toInteger()
            != requiredSeekPositionMilliseconds
        || std::abs(
            browserPreSeekPosition
            - static_cast<double>(
                m_seekRequestPositionMilliseconds))
            > maximumSeekArmPositionSkewMilliseconds
        || std::abs(
            browserPreSeekPosition
            - static_cast<double>(
                requiredSeekPositionMilliseconds))
            < minimumSeekJumpMilliseconds) {
        fail(
            u"qt-media-seek-arm",
            u"seek tracker did not bind the exact nonce-owned media element");
        return {};
    }

    m_captureAcknowledged = true;
    m_requestIds = requestIds;
    m_seekElementId = seekArm.value(u"elementId").toString();
    m_browserSeekRequestMonotonicMilliseconds =
        browserRequestMonotonic;
    append(
        u"qt-media-capture-acknowledged",
        QJsonObject{
            {QStringLiteral("requestIds"),
             QJsonArray::fromStringList(m_requestIds)},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    m_seekRequested = true;
    m_seekRequestedAtMilliseconds = m_elapsed.elapsed();
    append(
        u"qt-media-seek-requested",
        QJsonObject{
            {QStringLiteral("elementId"), m_seekElementId},
            {QStringLiteral("preSeekPositionMilliseconds"),
             browserPreSeekPosition},
            {QStringLiteral("positionMilliseconds"),
             requiredSeekPositionMilliseconds},
            {QStringLiteral("requestMonotonicMilliseconds"),
             m_browserSeekRequestMonotonicMilliseconds},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    m_seekTimeout->start(seekResponseTimeout);
    m_seekProofPoll->start();
    m_player->setPosition(requiredSeekPositionMilliseconds);
    pollSeekProof();
    return QJsonObject{
        {QStringLiteral("command"),
         browserRuntimeCommandName(
             BrowserRuntimeCommand::AcknowledgeMediaFrameCapture)
             .toString()},
        {QStringLiteral("elementId"), m_seekElementId},
        {QStringLiteral("preSeekPositionMilliseconds"),
         browserPreSeekPosition},
        {QStringLiteral("requestMonotonicMilliseconds"),
         m_browserSeekRequestMonotonicMilliseconds},
        {QStringLiteral("requestIds"),
         QJsonArray::fromStringList(m_requestIds)},
        {QStringLiteral("runNonce"),
         static_cast<qint64>(m_runNonce)},
        {QStringLiteral("seekPositionMilliseconds"),
         requiredSeekPositionMilliseconds},
    };
}

void MediaProbe::createPlayer()
{
    m_player = new QMediaPlayer{this};
    m_audioOutput = new QAudioOutput{this};
    m_audioOutput->setMuted(true);
    m_player->setAudioOutput(m_audioOutput);
    m_player->setVideoSink(m_videoSink);
    const QAudioDevice selectedOutput = m_audioOutput->device();
    const QByteArray selectedOutputIdSha256 =
        QCryptographicHash::hash(
            selectedOutput.id(),
            QCryptographicHash::Sha256).toHex();
    append(
        u"qt-media-player-output-created",
        QJsonObject{
            {QStringLiteral("audioOutputConstructed"), true},
            {QStringLiteral("mediaPlayerConstructed"), true},
            {QStringLiteral("outputDeviceIdSha256"),
             QString::fromLatin1(selectedOutputIdSha256)},
            {QStringLiteral("outputDeviceIsDefault"),
             selectedOutput.isDefault()},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });

    connect(
        m_player,
        &QMediaPlayer::mediaStatusChanged,
        this,
        [this](QMediaPlayer::MediaStatus status) {
            mediaStatusChanged(static_cast<int>(status));
        });
    connect(
        m_player,
        &QMediaPlayer::playbackStateChanged,
        this,
        [this](QMediaPlayer::PlaybackState state) {
            playbackStateChanged(static_cast<int>(state));
        });
    connect(
        m_player,
        &QMediaPlayer::positionChanged,
        this,
        &MediaProbe::positionChanged);
    connect(
        m_player,
        &QMediaPlayer::errorOccurred,
        this,
        [this](QMediaPlayer::Error, const QString &errorString) {
            fail(u"qt-media-error", errorString);
        });
    connect(
        m_videoSink,
        &QVideoSink::videoFrameChanged,
        this,
        &MediaProbe::videoFrameChanged);
}

void MediaProbe::publishDeviceSnapshot(QStringView reason)
{
    if (m_failed) {
        return;
    }
    if (m_deviceSnapshotCount >= maximumDeviceSnapshots) {
        fail(
            u"qt-media-device-change-overflow",
            u"browser media devices changed more often than the bounded probe");
        return;
    }
    ++m_deviceSnapshotCount;
    append(
        u"qt-media-device-snapshot",
        QJsonObject{
            {QStringLiteral("audioInputSignalCount"),
             static_cast<qint64>(m_audioInputSignalCount)},
            {QStringLiteral("audioInputs"),
             encodeAudioDevices(QMediaDevices::audioInputs())},
            {QStringLiteral("audioOutputSignalCount"),
             static_cast<qint64>(m_audioOutputSignalCount)},
            {QStringLiteral("audioOutputs"),
             encodeAudioDevices(QMediaDevices::audioOutputs())},
            {QStringLiteral("ordinal"),
             static_cast<qint64>(m_deviceSnapshotCount)},
            {QStringLiteral("reason"), reason.toString()},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    m_deviceSettleTimer->start();
}

void MediaProbe::publishDeviceBatchSettled()
{
    if (m_failed) {
        return;
    }
    if (m_deviceSettlementCount >= maximumDeviceSettlements) {
        fail(
            u"qt-media-device-settlement-overflow",
            u"browser media-device batches settled more often than allowed");
        return;
    }
    ++m_deviceSettlementCount;
    append(
        u"qt-media-device-batch-settled",
        QJsonObject{
            {QStringLiteral("audioInputSignalCount"),
             static_cast<qint64>(m_audioInputSignalCount)},
            {QStringLiteral("audioInputs"),
             encodeAudioDevices(QMediaDevices::audioInputs())},
            {QStringLiteral("audioOutputSignalCount"),
             static_cast<qint64>(m_audioOutputSignalCount)},
            {QStringLiteral("audioOutputs"),
             encodeAudioDevices(QMediaDevices::audioOutputs())},
            {QStringLiteral("settledOrdinal"),
             static_cast<qint64>(m_deviceSettlementCount)},
            {QStringLiteral("snapshotOrdinal"),
             static_cast<qint64>(m_deviceSnapshotCount)},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
}

void MediaProbe::mediaStatusChanged(int statusValue)
{
    if (m_failed || m_player == nullptr) {
        return;
    }
    const auto status =
        static_cast<QMediaPlayer::MediaStatus>(statusValue);
    if (status == QMediaPlayer::LoadedMedia && !m_metadataLoaded) {
        m_metadataLoaded = true;
        append(
            u"qt-media-metadata-loaded",
            QJsonObject{
                {QStringLiteral("durationMilliseconds"),
                 m_player->duration()},
                {QStringLiteral("hasVideo"), m_player->hasVideo()},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
    }
    if (status == QMediaPlayer::InvalidMedia) {
        fail(
            u"qt-media-invalid",
            u"Qt Multimedia rejected the served WebM fixture");
        return;
    }
    if (status == QMediaPlayer::EndOfMedia) {
        QTimer::singleShot(
            0,
            this,
            &MediaProbe::handleNaturalEnd);
    }
}

void MediaProbe::playbackStateChanged(int stateValue)
{
    if (m_failed || m_player == nullptr) {
        return;
    }
    const auto state =
        static_cast<QMediaPlayer::PlaybackState>(stateValue);
    if (state == QMediaPlayer::PlayingState
        && !m_playingObserved) {
        m_playingObserved = true;
        append(
            u"qt-media-playing",
            QJsonObject{{
                QStringLiteral("runNonce"),
                static_cast<qint64>(m_runNonce)}});
    }
    if (state == QMediaPlayer::PausedState
        && m_capturePauseRequested
        && !m_capturePaused) {
        m_capturePaused = true;
        m_captureReady = true;
        m_capturePausedPositionMilliseconds = m_player->position();
        append(
            u"qt-media-capture-ready",
            QJsonObject{
                {QStringLiteral("framePositionSamples"),
                 m_framePositionSamples},
                {QStringLiteral("paused"), true},
                {QStringLiteral("positionMilliseconds"),
                 m_capturePausedPositionMilliseconds},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
    }
    if (state == QMediaPlayer::PlayingState
        && m_resumeRequested
        && !m_resumeObserved) {
        m_resumePositionMilliseconds = m_player->position();
        if (std::abs(
                m_resumePositionMilliseconds
                - requiredSeekPositionMilliseconds)
            > maximumSeekTargetErrorMilliseconds) {
            fail(
                u"qt-media-resume-position",
                u"playback resumed outside the proven seek target");
            return;
        }
        m_resumeObserved = true;
        append(
            u"qt-media-playback-resumed",
            QJsonObject{
                {QStringLiteral("positionMilliseconds"),
                 m_resumePositionMilliseconds},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
    }
}

void MediaProbe::videoFrameChanged(const QVideoFrame &frame)
{
    if (m_failed
        || m_player == nullptr
        || !frame.isValid()) {
        return;
    }
    const qint64 position = m_player->position();
    if (position <= m_lastFramePosition) {
        return;
    }
    m_lastFramePosition = position;
    if (m_framePositionSamples.size() < maximumFramePositionSamples) {
        m_framePositionSamples.append(position);
        append(
            u"qt-media-video-frame",
            QJsonObject{
                {QStringLiteral("ordinal"),
                 m_framePositionSamples.size()},
                {QStringLiteral("positionMilliseconds"), position},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
    } else {
        m_framePositionSamples.replace(
            maximumFramePositionSamples - 1,
            position);
    }
    if (!m_capturePauseRequested
        && m_framePositionSamples.size() >= 2
        && position >= requiredPositionAdvanceMilliseconds) {
        m_capturePauseRequested = true;
        append(
            u"qt-media-capture-pause-requested",
            QJsonObject{
                {QStringLiteral("framePositionSamples"),
                 m_framePositionSamples},
                {QStringLiteral("positionMilliseconds"),
                 position},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
        m_player->pause();
    }
    if (m_seekRequested
        && m_resumeObserved
        && position >= requiredSeekPositionMilliseconds
        && (m_postSeekFramePositionSamples.isEmpty()
            || position
                > m_postSeekFramePositionSamples.at(
                    m_postSeekFramePositionSamples.size() - 1).toInteger())) {
        if (m_postSeekFramePositionSamples.size()
            < maximumPostSeekFramePositionSamples) {
            m_postSeekFramePositionSamples.append(position);
            append(
                u"qt-media-post-seek-video-frame",
                QJsonObject{
                    {QStringLiteral("ordinal"),
                     m_postSeekFramePositionSamples.size()},
                    {QStringLiteral("positionMilliseconds"), position},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
        } else {
            m_postSeekFramePositionSamples.replace(
                maximumPostSeekFramePositionSamples - 1,
                position);
        }
    }
}

void MediaProbe::positionChanged(qint64)
{
    if (m_failed || !m_seekRequested || m_seekObserved) {
        return;
    }
    pollSeekProof();
}

void MediaProbe::pollSeekProof()
{
    if (m_failed || !m_seekRequested || m_seekObserved) {
        return;
    }
    const QJsonObject proof =
        BrowserRuntimeBridge::finishOwnedMediaSeekTracking(
            m_runNonce);
    if (proof.isEmpty()) {
        return;
    }
    const QStringList expectedKeys{
        QStringLiteral("elementId"),
        QStringLiteral("preSeekPositionMilliseconds"),
        QStringLiteral("requestMonotonicMilliseconds"),
        QStringLiteral("runNonce"),
        QStringLiteral("seekedMonotonicMilliseconds"),
        QStringLiteral("seekedPositionMilliseconds"),
        QStringLiteral("seekingMonotonicMilliseconds"),
        QStringLiteral("seekingPositionMilliseconds"),
        QStringLiteral("targetPositionMilliseconds"),
    };
    const double preSeekPosition =
        proof.value(u"preSeekPositionMilliseconds").toDouble(-1.0);
    const double requestMonotonic =
        proof.value(u"requestMonotonicMilliseconds").toDouble(-1.0);
    const double seekingMonotonic =
        proof.value(u"seekingMonotonicMilliseconds").toDouble(-1.0);
    const double seekingPosition =
        proof.value(u"seekingPositionMilliseconds").toDouble(-1.0);
    const double seekedMonotonic =
        proof.value(u"seekedMonotonicMilliseconds").toDouble(-1.0);
    const double seekedPosition =
        proof.value(u"seekedPositionMilliseconds").toDouble(-1.0);
    const double responseMilliseconds =
        seekedMonotonic - requestMonotonic;
    const double jumpMilliseconds =
        std::abs(seekedPosition - preSeekPosition);
    const double targetErrorMilliseconds =
        std::abs(
            seekedPosition
            - static_cast<double>(
                requiredSeekPositionMilliseconds));
    const qint64 nativeResponseElapsedMilliseconds =
        m_elapsed.elapsed() - m_seekRequestedAtMilliseconds;
    const auto responseLimit =
        seekResponseTimeout / std::chrono::milliseconds{1};
    if (proof.keys() != expectedKeys
        || proof.value(u"elementId").toString() != m_seekElementId
        || proof.value(u"runNonce").toInteger()
            != static_cast<qint64>(m_runNonce)
        || proof.value(u"targetPositionMilliseconds").toInteger()
            != requiredSeekPositionMilliseconds
        || !std::isfinite(preSeekPosition)
        || !std::isfinite(requestMonotonic)
        || !std::isfinite(seekingMonotonic)
        || !std::isfinite(seekingPosition)
        || !std::isfinite(seekedMonotonic)
        || !std::isfinite(seekedPosition)
        || requestMonotonic
            != m_browserSeekRequestMonotonicMilliseconds
        || std::abs(
            preSeekPosition
            - static_cast<double>(
                m_seekRequestPositionMilliseconds))
            > maximumSeekArmPositionSkewMilliseconds
        || seekingMonotonic < requestMonotonic
        || seekedMonotonic < seekingMonotonic
        || responseMilliseconds
            > responseLimit
        || nativeResponseElapsedMilliseconds < 0
        || nativeResponseElapsedMilliseconds
            > responseLimit
        || jumpMilliseconds < minimumSeekJumpMilliseconds
        || targetErrorMilliseconds
            > maximumSeekTargetErrorMilliseconds) {
        fail(
            u"qt-media-seek-proof",
            u"seeking/seeked acknowledgement was unowned or out of bounds");
        return;
    }
    m_seekObserved = true;
    m_seekProofPoll->stop();
    m_seekTimeout->stop();
    m_seekProof = proof;
    m_seekProof.insert(
        QStringLiteral("jumpMilliseconds"),
        jumpMilliseconds);
    m_seekProof.insert(
        QStringLiteral("nativeResponseElapsedMilliseconds"),
        nativeResponseElapsedMilliseconds);
    m_seekProof.insert(
        QStringLiteral("responseMilliseconds"),
        responseMilliseconds);
    m_seekProof.insert(
        QStringLiteral("targetErrorMilliseconds"),
        targetErrorMilliseconds);
    append(u"qt-media-seek-observed", m_seekProof);
    m_resumeRequested = true;
    append(
        u"qt-media-resume-requested",
        QJsonObject{
            {QStringLiteral("positionMilliseconds"),
             m_player->position()},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    m_player->play();
}

void MediaProbe::handleNaturalEnd()
{
    if (m_failed || m_naturalEndObserved || m_player == nullptr) {
        return;
    }
    const qint64 durationMilliseconds = m_player->duration();
    const qint64 endPositionMilliseconds = m_player->position();
    const qint64 finalPostSeekFramePosition =
        m_postSeekFramePositionSamples.isEmpty()
        ? -1
        : m_postSeekFramePositionSamples.at(
            m_postSeekFramePositionSamples.size() - 1).toInteger();
    if (!m_metadataLoaded
        || !m_playingObserved
        || !m_captureReady
        || !m_capturePaused
        || !m_captureAcknowledged
        || !m_seekRequested
        || !m_seekObserved
        || !m_resumeRequested
        || !m_resumeObserved
        || std::abs(
            m_resumePositionMilliseconds
            - requiredSeekPositionMilliseconds)
            > maximumSeekTargetErrorMilliseconds
        || m_seekProof.isEmpty()
        || m_framePositionSamples.size() < 2
        || m_lastFramePosition
            < requiredPositionAdvanceMilliseconds
        || m_postSeekFramePositionSamples.size() < 2
        || finalPostSeekFramePosition
            < m_resumePositionMilliseconds
                + requiredPostSeekAdvanceMilliseconds
        || durationMilliseconds <= 0
        || endPositionMilliseconds
            < durationMilliseconds
                - maximumEndPositionErrorMilliseconds
        || endPositionMilliseconds
            > durationMilliseconds
                + maximumEndPositionErrorMilliseconds) {
        fail(
            u"qt-media-natural-end-prerequisite",
            u"natural end lacked post-seek playback or bounded end position");
        return;
    }
    m_naturalEndObserved = true;
    m_seekProofPoll->stop();
    m_seekTimeout->stop();
    m_completionCallback(
        u"qt-media-natural-end",
        QJsonObject{
            {QStringLiteral("durationMilliseconds"),
             durationMilliseconds},
            {QStringLiteral("endPositionMilliseconds"),
             endPositionMilliseconds},
            {QStringLiteral("framePositionSamples"),
             m_framePositionSamples},
            {QStringLiteral("postSeekFramePositionSamples"),
             m_postSeekFramePositionSamples},
            {QStringLiteral("requestIds"),
             QJsonArray::fromStringList(m_requestIds)},
            {QStringLiteral("resumeObserved"), m_resumeObserved},
            {QStringLiteral("resumePositionMilliseconds"),
             m_resumePositionMilliseconds},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
            {QStringLiteral("seekObserved"), m_seekObserved},
            {QStringLiteral("seekProof"), m_seekProof},
        });
    armBackendRemovalAndDestroy();
}

void MediaProbe::armBackendRemovalAndDestroy()
{
    if (m_player == nullptr) {
        fail(
            u"qt-media-player-missing",
            u"media player disappeared before backend removal");
        return;
    }
    m_backendRemovalArmRecord =
        BrowserRuntimeBridge::armOwnedMediaBackendRemoval(
            m_runNonce);
    if (!isValidMediaRemovalArmSnapshot(
            m_backendRemovalArmRecord,
            m_runNonce)) {
        fail(
            u"qt-media-backend-removal-arm",
            u"exact nonce-owned browser media source could not be "
            u"armed for observation-only destruction validation");
        return;
    }
    append(
        u"qt-media-backend-removal-armed",
        m_backendRemovalArmRecord);
    m_player->setVideoSink(nullptr);
    m_player->setAudioOutput(nullptr);

    QMediaPlayer *const player = m_player;
    QAudioOutput *const audioOutput = m_audioOutput;
    connect(player, &QObject::destroyed, this, [this] {
        m_playerDestructionRecorded = true;
        append(
            u"qt-media-player-destroyed",
            QJsonObject{{
                QStringLiteral("runNonce"),
                static_cast<qint64>(m_runNonce)}});
    });
    connect(audioOutput, &QObject::destroyed, this, [this] {
        m_audioOutputDestructionRecorded = true;
        append(
            u"qt-media-audio-output-destroyed",
            QJsonObject{{
                QStringLiteral("runNonce"),
                static_cast<qint64>(m_runNonce)}});
        finishObjectTeardownIfReady();
    });
    m_player = nullptr;
    m_audioOutput = nullptr;
    m_backendRemovalRequestedAtMilliseconds = m_elapsed.elapsed();
    m_backendRemovalPoll->start();
    player->deleteLater();
    audioOutput->deleteLater();
}

void MediaProbe::finishBackendRemoval()
{
    if (m_failed) {
        return;
    }
    const qint64 responseElapsedMilliseconds =
        m_elapsed.elapsed()
        - m_backendRemovalRequestedAtMilliseconds;
    if (m_backendRemovalRequestedAtMilliseconds < 0
        || responseElapsedMilliseconds
            > std::chrono::duration_cast<std::chrono::milliseconds>(
                backendRemovalResponseTimeout).count()) {
        fail(
            u"qt-media-backend-removal-timeout",
            u"Qt media backend destruction and browser media-element "
            u"resource release did not complete within one second");
        return;
    }
    if (!m_playerDestructionRecorded) {
        return;
    }
    const QJsonObject backend =
        BrowserRuntimeBridge::finishOwnedMediaBackendRemoval(
            m_runNonce);
    const bool validRemovalSnapshot = isValidMediaRemovalSnapshot(
            backend,
            m_runNonce,
            false);
    const bool stableRemovalIdentity = hasStableMediaRemovalIdentity(
            m_backendRemovalArmRecord,
            backend);
    if (!validRemovalSnapshot || !stableRemovalIdentity) {
        fail(
            u"qt-media-backend-removal",
            QStringLiteral(
                "browser media removal validation failed "
                "(validSnapshot=%1, stableIdentity=%2)")
                .arg(validRemovalSnapshot)
                .arg(stableRemovalIdentity));
        return;
    }
    if (!backend.value(u"mediaElementResourceReleased").toBool()) {
        m_backendRemovalStableSinceMilliseconds = -1;
        return;
    }
    const qint64 nowMilliseconds = m_elapsed.elapsed();
    if (m_backendRemovalStableSinceMilliseconds < 0) {
        m_backendRemovalStableSinceMilliseconds = nowMilliseconds;
        return;
    }
    if (nowMilliseconds - m_backendRemovalStableSinceMilliseconds
        < std::chrono::duration_cast<std::chrono::milliseconds>(
            backendRemovalStabilityWindow).count()) {
        return;
    }
    const QJsonObject releasedBackend =
        BrowserRuntimeBridge::releaseOwnedMediaBackendRemoval(
            m_runNonce);
    if (!isValidMediaRemovalSnapshot(
            releasedBackend,
            m_runNonce,
            true)
        || !hasStableMediaRemovalIdentity(
            m_backendRemovalArmRecord,
            releasedBackend)
        || releasedBackend != backend) {
        fail(
            u"qt-media-backend-release",
            u"validated browser media element could not be released");
        return;
    }
    m_backendRemovalPoll->stop();
    m_backendRemovalRecorded = true;
    append(u"qt-media-element-resource-released", releasedBackend);
    append(u"qt-media-backend-removed", releasedBackend);
    finishObjectTeardownIfReady();
}

void MediaProbe::finishObjectTeardownIfReady()
{
    if (m_failed
        || m_teardownCompleted
        || !m_backendRemovalRecorded
        || !m_playerDestructionRecorded
        || !m_audioOutputDestructionRecorded) {
        return;
    }
    m_teardownCompleted = true;
    m_timeout->stop();
    m_completionCallback(
        u"qt-media-clean-teardown",
        QJsonObject{
            {QStringLiteral("audioOutputDestructionRecorded"),
             m_audioOutputDestructionRecorded},
            {QStringLiteral("backendRemovalRecorded"),
             m_backendRemovalRecorded},
            {QStringLiteral("playerDestructionRecorded"),
             m_playerDestructionRecorded},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
}

void MediaProbe::append(QStringView type, QJsonObject payload)
{
    if (m_eventCallback) {
        m_eventCallback(type, std::move(payload));
    }
}

void MediaProbe::fail(QStringView code, QStringView detail)
{
    if (m_failed || m_teardownCompleted) {
        return;
    }
    m_failed = true;
    m_timeout->stop();
    m_deviceSettleTimer->stop();
    m_seekProofPoll->stop();
    m_seekTimeout->stop();
    m_backendRemovalPoll->stop();
    if (m_player != nullptr) {
        m_player->setVideoSink(nullptr);
        m_player->setAudioOutput(nullptr);
        m_player->deleteLater();
        m_player = nullptr;
    }
    if (m_audioOutput != nullptr) {
        m_audioOutput->deleteLater();
        m_audioOutput = nullptr;
    }
    if (m_runNonce != 0) {
        static_cast<void>(
            BrowserRuntimeBridge::finishOwnedMediaBackendRemoval(
                m_runNonce,
                true));
    }
    if (m_failureCallback) {
        m_failureCallback(code, detail);
    }
}
