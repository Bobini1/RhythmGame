#pragma once

#include "BrowserRuntimeBridge.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include <memory>

class Gate1bReport;
class ApplicationCycleOrderProbe;
class QEvent;
class JspiNestedLoopProbe;
class MediaProbe;
class NetworkProbe;
class QQuickWindow;
class QTimer;
class QVideoSink;
class RenderProbe;
struct RenderFrameMilestone;
struct ThreadProbeContext;

class ProbeState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
        bool exceptionPassed
        READ exceptionPassed
        NOTIFY exceptionPassedChanged)
    Q_PROPERTY(
        bool threadPassed
        READ threadPassed
        NOTIFY threadPassedChanged)
    Q_PROPERTY(
        bool phaseFrozen
        READ phaseFrozen
        NOTIFY phaseFrozenChanged)
    Q_PROPERTY(
        double shaderPhase
        READ shaderPhase
        NOTIFY shaderPhaseChanged)

public:
    explicit ProbeState(
        QStringList expectedArguments,
        QObject *parent = nullptr);
    ~ProbeState() override;

    [[nodiscard]] bool exceptionPassed() const;
    [[nodiscard]] bool threadPassed() const;
    [[nodiscard]] bool phaseFrozen() const;
    [[nodiscard]] double shaderPhase() const;

    void attachWindow(QQuickWindow *window);
    void recordMainReturning();
    void postMainTick();
    void failStartup(QStringView code, QStringView detail);

    Q_INVOKABLE bool attachMediaVideoSink(QVideoSink *videoSink);
    Q_INVOKABLE void beginUserActivatedProbes();

signals:
    void exceptionPassedChanged();
    void threadPassedChanged();
    void phaseFrozenChanged();
    void shaderPhaseChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    [[nodiscard]] QJsonObject handleBrowserCommand(
        BrowserRuntimeCommand command,
        const QJsonObject &payload);
    void runStaticLibraryExceptionProbe();
    void startApplicationCycleOrderProbe();
    void startForegroundLatencyProbe(int sampleCount);
    void recordForegroundTimerLatency();
    [[nodiscard]] QJsonObject startHiddenTimerProbe();
    [[nodiscard]] QJsonObject startVisibleResumeTimerProbe();
    [[nodiscard]] QJsonObject armBfcacheResumeProbe();
    void releaseBfcacheResumeHandler();
    void startThreadProbes();
    void startQtConcurrentWorker();
    void pollExplicitWorker();
    void maybeFinalizeThreadProbes();
    void startJspiProbe();
    void publishRenderMilestone(
        const RenderFrameMilestone &milestone);
    void completeRenderCapture(
        const RenderFrameMilestone &milestone);
    [[nodiscard]] bool coreChecksComplete() const;
    void maybeResolveReady();
    void maybeMarkCoreComplete();

    QStringList m_expectedArguments;
    Gate1bReport *m_report = nullptr;
    ApplicationCycleOrderProbe *m_applicationCycleProbe = nullptr;
    JspiNestedLoopProbe *m_jspiProbe = nullptr;
    RenderProbe *m_renderProbe = nullptr;
    NetworkProbe *m_networkProbe = nullptr;
    MediaProbe *m_mediaProbe = nullptr;
    QTimer *m_foregroundLatencyTimer = nullptr;
    QTimer *m_hiddenLifecycleTimer = nullptr;
    QTimer *m_visibleResumeTimer = nullptr;
    QPointer<QQuickWindow> m_window;
    std::unique_ptr<ThreadProbeContext> m_threadContext;

    bool m_exceptionPassed = false;
    bool m_threadPassed = false;
    bool m_phaseFrozen = false;
    bool m_qmlRootAttached = false;
    bool m_mainReturning = false;
    bool m_postMainObserved = false;
    bool m_postMainRenderReady = false;
    bool m_pingAcknowledged = false;
    bool m_readyResolved = false;
    bool m_userActivatedProbesStarted = false;
    bool m_hiddenTimerProbeStarted = false;
    bool m_visibleResumeTimerProbeStarted = false;
    int m_foregroundInputSampleCount = 0;
    int m_foregroundInputSamplesRemaining = 0;
    int m_foregroundTimerSampleCount = 0;
    qint64 m_foregroundTimerDeadlineMicroseconds = 0;
    QJsonArray m_foregroundTimerLatenessMicroseconds;
    double m_shaderPhase = 0.0;
    quint64 m_phaseGeneration = 0;
    quint32 m_runtimeRunNonce = 0;
    quint32 m_bfcacheResumeHandler = 0;
    quint32 m_bfcacheResumeNonce = 0;
};
