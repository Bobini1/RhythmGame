#include "ProbeState.h"

#include "BrowserRuntimeBridge.h"
#include "ExceptionBoundary.h"
#include "Gate1bReport.h"
#include "JspiNestedLoopProbe.h"
#include "MediaProbe.h"
#include "NetworkProbe.h"
#include "RenderProbe.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QEvent>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonValue>
#include <QQuickWindow>
#include <QTimer>
#include <QVideoSink>
#include <QtConcurrent>

#ifdef __EMSCRIPTEN__
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#endif

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <pthread.h>
#include <sched.h>
#include <stdexcept>
#include <string_view>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten/threading.h>
#endif

extern "C" const char *rhythmGameWasmProbeInputDigest();

namespace
{
constexpr quint32 explicitInputNonce = 0x13579BDFU;
constexpr quint32 explicitNonceXor = 0xA5A55A5AU;
constexpr int foregroundInputSampleTarget = 64;
constexpr auto foregroundTimerInterval =
    std::chrono::milliseconds{5};
constexpr int foregroundTimerSampleTarget = 32;
constexpr auto hiddenLifecycleTimerInterval =
    std::chrono::milliseconds{125};
constexpr auto visibleResumeTimerInterval =
    std::chrono::milliseconds{125};
constexpr int workerPollLimit = 2000;
constexpr std::string_view inputDigestMarkerPrefix{
    "RG_WASM_PROBE_INPUT_SHA256="};

QString compiledInputBuildId()
{
    const QByteArray marker{rhythmGameWasmProbeInputDigest()};
    const QByteArray prefix{
        inputDigestMarkerPrefix.data(),
        static_cast<qsizetype>(inputDigestMarkerPrefix.size())};
    if (!marker.startsWith(prefix) || marker.size() != prefix.size() + 64) {
        throw std::runtime_error{"compiled-input-digest-shape"};
    }
    const QByteArray digest = marker.sliced(prefix.size());
    for (const char value : digest) {
        const bool decimal = value >= '0' && value <= '9';
        const bool lowerHex = value >= 'a' && value <= 'f';
        if (!decimal && !lowerHex) {
            throw std::runtime_error{"compiled-input-digest-shape"};
        }
    }
    return QString::fromLatin1(digest);
}

struct ExplicitWorkerState
{
    std::atomic<bool> explicitReady{false};
    std::atomic<bool> releaseExplicit{false};
    std::atomic<bool> overlapObserved{false};
    std::atomic<bool> completed{false};
    std::atomic<bool> timedOut{false};
    std::atomic<bool> isMainRuntimeThread{true};
    std::atomic<quintptr> threadId{0};
    std::atomic<quint32> transformedNonce{0};
    std::atomic<qint64> explicitReadyMicroseconds{0};
    std::atomic<qint64> explicitCompletedMicroseconds{0};
    std::atomic<qint64> qtConcurrentReleaseMicroseconds{0};
    quint32 inputNonce = explicitInputNonce;
};

struct ConcurrentWorkerResult
{
    int result = 0;
    quintptr threadId = 0;
    bool isMainRuntimeThread = true;
    bool explicitReadyObserved = false;
    bool overlapObserved = false;
    qint64 qtConcurrentStartedMicroseconds = 0;
    qint64 qtConcurrentObservedReadyMicroseconds = 0;
    qint64 qtConcurrentReleaseMicroseconds = 0;
};

qint64 steadyMicroseconds()
{
    const auto elapsed = std::chrono::duration_cast<
        std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    return elapsed / std::chrono::microseconds{1};
}

qint64 milliseconds(std::chrono::milliseconds duration)
{
    return duration / std::chrono::milliseconds{1};
}

qint64 microseconds(std::chrono::microseconds duration)
{
    return duration / std::chrono::microseconds{1};
}

quintptr currentPthreadIdentity()
{
    return static_cast<quintptr>(pthread_self());
}

void workerPause()
{
#ifdef __EMSCRIPTEN__
    emscripten_thread_sleep(1);
#else
    sched_yield();
#endif
}

void *runExplicitWorker(void *opaque)
{
    std::unique_ptr<std::shared_ptr<ExplicitWorkerState>> stateOwner{
        static_cast<std::shared_ptr<ExplicitWorkerState> *>(opaque)};
    const std::shared_ptr<ExplicitWorkerState> state =
        std::move(*stateOwner);
    stateOwner.reset();
    state->threadId.store(
        currentPthreadIdentity(),
        std::memory_order_relaxed);
#ifdef __EMSCRIPTEN__
    state->isMainRuntimeThread.store(
        emscripten_is_main_runtime_thread() != 0,
        std::memory_order_relaxed);
#else
    state->isMainRuntimeThread.store(false, std::memory_order_relaxed);
#endif
    state->explicitReadyMicroseconds.store(
        steadyMicroseconds(),
        std::memory_order_relaxed);
    state->explicitReady.store(true, std::memory_order_release);

    bool released = false;
    for (int attempt = 0; attempt < workerPollLimit; ++attempt) {
        if (state->releaseExplicit.load(std::memory_order_acquire)) {
            released = true;
            break;
        }
        workerPause();
    }

    state->timedOut.store(!released, std::memory_order_relaxed);
    state->transformedNonce.store(
        state->inputNonce ^ explicitNonceXor,
        std::memory_order_relaxed);
    state->explicitCompletedMicroseconds.store(
        steadyMicroseconds(),
        std::memory_order_relaxed);
    state->completed.store(true, std::memory_order_release);
    return nullptr;
}

QString threadIdentityString(quintptr identity)
{
    return QString::number(static_cast<qulonglong>(identity));
}

int qtConcurrentResult()
{
    return 42;
}
}

class ApplicationCycleEventReceiver final : public QObject
{
public:
    using RecordCallback = std::function<void(QStringView)>;

    ApplicationCycleEventReceiver(
        QEvent::Type eventType,
        QString phase,
        RecordCallback record,
        QObject *parent = nullptr)
        : QObject{parent}
        , m_eventType{eventType}
        , m_phase{std::move(phase)}
        , m_record{std::move(record)}
    {
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() != m_eventType) {
            return QObject::event(event);
        }
        if (!m_recorded) {
            m_recorded = true;
            m_record(m_phase);
        }
        if (m_eventType == QEvent::DeferredDelete) {
            return QObject::event(event);
        }
        return true;
    }

