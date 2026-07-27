#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QStringView>
#include <QtTypes>

#include <functional>
#include <memory>

struct ExclusiveSuspendProbeState;
class QTimer;

struct JspiNestedLoopResult
{
    bool promiseResolvedWhileExec;
    bool quitDelivered;
    bool postLoopSentinel;
    bool watchdogTimedOut;
    bool fullPumpDeferredWhilePrimary;
    bool nativeComposedPathsIntact;
    bool nativeEventIdentitiesIntact;
    bool nativeStackCanariesIntact;
    bool primaryStackCanaryIntact;
    bool primaryStackCanaryObservedIntact;
    quint32 requestedNonce;
    quint32 resolvedNonce;
    qint64 elapsedMicroseconds;
    QJsonArray nativeEnterOrder;
    QJsonArray nativeExitOrder;
};

struct ExclusiveSuspendGuardResult
{
    bool passed;
    QJsonObject detail;
};

class JspiNestedLoopProbe final : public QObject
{
public:
    using EventCallback =
        std::function<void(QStringView, QJsonObject)>;
    using ExclusiveSuspendGuardCallback =
        std::function<void(ExclusiveSuspendGuardResult)>;

    explicit JspiNestedLoopProbe(
        EventCallback eventCallback,
        QObject *parent = nullptr);
    ~JspiNestedLoopProbe() override;

    [[nodiscard]] JspiNestedLoopResult runJspiNestedLoop();
    void startExclusiveSuspendGuardProbe(
        ExclusiveSuspendGuardCallback callback);
    [[nodiscard]] bool armNativeDepthLimitProbe();
    [[nodiscard]] bool armNativeSuspensionTrapProbe();

private:
    void finishExclusiveSuspendGuardProbe(
        const std::shared_ptr<ExclusiveSuspendProbeState> &state);
    void releaseExclusiveSuspendGuardProbe(
        const std::shared_ptr<ExclusiveSuspendProbeState> &state);
    void releaseNativeBoundaryNegativeProbeHandlers();

    EventCallback m_eventCallback;
    std::shared_ptr<ExclusiveSuspendProbeState> m_exclusiveState;
    QTimer *m_exclusiveFinalizationWatchdog = nullptr;
    QList<quint32> m_nativeBoundaryNegativeHandlers;
    bool m_nativeBoundaryNegativeProbeArmed = false;
};
