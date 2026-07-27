#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStringView>
#include <QUrl>
#include <QtTypes>

#include <cstdint>
#include <functional>

struct BrowserCapabilities
{
    bool secureContext;
    bool crossOriginIsolated;
    bool sharedArrayBuffer;
    bool jspiApi;
    bool webGl2Api;
    bool audioWorklet;
    bool opfs;
    bool fileSystemAccess;
};

enum class BrowserRuntimeCommand : quint8
{
    AcknowledgeMediaFrameCapture,
    ArmBfcacheResumeProbe,
    ArmHiddenTimerProbe,
    ArmVisibleResumeTimerProbe,
    BeginForegroundLatencySampling,
    ProbePing,
    SetShaderPhase,
    TriggerNativeDepthLimit,
    TriggerNativeSuspensionTrap,
};

using BrowserRuntimeCommandHandler =
    std::function<QJsonObject(
        BrowserRuntimeCommand,
        const QJsonObject &)>;

[[nodiscard]] BrowserCapabilities browserCapabilities();
[[nodiscard]] QStringView browserRuntimeCommandName(
    BrowserRuntimeCommand command);
[[nodiscard]] bool publishGate1bEvent(const QJsonObject &event);
[[nodiscard]] bool publishGate1bSnapshot(const QJsonObject &snapshot);
[[nodiscard]] bool resolveGate1bReady(const QJsonObject &snapshot);
void rejectGate1bReady(QStringView code, const QJsonObject &detail);

namespace BrowserRuntimeBridge
{
[[nodiscard]] QUrl sameOriginUrl(QStringView pathAndQuery);
[[nodiscard]] bool beginOwnedMediaBackendTracking(quint32 runNonce);
[[nodiscard]] QJsonObject armOwnedMediaSeekTracking(
    quint32 runNonce,
    qint64 targetPositionMilliseconds);
[[nodiscard]] QJsonObject finishOwnedMediaSeekTracking(
    quint32 runNonce);
[[nodiscard]] QJsonObject armOwnedMediaBackendRemoval(
    quint32 runNonce);
[[nodiscard]] QJsonObject finishOwnedMediaBackendRemoval(
    quint32 runNonce,
    bool abandon = false);
[[nodiscard]] QJsonObject releaseOwnedMediaBackendRemoval(
    quint32 runNonce);
}

[[nodiscard]] bool browserUserActivationIsActive();
[[nodiscard]] quint32 generateOwnedBrowserNonce();
[[nodiscard]] quint32 awaitOwnedBrowserNonce(
    quint32 requestedNonce,
    quint32 firstNativeDepthHandler,
    const std::uint8_t *primaryStackCanary,
    int primaryStackCanaryLength,
    int primaryStackCanarySeed,
    bool &fullPumpDeferred,
    bool &primaryStackCanaryObservedIntact);
[[nodiscard]] bool dispatchNativeDepthProbeEvent(
    quint32 handler,
    int ordinal);
[[nodiscard]] bool scheduleNativeDepthProbeEvent(
    quint32 handler,
    int ordinal);
[[nodiscard]] bool dispatchNativeDepthLimitAttemptEvent();
[[nodiscard]] bool verifyNativeStackCanary(
    const std::uint8_t *bytes,
    int length,
    int seed);
[[nodiscard]] int awaitUnmatchedNativeSuspension();
[[nodiscard]] bool scheduleExclusiveSuspendGuardProbe(
    quint32 firstHandler,
    quint32 secondHandler,
    quint32 exclusiveHandler,
    quint32 completionHandler);
[[nodiscard]] bool armExclusiveSuspendGuardNormalDrain();
[[nodiscard]] QJsonObject takeExclusiveSuspendGuardProbeResult();
void cancelExclusiveSuspendGuardProbe();
[[nodiscard]] constexpr quint32 jspiWatchdogSentinel()
{
    return 0xFFFFFFFFU;
}

void installBrowserRuntimeCommandHandler(
    QObject *owner,
    BrowserRuntimeCommandHandler handler);