private:
    QEvent::Type m_eventType;
    QString m_phase;
    RecordCallback m_record;
    bool m_recorded = false;
};

class ApplicationCycleOrderProbe final : public QObject
{
public:
    using CompletionCallback =
        std::function<void(bool, QJsonObject)>;

    explicit ApplicationCycleOrderProbe(
        CompletionCallback completion,
        QObject *parent = nullptr)
        : QObject{parent}
        , m_completion{std::move(completion)}
    {
    }

    ~ApplicationCycleOrderProbe() override
    {
        releaseNativeHandler();
    }

    void start()
    {
        if (m_started || m_finished) {
            finish(false, u"application-cycle-probe-restarted");
            return;
        }
        m_started = true;

        constexpr auto postedEventType =
            static_cast<QEvent::Type>(QEvent::User + 73);
        auto record = [this](QStringView phase) {
            recordPhase(phase);
        };
        auto *postedReceiver = new ApplicationCycleEventReceiver{
            postedEventType,
            QStringLiteral("posted"),
            record,
            this};
        QCoreApplication::postEvent(
            postedReceiver,
            new QEvent(postedEventType));

#ifdef __EMSCRIPTEN__
        QWasmSuspendResumeControl *control =
            QWasmSuspendResumeControl::get();
        m_nativeHandler = control->registerEventHandler(
            [this](emscripten::val) {
                recordPhase(u"native");
            });
        control->jsEventHandlerAt(m_nativeHandler)(
            emscripten::val::object());
#else
        finish(false, u"application-cycle-probe-requires-emscripten");
        return;
#endif

        auto *timer = new QTimer{this};
        timer->setSingleShot(true);
        timer->setTimerType(Qt::PreciseTimer);
        connect(timer, &QTimer::timeout, this, [this, timer] {
            recordPhase(u"timer");
            timer->deleteLater();
            if (m_deferredDeleteReceiver != nullptr) {
                m_deferredDeleteReceiver->deleteLater();
                m_deferredDeleteReceiver = nullptr;
            }
        });
        timer->start(0);

        m_deferredDeleteReceiver =
            new ApplicationCycleEventReceiver(
                QEvent::DeferredDelete,
                QStringLiteral("deferred-delete"),
                record,
                this);
    }

private:
    void recordPhase(QStringView phase)
    {
        if (m_finished) {
            return;
        }
        m_observedOrder.append(phase.toString());
        const QJsonArray expectedPrefix{
            QStringLiteral("posted"),
            QStringLiteral("native"),
            QStringLiteral("timer"),
            QStringLiteral("deferred-delete"),
        };
        const qsizetype observedIndex = m_observedOrder.size() - 1;
        if (observedIndex >= expectedPrefix.size()
            || m_observedOrder.at(observedIndex)
                != expectedPrefix.at(observedIndex)) {
            finish(false, u"application-cycle-order-mismatch");
            return;
        }
        if (m_observedOrder.size() == expectedPrefix.size()) {
            finish(true, {});
        }
    }

    void finish(bool passed, QStringView error)
    {
        if (m_finished) {
            return;
        }
        m_finished = true;
        QTimer::singleShot(0, this, [this] {
            releaseNativeHandler();
        });
        QJsonObject detail = {
            {QStringLiteral("expectedOrder"),
             QJsonArray{
                 QStringLiteral("posted"),
                 QStringLiteral("native"),
                 QStringLiteral("timer"),
                 QStringLiteral("deferred-delete"),
             }},
            {QStringLiteral("observedOrder"), m_observedOrder},
        };
        if (!error.isEmpty()) {
            detail.insert(
                QStringLiteral("error"),
                error.toString());
        }
        auto completion = std::move(m_completion);
        if (completion) {
            completion(passed, std::move(detail));
        }
    }

    void releaseNativeHandler()
    {
#ifdef __EMSCRIPTEN__
        if (m_nativeHandler != 0) {
            QWasmSuspendResumeControl::get()->removeEventHandler(
                m_nativeHandler);
            m_nativeHandler = 0;
        }
#endif
    }

    CompletionCallback m_completion;
    QJsonArray m_observedOrder;
    ApplicationCycleEventReceiver *m_deferredDeleteReceiver = nullptr;
    quint32 m_nativeHandler = 0;
    bool m_started = false;
    bool m_finished = false;
};

struct ThreadProbeContext
{
    pthread_t explicitThread = {};
    std::shared_ptr<ExplicitWorkerState> explicitState;
    ConcurrentWorkerResult concurrentResult;
    QTimer *pollTimer = nullptr;
    bool explicitCreated = false;
    bool explicitReaped = false;
    bool concurrentStarted = false;
    bool concurrentFinished = false;
    bool reported = false;
};

ProbeState::ProbeState(
    QStringList expectedArguments,
    QObject *parent)
    : QObject{parent}
    , m_expectedArguments{std::move(expectedArguments)}
    , m_report{new Gate1bReport{this}}
    , m_jspiProbe{new JspiNestedLoopProbe{
          [this](QStringView type, QJsonObject payload) {
              m_report->append(type, std::move(payload));
          },
          this}}
    , m_renderProbe{new RenderProbe{
          [this](const RenderFrameMilestone &milestone) {
              publishRenderMilestone(milestone);
          },
          [this](const RenderFrameMilestone &milestone) {
              completeRenderCapture(milestone);
          },
          [this](QStringView code, QStringView detail) {
              m_report->fail(code, detail);
          },
          this}}
    , m_networkProbe{new NetworkProbe{
          [this](QStringView type, QJsonObject payload) {
              m_report->append(type, std::move(payload));
          },
          [this](QStringView check, QJsonObject detail) {
              if (!m_report->isTerminal()) {
                  m_report->pass(check, std::move(detail));
              }
          },
          [this](QStringView code, QStringView detail) {
              m_report->fail(code, detail);
          },
          this}}
    , m_mediaProbe{new MediaProbe{
          [this](QStringView type, QJsonObject payload) {
              m_report->append(type, std::move(payload));
          },
          [this](QStringView check, QJsonObject detail) {
              if (!m_report->isTerminal()) {
                  m_report->pass(check, std::move(detail));
              }
          },
          [this](QStringView code, QStringView detail) {
              m_report->fail(code, detail);
          },
          this}}
{
    QPointer<ProbeState> guardedThis{this};
    installBrowserRuntimeCommandHandler(
        this,
        [guardedThis](
            BrowserRuntimeCommand command,
            const QJsonObject &payload) -> QJsonObject {
            if (guardedThis.isNull()) {
                throw std::runtime_error{
                    "runtime-command-owner-destroyed"};
            }
            return guardedThis->handleBrowserCommand(command, payload);
        });
}

ProbeState::~ProbeState()
{
    releaseBfcacheResumeHandler();
    if (!m_threadContext) {
        return;
    }
    auto &context = *m_threadContext;
    if (context.pollTimer != nullptr) {
        context.pollTimer->stop();
    }
    if (!context.explicitCreated || context.explicitReaped) {
        return;
    }

    context.explicitState->releaseExplicit.store(
        true,
        std::memory_order_release);
    const int detachResult =
        pthread_detach(context.explicitThread);
    context.explicitReaped = detachResult == 0;
}

bool ProbeState::exceptionPassed() const
{
    return m_exceptionPassed;
}

bool ProbeState::threadPassed() const
{
    return m_threadPassed;
}

bool ProbeState::phaseFrozen() const
{
    return m_phaseFrozen;
}

double ProbeState::shaderPhase() const
{
    return m_shaderPhase;
}

void ProbeState::attachWindow(QQuickWindow *window)
{
    if (window == nullptr || !m_window.isNull()) {
        m_report->fail(
            u"qml-root-invalid",
            u"exactly one QQuickWindow must be attached");
        return;
    }
    m_window = window;
    m_window->installEventFilter(this);
    m_renderProbe->attachWindow(window);
    m_qmlRootAttached = true;
    m_report->append(u"qml-root-attached");
}

bool ProbeState::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window.data()
        && event->type() == QEvent::MouseMove
        && event->spontaneous()
        && m_foregroundInputSamplesRemaining > 0
        && !m_report->isTerminal()) {
        const int ordinal =
            m_foregroundInputSampleCount
            - m_foregroundInputSamplesRemaining
            + 1;
        --m_foregroundInputSamplesRemaining;
        m_report->append(
            u"qt-foreground-input-sample",
            QJsonObject{
                {QStringLiteral("ordinal"), ordinal},
                {QStringLiteral("spontaneous"), true},
            });
        if (m_foregroundInputSamplesRemaining == 0) {
            m_report->pass(
                u"qt-foreground-input-delivery",
                QJsonObject{
                    {QStringLiteral("sameDispatchRequired"), true},
                    {QStringLiteral("sampleCount"),
                     m_foregroundInputSampleCount},
                });
        }
    }
    return QObject::eventFilter(watched, event);
}

void ProbeState::recordMainReturning()
{
    if (m_mainReturning) {
        return;
    }
    m_mainReturning = true;
    m_report->append(u"main-returning");
}

void ProbeState::postMainTick()
{
    if (m_postMainObserved || m_report->isTerminal()) {
        return;
    }
    m_postMainObserved = true;
    m_report->append(u"post-main-tick");
    const QStringList arguments = QCoreApplication::arguments();
    const bool argumentsMatchRetainedCopy =
        arguments == m_expectedArguments;
    const QString firstApplicationFilePath =
        QCoreApplication::applicationFilePath();
    const bool applicationFilePathStable =
        QCoreApplication::applicationFilePath()
        == firstApplicationFilePath;
    m_report->append(
        u"post-main-application-state",
        QJsonObject{
            {QStringLiteral("applicationFilePathStable"),
             applicationFilePathStable},
            {QStringLiteral("argumentCount"), arguments.size()},
            {QStringLiteral("argumentsMatchRetainedCopy"),
             argumentsMatchRetainedCopy},
        });
    if (arguments.isEmpty()
        || !argumentsMatchRetainedCopy
        || !applicationFilePathStable) {
        m_report->fail(
            u"post-main-application-state-invalid",
            u"Qt application arguments did not survive main return");
        return;
    }
    if (!m_report->requiredCapabilitiesAvailable()) {
        m_report->fail(
            u"browser-capability-missing",
            u"one or more required Chromium capabilities are unavailable");
        return;
    }
    m_runtimeRunNonce = generateOwnedBrowserNonce();
    if (m_runtimeRunNonce == 0
        || m_runtimeRunNonce == jspiWatchdogSentinel()) {
        m_report->fail(
            u"runtime-run-nonce",
            u"browser did not generate an owned Task 4 run nonce");
        return;
    }
    if (!m_mediaProbe->armDeviceObservation(m_runtimeRunNonce)) {
        m_report->fail(
            u"qt-media-device-observation",
            u"Qt media-device observation could not be armed exactly once");
        return;
    }
    m_networkProbe->start(m_runtimeRunNonce);
    static_cast<void>(m_renderProbe->beginPostMainCapture());
    startThreadProbes();
    startJspiProbe();
    maybeResolveReady();
}

void ProbeState::failStartup(QStringView code, QStringView detail)
{
    m_report->fail(code, detail);
}

bool ProbeState::attachMediaVideoSink(QVideoSink *videoSink)
{
    const bool attached = m_mediaProbe->attachVideoSink(videoSink);
    if (!attached) {
        m_report->fail(
            u"qt-media-video-sink",
            u"QML did not attach exactly one VideoOutput videoSink");
    }
    return attached;
}

void ProbeState::beginUserActivatedProbes()
{
    if (m_userActivatedProbesStarted || m_report->isTerminal()) {
        return;
    }
    m_userActivatedProbesStarted = true;
    const bool active = browserUserActivationIsActive();
    m_report->append(
        u"user-activation-sampled",
        QJsonObject{{QStringLiteral("active"), active}});
    if (!active) {
        m_report->fail(
            u"user-activation-inactive",
            u"trusted QML click did not retain transient activation");
        return;
    }
    m_mediaProbe->start(m_runtimeRunNonce);
}

QJsonObject ProbeState::handleBrowserCommand(
    BrowserRuntimeCommand command,
    const QJsonObject &payload)
{
    if (m_report->isTerminal()) {
        throw std::runtime_error{"runtime-command-terminal"};
    }

    switch (command) {
    case BrowserRuntimeCommand::AcknowledgeMediaFrameCapture: {
        const QJsonArray requestIdValues =
            payload.value(u"requestIds").toArray();
        QStringList requestIds;
        requestIds.reserve(requestIdValues.size());
        for (const QJsonValue &value : requestIdValues) {
            requestIds.push_back(value.toString());
        }
        QJsonObject acknowledgement =
            m_mediaProbe->acknowledgeVisualCapture(
                static_cast<quint32>(
                    payload.value(u"runNonce").toInteger()),
                requestIds);
        if (acknowledgement.isEmpty()) {
            throw std::runtime_error{
                "media-frame-capture-acknowledgement-invalid"};
        }
        m_report->append(
            u"command-acknowledged",
            acknowledgement);
        return acknowledgement;
    }
    case BrowserRuntimeCommand::ProbePing: {
        m_pingAcknowledged = true;
        startApplicationCycleOrderProbe();
        QJsonObject acknowledgement = {
            {QStringLiteral("command"),
             browserRuntimeCommandName(command).toString()},
            {QStringLiteral("inputBuildId"),
             compiledInputBuildId()},
        };
        m_report->append(u"command-acknowledged", acknowledgement);
        maybeResolveReady();
        return acknowledgement;
    }
    case BrowserRuntimeCommand::BeginForegroundLatencySampling: {
        if (!m_readyResolved || !coreChecksComplete()) {
            throw std::runtime_error{
                "foreground-latency-before-core"};
        }
        const int sampleCount = payload.value(u"sampleCount").toInt();
        startForegroundLatencyProbe(sampleCount);
        QJsonObject acknowledgement = {
            {QStringLiteral("command"),
             browserRuntimeCommandName(command).toString()},
            {QStringLiteral("inputSampleCount"), sampleCount},
            {QStringLiteral("timerIntervalMilliseconds"),
             milliseconds(foregroundTimerInterval)},
            {QStringLiteral("timerSampleCount"),
             foregroundTimerSampleTarget},
        };
        m_report->append(
            u"command-acknowledged",
            acknowledgement);
        return acknowledgement;
    }
    case BrowserRuntimeCommand::ArmBfcacheResumeProbe:
        if (!m_readyResolved || !coreChecksComplete()) {
            throw std::runtime_error{
                "bfcache-resume-before-core"};
        }
        return armBfcacheResumeProbe();
    case BrowserRuntimeCommand::ArmHiddenTimerProbe:
        if (!m_readyResolved || !coreChecksComplete()) {
            throw std::runtime_error{
                "hidden-timer-before-core"};
        }
        return startHiddenTimerProbe();
    case BrowserRuntimeCommand::ArmVisibleResumeTimerProbe:
        if (!m_readyResolved || !coreChecksComplete()) {
            throw std::runtime_error{
                "visible-resume-timer-before-core"};
        }
        return startVisibleResumeTimerProbe();
    case BrowserRuntimeCommand::TriggerNativeDepthLimit:
    case BrowserRuntimeCommand::TriggerNativeSuspensionTrap: {
        if (!m_readyResolved) {
            throw std::runtime_error{
                "native-boundary-negative-before-ready"};
        }
        const bool armed =
            command == BrowserRuntimeCommand::TriggerNativeDepthLimit
            ? m_jspiProbe->armNativeDepthLimitProbe()
            : m_jspiProbe->armNativeSuspensionTrapProbe();
        if (!armed) {
            throw std::runtime_error{
                "native-boundary-negative-arm-failed"};
        }
        QJsonObject acknowledgement = {
            {QStringLiteral("armed"), true},
            {QStringLiteral("command"),
             browserRuntimeCommandName(command).toString()},
        };
        m_report->append(u"command-acknowledged", acknowledgement);
        return acknowledgement;
    }
    case BrowserRuntimeCommand::SetShaderPhase: {
        if (!m_readyResolved) {
            throw std::runtime_error{"shader-phase-before-ready"};
        }

        const double phase = payload.value(u"phase").toDouble();
        if (!m_phaseFrozen) {
            m_phaseFrozen = true;
            emit phaseFrozenChanged();
        }
        if (m_shaderPhase != phase) {
            m_shaderPhase = phase;
            emit shaderPhaseChanged();
        }
        ++m_phaseGeneration;
        const quint64 frameBaseline =
            m_renderProbe->beginPhaseCapture(m_phaseGeneration);

        QJsonObject acknowledgement = {
            {QStringLiteral("command"),
             browserRuntimeCommandName(command).toString()},
            {QStringLiteral("frameBaseline"),
             static_cast<qint64>(frameBaseline)},
            {QStringLiteral("generation"),
             static_cast<qint64>(m_phaseGeneration)},
            {QStringLiteral("phase"), phase},
        };
        m_report->append(u"command-acknowledged", acknowledgement);
        return acknowledgement;
    }
    }
    throw std::runtime_error{"runtime-command-enum-invalid"};
}

void ProbeState::startApplicationCycleOrderProbe()
{
    if (m_applicationCycleProbe != nullptr
        || m_report->isTerminal()) {
        return;
    }
    m_applicationCycleProbe = new ApplicationCycleOrderProbe(
        [this](bool passed, QJsonObject detail) {
            if (m_report->isTerminal()) {
                return;
            }
            if (!passed) {
                m_report->append(
                    u"qt-application-cycle-order-observed",
                    detail);
                m_report->fail(
                    u"qt-application-cycle-order",
                    u"retained Qt pump did not preserve "
                    u"posted/native/timer/deferred-delete order");
                return;
            }
            m_report->pass(
                u"qt-application-cycle-order",
                std::move(detail));
            maybeMarkCoreComplete();
        },
        this);
    m_applicationCycleProbe->start();
}

void ProbeState::startForegroundLatencyProbe(int sampleCount)
{
    if (sampleCount != foregroundInputSampleTarget
        || m_foregroundInputSampleCount != 0
        || m_foregroundLatencyTimer != nullptr) {
        throw std::runtime_error{
            "foreground-latency-probe-invalid"};
    }

    m_foregroundInputSampleCount = sampleCount;
    m_foregroundInputSamplesRemaining = sampleCount;
    m_foregroundTimerSampleCount = foregroundTimerSampleTarget;
    m_foregroundLatencyTimer = new QTimer{this};
    m_foregroundLatencyTimer->setSingleShot(true);
    m_foregroundLatencyTimer->setTimerType(Qt::PreciseTimer);
    m_foregroundLatencyTimer->setInterval(
        foregroundTimerInterval);
    connect(
        m_foregroundLatencyTimer,
        &QTimer::timeout,
        this,
        &ProbeState::recordForegroundTimerLatency);
    m_foregroundTimerDeadlineMicroseconds =
        steadyMicroseconds()
        + microseconds(foregroundTimerInterval);
    m_foregroundLatencyTimer->start();
}

void ProbeState::recordForegroundTimerLatency()
{
    if (m_report->isTerminal()
        || m_foregroundLatencyTimer == nullptr
        || m_foregroundTimerLatenessMicroseconds.size()
            >= m_foregroundTimerSampleCount) {
        return;
    }

    const qint64 observedMicroseconds = steadyMicroseconds();
    const qint64 latenessMicroseconds = std::max<qint64>(
        0,
        observedMicroseconds
            - m_foregroundTimerDeadlineMicroseconds);
    const int ordinal =
        m_foregroundTimerLatenessMicroseconds.size() + 1;
    m_foregroundTimerLatenessMicroseconds.append(
        latenessMicroseconds);
    m_report->append(
        u"qt-foreground-timer-sample",
        QJsonObject{
            {QStringLiteral("latenessMicroseconds"),
             latenessMicroseconds},
            {QStringLiteral("ordinal"), ordinal},
        });

    if (m_foregroundTimerLatenessMicroseconds.size()
        == m_foregroundTimerSampleCount) {
        m_foregroundLatencyTimer->deleteLater();
        m_foregroundLatencyTimer = nullptr;
        m_report->pass(
            u"qt-foreground-timer-delivery",
            QJsonObject{
                {QStringLiteral("intervalMilliseconds"),
                 milliseconds(foregroundTimerInterval)},
                {QStringLiteral("latenessMicroseconds"),
                 m_foregroundTimerLatenessMicroseconds},
                {QStringLiteral("sampleCount"),
                 m_foregroundTimerSampleCount},
            });
        return;
    }

    m_foregroundTimerDeadlineMicroseconds =
        observedMicroseconds
        + microseconds(foregroundTimerInterval);
    m_foregroundLatencyTimer->start();
}

QJsonObject ProbeState::startHiddenTimerProbe()
{
    if (m_hiddenTimerProbeStarted
        || m_hiddenLifecycleTimer != nullptr) {
        throw std::runtime_error{
            "hidden-timer-probe-already-started"};
    }
    m_hiddenTimerProbeStarted = true;
    m_hiddenLifecycleTimer = new QTimer{this};
    m_hiddenLifecycleTimer->setSingleShot(true);
    m_hiddenLifecycleTimer->setTimerType(Qt::PreciseTimer);
    m_hiddenLifecycleTimer->setInterval(
        hiddenLifecycleTimerInterval);
    connect(
        m_hiddenLifecycleTimer,
        &QTimer::timeout,
        this,
        [this] {
            if (m_report->isTerminal()) {
                return;
            }
            m_report->append(
                u"qt-hidden-timer-sentinel",
                QJsonObject{{
                    QStringLiteral("intervalMilliseconds"),
                    milliseconds(hiddenLifecycleTimerInterval)}});
            m_report->pass(
                u"qt-hidden-timer-delivery",
                QJsonObject{{
                    QStringLiteral("intervalMilliseconds"),
                    milliseconds(hiddenLifecycleTimerInterval)}});
            m_hiddenLifecycleTimer->deleteLater();
            m_hiddenLifecycleTimer = nullptr;
        });
    m_hiddenLifecycleTimer->start();

    QJsonObject acknowledgement = {
        {QStringLiteral("command"),
         browserRuntimeCommandName(
             BrowserRuntimeCommand::ArmHiddenTimerProbe).toString()},
        {QStringLiteral("intervalMilliseconds"),
         milliseconds(hiddenLifecycleTimerInterval)},
    };
    m_report->append(
        u"command-acknowledged",
        acknowledgement);
    return acknowledgement;
}

QJsonObject ProbeState::startVisibleResumeTimerProbe()
{
    if (m_visibleResumeTimerProbeStarted
        || m_visibleResumeTimer != nullptr) {
        throw std::runtime_error{
            "visible-resume-timer-probe-already-started"};
    }
    m_visibleResumeTimerProbeStarted = true;
    m_visibleResumeTimer = new QTimer{this};
    m_visibleResumeTimer->setSingleShot(true);
    m_visibleResumeTimer->setTimerType(Qt::PreciseTimer);
    m_visibleResumeTimer->setInterval(
        visibleResumeTimerInterval);
    connect(
        m_visibleResumeTimer,
        &QTimer::timeout,
        this,
        [this] {
            if (m_report->isTerminal()) {
                return;
            }
            m_report->append(
                u"qt-visible-resume-timer-sentinel",
                QJsonObject{{
                    QStringLiteral("intervalMilliseconds"),
                    milliseconds(visibleResumeTimerInterval)}});
            m_report->pass(
                u"qt-visible-resume-timer-delivery",
                QJsonObject{{
                    QStringLiteral("intervalMilliseconds"),
                    milliseconds(visibleResumeTimerInterval)}});
            m_visibleResumeTimer->deleteLater();
            m_visibleResumeTimer = nullptr;
        });
    m_visibleResumeTimer->start();

    QJsonObject acknowledgement = {
        {QStringLiteral("command"),
         browserRuntimeCommandName(
             BrowserRuntimeCommand::ArmVisibleResumeTimerProbe).toString()},
        {QStringLiteral("intervalMilliseconds"),
         milliseconds(visibleResumeTimerInterval)},
    };
    m_report->append(
        u"command-acknowledged",
        acknowledgement);
    return acknowledgement;
}

QJsonObject ProbeState::armBfcacheResumeProbe()
{
#ifdef __EMSCRIPTEN__
    if (m_bfcacheResumeHandler != 0
        || m_bfcacheResumeNonce != 0) {
        throw std::runtime_error{
            "bfcache-resume-probe-already-armed"};
    }
    m_bfcacheResumeNonce = generateOwnedBrowserNonce();
    if (m_bfcacheResumeNonce == 0) {
        throw std::runtime_error{
            "bfcache-resume-probe-nonce"};
    }
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    m_bfcacheResumeHandler = control->registerEventHandler(
        [this](emscripten::val event) {
            quint32 observedNonce = 0;
            try {
                observedNonce =
                    event["gate1bBfcacheNonce"].as<quint32>();
            } catch (...) {
                observedNonce = 0;
            }
            const quint32 expectedNonce = m_bfcacheResumeNonce;
            if (observedNonce == 0
                || observedNonce != expectedNonce) {
                m_report->fail(
                    u"qt-bfcache-resume",
                    u"restored native sentinel nonce mismatch");
                return;
            }
            m_report->append(
                u"qt-bfcache-resume-sentinel",
                QJsonObject{{
                    QStringLiteral("runNonce"),
                    static_cast<qint64>(observedNonce)}});
            m_report->pass(
                u"qt-bfcache-resume",
                QJsonObject{{
                    QStringLiteral("runNonce"),
                    static_cast<qint64>(observedNonce)}});
            QTimer::singleShot(0, this, [this] {
                releaseBfcacheResumeHandler();
            });
        });
    QJsonObject acknowledgement = {
        {QStringLiteral("command"),
         browserRuntimeCommandName(
             BrowserRuntimeCommand::ArmBfcacheResumeProbe).toString()},
        {QStringLiteral("handlerIndex"),
         static_cast<qint64>(m_bfcacheResumeHandler)},
        {QStringLiteral("runNonce"),
         static_cast<qint64>(m_bfcacheResumeNonce)},
    };
    m_report->append(
        u"command-acknowledged",
        acknowledgement);
    return acknowledgement;
#else
    throw std::runtime_error{
        "bfcache-resume-probe-requires-emscripten"};
#endif
}

void ProbeState::releaseBfcacheResumeHandler()
{
#ifdef __EMSCRIPTEN__
    if (m_bfcacheResumeHandler != 0) {
        QWasmSuspendResumeControl::get()->removeEventHandler(
            m_bfcacheResumeHandler);
        m_bfcacheResumeHandler = 0;
    }
#endif
    m_bfcacheResumeNonce = 0;
}

void ProbeState::runStaticLibraryExceptionProbe()
{
    try {
        static_cast<void>(crossStaticLibraryBoundary());
    } catch (const std::runtime_error &error) {
        m_exceptionPassed =
            std::string_view{error.what()} == "wasm-native-exception";
    } catch (...) {
        m_exceptionPassed = false;
    }

    emit exceptionPassedChanged();
    if (!m_exceptionPassed) {
        m_report->fail(
            u"static-library-exception",
            u"native exception did not cross the static library boundary");
        return;
    }
    m_report->pass(
        u"static-library-exception",
        QJsonObject{{
            QStringLiteral("message"),
            QStringLiteral("wasm-native-exception")}});
    maybeMarkCoreComplete();
}

void ProbeState::startThreadProbes()
{
    if (m_threadContext || m_report->isTerminal()) {
        return;
    }
    runStaticLibraryExceptionProbe();
    if (m_report->isTerminal()) {
        return;
    }

    m_threadContext = std::make_unique<ThreadProbeContext>();
    auto &context = *m_threadContext;
    context.explicitState = std::make_shared<ExplicitWorkerState>();
    auto *const workerStateOwner =
        new std::shared_ptr<ExplicitWorkerState>{
            context.explicitState};
    const int createResult = pthread_create(
        &context.explicitThread,
        nullptr,
        &runExplicitWorker,
        workerStateOwner);
    if (createResult != 0) {
        delete workerStateOwner;
        m_report->fail(
            u"explicit-pthread-create",
            u"pthread_create failed");
        return;
    }
    context.explicitCreated = true;

    context.pollTimer = new QTimer{this};
    context.pollTimer->setInterval(std::chrono::milliseconds{1});
    connect(
        context.pollTimer,
        &QTimer::timeout,
        this,
        &ProbeState::pollExplicitWorker);
    context.pollTimer->start();
}

void ProbeState::startQtConcurrentWorker()
{
    if (!m_threadContext
        || m_threadContext->concurrentStarted
        || !m_threadContext->explicitState->explicitReady.load(
            std::memory_order_acquire)) {
        return;
    }
    auto &context = *m_threadContext;
    context.concurrentStarted = true;
    const std::shared_ptr<ExplicitWorkerState> explicitState =
        context.explicitState;
    auto *watcher = new QFutureWatcher<ConcurrentWorkerResult>{this};
    connect(
        watcher,
        &QFutureWatcher<ConcurrentWorkerResult>::finished,
        this,
        [this, watcher] {
            if (!m_threadContext) {
                watcher->deleteLater();
                return;
            }
            m_threadContext->concurrentResult = watcher->result();
            m_threadContext->concurrentFinished = true;
            watcher->deleteLater();
            maybeFinalizeThreadProbes();
        });
    watcher->setFuture(QtConcurrent::run([explicitState] {
        const qint64 qtConcurrentStartedMicroseconds =
            steadyMicroseconds();
        ConcurrentWorkerResult result;
        result.qtConcurrentStartedMicroseconds =
            qtConcurrentStartedMicroseconds;
        result.result = qtConcurrentResult();
        result.threadId = currentPthreadIdentity();
#ifdef __EMSCRIPTEN__
        result.isMainRuntimeThread =
            emscripten_is_main_runtime_thread() != 0;
#else
        result.isMainRuntimeThread = false;
#endif

        for (int attempt = 0; attempt < workerPollLimit; ++attempt) {
            if (explicitState->explicitReady.load(
                    std::memory_order_acquire)) {
                result.explicitReadyObserved = true;
                result.qtConcurrentObservedReadyMicroseconds =
                    steadyMicroseconds();
                break;
            }
            workerPause();
        }
        result.overlapObserved =
            result.explicitReadyObserved
            && !explicitState->completed.load(
                std::memory_order_acquire);
        explicitState->overlapObserved.store(
            result.overlapObserved,
            std::memory_order_release);
        result.qtConcurrentReleaseMicroseconds = steadyMicroseconds();
        explicitState->qtConcurrentReleaseMicroseconds.store(
            result.qtConcurrentReleaseMicroseconds,
            std::memory_order_relaxed);
        explicitState->releaseExplicit.store(
            true,
            std::memory_order_release);
        return result;
    }));
}

void ProbeState::pollExplicitWorker()
{
    if (!m_threadContext
        || !m_threadContext->explicitCreated
        || m_threadContext->explicitReaped) {
        return;
    }
    auto &context = *m_threadContext;
    if (!context.concurrentStarted
        && context.explicitState->explicitReady.load(
            std::memory_order_acquire)) {
        startQtConcurrentWorker();
    }
    if (!context.explicitState->completed.load(
            std::memory_order_acquire)) {
        return;
    }

    void *workerReturn = nullptr;
    const int joinResult = pthread_tryjoin_np(
        context.explicitThread,
        &workerReturn);
    if (joinResult == EBUSY) {
        return;
    }
    if (joinResult != 0) {
        context.pollTimer->stop();
        m_report->fail(
            u"explicit-pthread-reap",
            u"pthread_tryjoin_np returned an unexpected result");
        return;
    }

    context.explicitReaped = true;
    context.pollTimer->stop();
    if (workerReturn != nullptr) {
        m_report->fail(
            u"explicit-pthread-return",
            u"explicit worker returned an unexpected value");
        return;
    }
    if (context.explicitState->timedOut.load(
            std::memory_order_acquire)) {
        context.reported = true;
        m_report->fail(
            u"explicit-pthread-overlap-timeout",
            u"explicit worker timed out before overlap release");
        return;
    }
    maybeFinalizeThreadProbes();
}

void ProbeState::maybeFinalizeThreadProbes()
{
    if (!m_threadContext || m_threadContext->reported) {
        return;
    }
    auto &context = *m_threadContext;
    if (!context.explicitReaped || !context.concurrentFinished) {
        return;
    }
    context.reported = true;

    const auto &explicitState = *context.explicitState;
    const ConcurrentWorkerResult &concurrent =
        context.concurrentResult;
    const quintptr mainThreadId = currentPthreadIdentity();
    const quintptr explicitThreadId =
        explicitState.threadId.load(std::memory_order_acquire);
    const bool explicitIsMain =
        explicitState.isMainRuntimeThread.load(
            std::memory_order_acquire);
    const bool overlapObserved =
        explicitState.overlapObserved.load(
            std::memory_order_acquire);
    const quint32 transformedNonce =
        explicitState.transformedNonce.load(
            std::memory_order_acquire);
    const qint64 explicitReadyMicroseconds =
        explicitState.explicitReadyMicroseconds.load(
            std::memory_order_acquire);
    const qint64 explicitCompletedMicroseconds =
        explicitState.explicitCompletedMicroseconds.load(
            std::memory_order_acquire);

    const bool valid =
        !explicitState.timedOut.load(std::memory_order_acquire)
        && concurrent.result == 42
        && concurrent.explicitReadyObserved
        && concurrent.overlapObserved
        && overlapObserved
        && !explicitIsMain
        && !concurrent.isMainRuntimeThread
        && mainThreadId != 0
        && explicitThreadId != 0
        && concurrent.threadId != 0
        && mainThreadId != explicitThreadId
        && mainThreadId != concurrent.threadId
        && explicitThreadId != concurrent.threadId
        && transformedNonce
            == (explicitState.inputNonce ^ explicitNonceXor)
        && explicitReadyMicroseconds > 0
        && concurrent.qtConcurrentStartedMicroseconds
            >= explicitReadyMicroseconds
        && concurrent.qtConcurrentObservedReadyMicroseconds
            >= concurrent.qtConcurrentStartedMicroseconds
        && concurrent.qtConcurrentReleaseMicroseconds
            >= concurrent.qtConcurrentObservedReadyMicroseconds
        && explicitCompletedMicroseconds
            >= concurrent.qtConcurrentReleaseMicroseconds;
    if (!valid) {
        m_report->fail(
            u"thread-proof-invalid",
            u"worker overlap, identity, or result validation failed");
        return;
    }

    m_report->pass(
        u"explicit-pthread",
        QJsonObject{
            {QStringLiteral("inputNonce"),
             static_cast<qint64>(explicitState.inputNonce)},
            {QStringLiteral("explicitCompletedMicroseconds"),
             explicitCompletedMicroseconds},
            {QStringLiteral("explicitReadyMicroseconds"),
             explicitReadyMicroseconds},
            {QStringLiteral("isMainRuntimeThread"),
             explicitIsMain},
            {QStringLiteral("mainThreadId"),
             threadIdentityString(mainThreadId)},
            {QStringLiteral("overlapObserved"),
             overlapObserved},
            {QStringLiteral("threadId"),
             threadIdentityString(explicitThreadId)},
            {QStringLiteral("transformedNonce"),
             static_cast<qint64>(transformedNonce)},
        });
    m_report->pass(
        u"qt-concurrent",
        QJsonObject{
            {QStringLiteral("isMainRuntimeThread"),
             concurrent.isMainRuntimeThread},
            {QStringLiteral("overlapObserved"),
             concurrent.overlapObserved},
            {QStringLiteral("qtConcurrentObservedReadyMicroseconds"),
             concurrent.qtConcurrentObservedReadyMicroseconds},
            {QStringLiteral("qtConcurrentReleaseMicroseconds"),
             concurrent.qtConcurrentReleaseMicroseconds},
            {QStringLiteral("qtConcurrentStartedMicroseconds"),
             concurrent.qtConcurrentStartedMicroseconds},
            {QStringLiteral("result"), concurrent.result},
            {QStringLiteral("threadId"),
             threadIdentityString(concurrent.threadId)},
        });

    m_threadPassed = true;
    emit threadPassedChanged();
    maybeMarkCoreComplete();
}

void ProbeState::startJspiProbe()
{
    if (m_report->isTerminal()) {
        return;
    }
    const JspiNestedLoopResult result =
        m_jspiProbe->runJspiNestedLoop();
    const QJsonArray expectedNativeEnterOrder{1, 2, 3, 4};
    const QJsonArray expectedNativeExitOrder{4, 3, 2, 1};
    const bool passed =
        result.requestedNonce != 0
        && result.requestedNonce == result.resolvedNonce
        && result.promiseResolvedWhileExec
        && result.quitDelivered
        && result.postLoopSentinel
        && !result.watchdogTimedOut
        && result.fullPumpDeferredWhilePrimary
        && result.nativeComposedPathsIntact
        && result.nativeEventIdentitiesIntact
        && result.nativeStackCanariesIntact
        && result.primaryStackCanaryIntact
        && result.primaryStackCanaryObservedIntact
        && result.nativeEnterOrder == expectedNativeEnterOrder
        && result.nativeExitOrder == expectedNativeExitOrder
        && result.elapsedMicroseconds > 0;
    if (!passed) {
        m_report->fail(
            u"jspi-nested-loop",
            u"owned Promise did not resume inside the nested event loop");
        return;
    }

    m_report->pass(
        u"jspi-nested-loop",
        QJsonObject{
            {QStringLiteral("elapsedMicroseconds"),
             result.elapsedMicroseconds},
            {QStringLiteral("fullPumpDeferredWhilePrimary"),
             result.fullPumpDeferredWhilePrimary},
            {QStringLiteral("nativeEnterOrder"),
             result.nativeEnterOrder},
            {QStringLiteral("nativeComposedPathsIntact"),
             result.nativeComposedPathsIntact},
            {QStringLiteral("nativeEventIdentitiesIntact"),
             result.nativeEventIdentitiesIntact},
            {QStringLiteral("nativeExitOrder"),
             result.nativeExitOrder},
            {QStringLiteral("nativeStackCanariesIntact"),
             result.nativeStackCanariesIntact},
            {QStringLiteral("postLoopSentinel"),
             result.postLoopSentinel},
            {QStringLiteral("primaryStackCanaryIntact"),
             result.primaryStackCanaryIntact},
            {QStringLiteral("primaryStackCanaryObservedIntact"),
             result.primaryStackCanaryObservedIntact},
            {QStringLiteral("promiseResolvedWhileExec"),
             result.promiseResolvedWhileExec},
            {QStringLiteral("quitDelivered"),
             result.quitDelivered},
            {QStringLiteral("requestedNonce"),
             static_cast<qint64>(result.requestedNonce)},
            {QStringLiteral("resolvedNonce"),
             static_cast<qint64>(result.resolvedNonce)},
            {QStringLiteral("watchdogTimedOut"),
             result.watchdogTimedOut},
        });

    m_jspiProbe->startExclusiveSuspendGuardProbe(
        [this](ExclusiveSuspendGuardResult exclusiveResult) {
            if (m_report->isTerminal()) {
                return;
            }
            if (!exclusiveResult.passed) {
                m_report->append(
                    u"qt-exclusive-suspend-guard-observed",
                    exclusiveResult.detail);
                m_report->fail(
                    u"qt-exclusive-suspend-guard",
                    u"foreign event pumps changed Qt exclusive "
                    u"suspension state");
                return;
            }
            m_report->pass(
                u"qt-exclusive-suspend-guard",
                exclusiveResult.detail);
            maybeMarkCoreComplete();
        });
}

void ProbeState::publishRenderMilestone(
    const RenderFrameMilestone &milestone)
{
    m_report->append(
        u"qt-render-frame",
        QJsonObject{
            {QStringLiteral("capture"), milestone.capture},
            {QStringLiteral("captureFrameCount"),
             milestone.captureFrameCount},
            {QStringLiteral("contextAttributesResult"),
             milestone.contextAttributesResult},
            {QStringLiteral("contextHandle"),
             static_cast<qint64>(milestone.contextHandle)},
            {QStringLiteral("frameSequence"),
             static_cast<qint64>(milestone.frameSequence)},
            {QStringLiteral("generation"),
             static_cast<qint64>(milestone.generation)},
            {QStringLiteral("graphicsApi"),
             milestone.graphicsApiName()},
            {QStringLiteral("majorVersion"),
             milestone.majorVersion},
        });
}

void ProbeState::completeRenderCapture(
    const RenderFrameMilestone &milestone)
{
    if (!milestone.hasWebGl2Evidence()) {
        m_report->fail(
            u"qt-render-webgl2",
            u"Qt scene graph did not expose its current WebGL2 context");
        return;
    }

    if (milestone.capture == u"post-main") {
        m_postMainRenderReady = true;
        m_report->pass(
            u"qt-render-webgl2",
            QJsonObject{
                {QStringLiteral("contextAttributesResult"),
                 milestone.contextAttributesResult},
                {QStringLiteral("contextHandle"),
                 static_cast<qint64>(milestone.contextHandle)},
                {QStringLiteral("graphicsApi"),
                 milestone.graphicsApiName()},
                {QStringLiteral("majorVersion"),
                 milestone.majorVersion},
                {QStringLiteral("postMainFrameCount"),
                 milestone.captureFrameCount},
            });
        maybeResolveReady();
        maybeMarkCoreComplete();
    }
}

bool ProbeState::coreChecksComplete() const
{
    return m_report->hasPassed(u"static-library-exception")
        && m_report->hasPassed(u"qt-application-cycle-order")
        && m_report->hasPassed(u"explicit-pthread")
        && m_report->hasPassed(u"qt-concurrent")
        && m_report->hasPassed(u"jspi-nested-loop")
        && m_report->hasPassed(u"qt-exclusive-suspend-guard")
        && m_report->hasPassed(u"qt-render-webgl2");
}

void ProbeState::maybeResolveReady()
{
    if (m_readyResolved
        || m_report->isTerminal()
        || !m_qmlRootAttached
        || !m_mainReturning
        || !m_postMainObserved
        || !m_postMainRenderReady
        || !m_pingAcknowledged) {
        return;
    }
    m_readyResolved = true;
    m_report->setPhase(
        coreChecksComplete() ? u"core-complete" : u"core-ready");
    m_report->resolveReady();
    maybeMarkCoreComplete();
}

void ProbeState::maybeMarkCoreComplete()
{
    if (m_report->isTerminal()
        || !coreChecksComplete()) {
        return;
    }
    m_report->setPhase(u"core-complete");
}
