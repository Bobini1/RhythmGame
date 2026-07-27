#include "JspiNestedLoopProbe.h"

#include "BrowserRuntimeBridge.h"

#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QScopeGuard>
#include <QTimer>

#ifdef __EMSCRIPTEN__
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
struct ExclusiveSuspendProbeState
{
    QJsonArray deliveryOrder;
    QList<quint32> registeredHandlers;
    QJsonObject handlerIndices;
    JspiNestedLoopProbe::ExclusiveSuspendGuardCallback callback;
    QString error;
    bool payloadsValid = true;
    bool completionDelivered = false;
    bool completionFinalizationScheduled = false;
    bool ownerReturned = false;
    bool ownerResumedByExclusive = false;
    bool exclusiveClearedBeforeNormalDrain = false;
    bool normalDrainArmed = false;
    int completionDrainCount = 0;
    int exclusiveDrainCount = 0;
};

#endif

namespace
{
constexpr int stackCanaryLength = 256;
constexpr int primaryStackCanarySeed = 0x5A;
constexpr auto exclusiveNativeFinalizationDeadline =
    std::chrono::milliseconds{2500};

void fillStackCanary(
    std::array<std::uint8_t, stackCanaryLength> &canary,
    int seed)
{
    for (std::size_t index = 0; index < canary.size(); ++index) {
        canary[index] = static_cast<std::uint8_t>(
            (seed + (static_cast<int>(index) * 37)) & 0xFF);
    }
}

#ifdef __EMSCRIPTEN__
QJsonArray handlerQueue(
    std::initializer_list<quint32> handlers)
{
    QJsonArray queue;
    for (const quint32 handler : handlers) {
        queue.append(static_cast<qint64>(handler));
    }
    return queue;
}

bool hasExactGuardShape(const QJsonObject &guard)
{
    return guard.size() == 6
        && guard.value(u"exclusiveAfter").isDouble()
        && guard.value(u"exclusiveBefore").isDouble()
        && guard.value(u"ordinal").isDouble()
        && guard.value(u"pendingAfter").isArray()
        && guard.value(u"pendingBefore").isArray()
        && guard.value(u"result").isBool();
}

bool hasExactNormalShape(const QJsonObject &normal)
{
    return normal.size() == 5
        && normal.value(u"exclusiveAfter").isDouble()
        && normal.value(u"exclusiveBefore").isDouble()
        && normal.value(u"pendingAfter").isArray()
        && normal.value(u"pendingBefore").isArray()
        && normal.value(u"result").isBool();
}

bool eventHasNativeDepthOrdinal(
    const emscripten::val &event,
    int expectedOrdinal)
{
    try {
        return event["gate1bNativeDepthOrdinal"].as<int>()
            == expectedOrdinal;
    } catch (...) {
        return false;
    }
}

void recordExclusiveDelivery(
    const std::shared_ptr<ExclusiveSuspendProbeState> &state,
    const emscripten::val &event,
    QStringView label,
    std::string_view expectedPayload,
    bool completion = false)
{
    state->deliveryOrder.append(label.toString());
    try {
        state->payloadsValid =
            state->payloadsValid
            && event["gate1bExclusiveProbe"].as<std::string>()
                == expectedPayload;
    } catch (...) {
        state->payloadsValid = false;
    }
    if (completion) {
        state->completionDelivered = true;
        ++state->completionDrainCount;
    }
}
#endif
}

JspiNestedLoopProbe::JspiNestedLoopProbe(
    EventCallback eventCallback,
    QObject *parent)
    : QObject{parent}
    , m_eventCallback{std::move(eventCallback)}
{
#ifdef __EMSCRIPTEN__
    m_exclusiveFinalizationWatchdog = new QTimer{this};
    m_exclusiveFinalizationWatchdog->setSingleShot(true);
    connect(
        m_exclusiveFinalizationWatchdog,
        &QTimer::timeout,
        this,
        [this] {
            const auto state = m_exclusiveState;
            if (!state || state->completionFinalizationScheduled) {
                return;
            }
            state->completionFinalizationScheduled = true;
            state->error =
                QStringLiteral("exclusive-completion-native-watchdog");
            QTimer::singleShot(0, this, [this, state] {
                finishExclusiveSuspendGuardProbe(state);
            });
        });
#endif
}

JspiNestedLoopProbe::~JspiNestedLoopProbe()
{
#ifdef __EMSCRIPTEN__
    if (m_exclusiveState) {
        releaseExclusiveSuspendGuardProbe(m_exclusiveState);
    }
    releaseNativeBoundaryNegativeProbeHandlers();
#endif
}

JspiNestedLoopResult JspiNestedLoopProbe::runJspiNestedLoop()
{
    JspiNestedLoopResult result = {};
    result.requestedNonce = generateOwnedBrowserNonce();
    result.nativeComposedPathsIntact = true;
    result.nativeEventIdentitiesIntact = true;
    result.nativeStackCanariesIntact = true;
    std::array<std::uint8_t, stackCanaryLength> primaryCanary{};
    fillStackCanary(primaryCanary, primaryStackCanarySeed);

    quint32 firstNativeDepthHandler = 1U;
#ifdef __EMSCRIPTEN__
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    QList<quint32> registeredHandlers;
    const auto registeredHandlerGuard = qScopeGuard([&] {
        for (const quint32 handler : registeredHandlers) {
            control->removeEventHandler(handler);
        }
    });

    quint32 nextHandler = 0U;
    for (int ordinal = 4; ordinal >= 1; --ordinal) {
        const int canarySeed = 0x20 + ordinal;
        const quint32 handler = control->registerEventHandler(
            [&, ordinal, canarySeed, nextHandler](
                emscripten::val event) {
                std::array<std::uint8_t, stackCanaryLength>
                    nativeCanary{};
                fillStackCanary(nativeCanary, canarySeed);
                result.nativeEnterOrder.append(ordinal);
                emscripten::val composedPathBefore =
                    emscripten::val::undefined();
                try {
                    const emscripten::val currentEvent =
                        control->currentEvent();
                    composedPathBefore =
                        event.call<emscripten::val>("composedPath");
                    result.nativeEventIdentitiesIntact =
                        result.nativeEventIdentitiesIntact
                        && event.strictlyEquals(currentEvent)
                        && eventHasNativeDepthOrdinal(event, ordinal)
                        && eventHasNativeDepthOrdinal(
                            currentEvent,
                            ordinal);
                    result.nativeComposedPathsIntact =
                        result.nativeComposedPathsIntact
                        && composedPathBefore.isArray()
                        && composedPathBefore["length"].as<int>() > 0;
                } catch (...) {
                    result.nativeEventIdentitiesIntact = false;
                    result.nativeComposedPathsIntact = false;
                }

                if (ordinal < 4) {
                    result.nativeEventIdentitiesIntact =
                        result.nativeEventIdentitiesIntact
                        && dispatchNativeDepthProbeEvent(
                            nextHandler,
                            ordinal + 1);
                }

                try {
                    const emscripten::val currentEvent =
                        control->currentEvent();
                    const emscripten::val composedPathAfter =
                        event.call<emscripten::val>("composedPath");
                    result.nativeEventIdentitiesIntact =
                        result.nativeEventIdentitiesIntact
                        && event.strictlyEquals(currentEvent)
                        && eventHasNativeDepthOrdinal(event, ordinal)
                        && eventHasNativeDepthOrdinal(
                            currentEvent,
                            ordinal);
                    result.nativeComposedPathsIntact =
                        result.nativeComposedPathsIntact
                        && composedPathAfter.isArray()
                        && composedPathBefore.strictlyEquals(
                            composedPathAfter)
                        && composedPathAfter["length"].as<int>() > 0;
                } catch (...) {
                    result.nativeEventIdentitiesIntact = false;
                    result.nativeComposedPathsIntact = false;
                }
                result.nativeStackCanariesIntact =
                    result.nativeStackCanariesIntact
                    && verifyNativeStackCanary(
                        nativeCanary.data(),
                        static_cast<int>(nativeCanary.size()),
                        canarySeed);
                result.nativeExitOrder.append(ordinal);
            });
        registeredHandlers.append(handler);
        nextHandler = handler;
    }
    firstNativeDepthHandler = nextHandler;
#endif

    QEventLoop loop;
    QElapsedTimer timer;
    timer.start();

    m_eventCallback(
        u"jspi-before-exec",
        QJsonObject{{
            QStringLiteral("requestedNonce"),
            static_cast<qint64>(result.requestedNonce)}});

    bool execActive = true;
    QTimer::singleShot(0, &loop, [&] {
        m_eventCallback(
            u"jspi-before-import",
            QJsonObject{{
                QStringLiteral("requestedNonce"),
                static_cast<qint64>(result.requestedNonce)}});

        result.resolvedNonce = awaitOwnedBrowserNonce(
            result.requestedNonce,
            firstNativeDepthHandler,
            primaryCanary.data(),
            static_cast<int>(primaryCanary.size()),
            primaryStackCanarySeed,
            result.fullPumpDeferredWhilePrimary,
            result.primaryStackCanaryObservedIntact);
        if (result.resolvedNonce == jspiWatchdogSentinel()) {
            result.watchdogTimedOut = true;
            loop.exit(1);
            return;
        }

        result.promiseResolvedWhileExec = execActive;
        const quint32 requestedNonce = result.requestedNonce;
        const quint32 resolvedNonce = result.resolvedNonce;
        m_eventCallback(
            u"jspi-promise-resolved",
            QJsonObject{{
                QStringLiteral("resolvedNonce"),
                static_cast<qint64>(result.resolvedNonce)}});

        result.quitDelivered = true;
        m_eventCallback(
            u"jspi-quit-delivered",
            QJsonObject{{
                QStringLiteral("requestedNonceMatches"),
                requestedNonce == resolvedNonce}});
        loop.quit();
    });

    loop.exec();
    execActive = false;
    result.postLoopSentinel = true;
    result.primaryStackCanaryIntact = verifyNativeStackCanary(
        primaryCanary.data(),
        static_cast<int>(primaryCanary.size()),
        primaryStackCanarySeed);
    result.elapsedMicroseconds = timer.nsecsElapsed() / 1000;
    m_eventCallback(
        u"jspi-after-exec",
        QJsonObject{
            {QStringLiteral("elapsedMicroseconds"),
             result.elapsedMicroseconds},
            {QStringLiteral("postLoopSentinel"),
             result.postLoopSentinel},
            {QStringLiteral("requestedNonce"),
             static_cast<qint64>(result.requestedNonce)},
            {QStringLiteral("resolvedNonce"),
             static_cast<qint64>(result.resolvedNonce)},
        });

    return result;
}

bool JspiNestedLoopProbe::armNativeDepthLimitProbe()
{
#ifdef __EMSCRIPTEN__
    if (m_nativeBoundaryNegativeProbeArmed) {
        return false;
    }
    m_nativeBoundaryNegativeProbeArmed = true;
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();

    quint32 nextHandler = 0;
    for (int ordinal = 4; ordinal >= 1; --ordinal) {
        const quint32 handler = control->registerEventHandler(
            [this, ordinal, nextHandler](emscripten::val event) {
                if (!eventHasNativeDepthOrdinal(event, ordinal)
                    ) {
                    throw std::runtime_error{
                        "native-depth-limit-event-invalid"};
                }
                if (ordinal == 4) {
                    m_eventCallback(
                        u"qt-native-depth-limit-attempt",
                        QJsonObject{
                            {QStringLiteral("activeDepth"), 4},
                            {QStringLiteral("requestedDepth"), 5},
                        });
                    if (!dispatchNativeDepthLimitAttemptEvent()) {
                        throw std::runtime_error{
                            "native-depth-limit-attempt-dispatch-failed"};
                    }
                    return;
                }
                if (nextHandler == 0
                    || !dispatchNativeDepthProbeEvent(
                        nextHandler,
                        ordinal + 1)) {
                    throw std::runtime_error{
                        "native-depth-limit-dispatch-failed"};
                }
            });
        m_nativeBoundaryNegativeHandlers.append(handler);
        nextHandler = handler;
    }
    const bool scheduled =
        scheduleNativeDepthProbeEvent(nextHandler, 1);
    if (!scheduled) {
        releaseNativeBoundaryNegativeProbeHandlers();
    }
    return scheduled;
#else
    return false;
#endif
}

bool JspiNestedLoopProbe::armNativeSuspensionTrapProbe()
{
#ifdef __EMSCRIPTEN__
    if (m_nativeBoundaryNegativeProbeArmed) {
        return false;
    }
    m_nativeBoundaryNegativeProbeArmed = true;
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    const quint32 handler = control->registerEventHandler(
        [this](emscripten::val event) {
            if (!eventHasNativeDepthOrdinal(event, 1)) {
                throw std::runtime_error{
                    "native-suspension-trap-event-invalid"};
            }
            m_eventCallback(
                u"qt-native-suspension-attempt",
                QJsonObject{{QStringLiteral("depth"), 1}});
            static_cast<void>(awaitUnmatchedNativeSuspension());
            m_eventCallback(
                u"qt-native-suspension-returned",
                QJsonObject{{QStringLiteral("depth"), 1}});
        });
    m_nativeBoundaryNegativeHandlers.append(handler);
    const bool scheduled = scheduleNativeDepthProbeEvent(handler, 1);
    if (!scheduled) {
        releaseNativeBoundaryNegativeProbeHandlers();
    }
    return scheduled;
#else
    return false;
#endif
}

void JspiNestedLoopProbe::releaseNativeBoundaryNegativeProbeHandlers()
{
#ifdef __EMSCRIPTEN__
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    const QList<quint32> registeredHandlers =
        std::exchange(m_nativeBoundaryNegativeHandlers, {});
    for (const quint32 handler : registeredHandlers) {
        control->removeEventHandler(handler);
    }
#else
    m_nativeBoundaryNegativeHandlers.clear();
#endif
    m_nativeBoundaryNegativeProbeArmed = false;
}

void JspiNestedLoopProbe::startExclusiveSuspendGuardProbe(
    ExclusiveSuspendGuardCallback callback)
{
    if (!callback) {
        return;
    }
#ifdef __EMSCRIPTEN__
    if (m_exclusiveState) {
        callback(ExclusiveSuspendGuardResult{
            false,
            QJsonObject{{
                QStringLiteral("error"),
                QStringLiteral("exclusive-probe-already-active")}},
        });
        return;
    }

    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    const auto state =
        std::make_shared<ExclusiveSuspendProbeState>();
    state->callback = std::move(callback);
    m_exclusiveState = state;

    const auto registerHandler =
        [&](QStringView label,
            std::string_view payload,
            bool completion = false) {
            const quint32 handler = control->registerEventHandler(
                [this, state, label = label.toString(),
                 payload = std::string{payload}, completion](
                    emscripten::val event) {
                    recordExclusiveDelivery(
                        state,
                        event,
                        label,
                        payload,
                        completion);
                    if (completion
                        && !state->completionFinalizationScheduled) {
                        m_exclusiveFinalizationWatchdog->stop();
                        state->completionFinalizationScheduled = true;
                        QTimer::singleShot(0, this, [this, state] {
                            finishExclusiveSuspendGuardProbe(state);
                        });
                    }
                });
            state->registeredHandlers.append(handler);
            return handler;
        };
    const auto fail = [&](QStringView error) {
        state->error = error.toString();
        finishExclusiveSuspendGuardProbe(state);
    };

    try {
        const quint32 firstHandler =
            registerHandler(u"first", "first");
        const quint32 secondHandler =
            registerHandler(u"second", "second");
        const quint32 exclusiveHandler =
            registerHandler(u"exclusive", "exclusive");
        const quint32 completionHandler =
            registerHandler(u"completion", "completion", true);
        state->handlerIndices = QJsonObject{
            {QStringLiteral("completion"),
             static_cast<qint64>(completionHandler)},
            {QStringLiteral("exclusive"),
             static_cast<qint64>(exclusiveHandler)},
            {QStringLiteral("first"),
             static_cast<qint64>(firstHandler)},
            {QStringLiteral("second"),
             static_cast<qint64>(secondHandler)},
        };
        m_exclusiveFinalizationWatchdog->start(
            exclusiveNativeFinalizationDeadline);

        if (!scheduleExclusiveSuspendGuardProbe(
                firstHandler,
                secondHandler,
                exclusiveHandler,
                completionHandler)) {
            fail(u"browser-schedule-rejected");
            return;
        }

        control->suspendExclusive({exclusiveHandler});
        state->ownerResumedByExclusive =
            QWasmSuspendResumeControl::suspendResumeControlJs()
                ["exclusiveEventHandler"].as<quint32>()
            == exclusiveHandler;
        state->exclusiveDrainCount =
            control->sendPendingEvents();
        state->exclusiveClearedBeforeNormalDrain =
            QWasmSuspendResumeControl::suspendResumeControlJs()
                ["exclusiveEventHandler"].as<quint32>()
            == 0;
        state->normalDrainArmed =
            armExclusiveSuspendGuardNormalDrain();
        if (!state->normalDrainArmed) {
            fail(u"normal-drain-arm-rejected");
            return;
        }

        // The normal public pump and completion delivery must happen only
        // after this exclusive owner has returned to the browser.
        state->ownerReturned = true;
    } catch (const std::exception &error) {
        state->error = QString::fromUtf8(error.what());
        finishExclusiveSuspendGuardProbe(state);
    } catch (...) {
        fail(u"unknown-exclusive-probe-error");
    }
#else
    callback(ExclusiveSuspendGuardResult{
        false,
        QJsonObject{{
            QStringLiteral("error"),
            QStringLiteral("emscripten-required")}},
    });
#endif
}

void JspiNestedLoopProbe::finishExclusiveSuspendGuardProbe(
    const std::shared_ptr<ExclusiveSuspendProbeState> &state)
{
#ifdef __EMSCRIPTEN__
    if (!state || m_exclusiveState != state) {
        return;
    }

    ExclusiveSuspendGuardResult result = {};
    try {
        if (!state->error.isEmpty()) {
            result.detail = QJsonObject{{
                QStringLiteral("error"),
                state->error}};
        } else {
            const QJsonObject browserObservation =
                takeExclusiveSuspendGuardProbeResult();
            const QJsonArray guards =
                browserObservation.value(u"guards").toArray();
            const bool exclusiveDomDispatch =
                browserObservation.value(u"exclusiveDomDispatch").toBool();
            const QJsonObject firstGuard =
                guards.size() > 0
                ? guards.at(0).toObject()
                : QJsonObject{};
            const QJsonObject secondGuard =
                guards.size() > 1
                ? guards.at(1).toObject()
                : QJsonObject{};
            const QJsonObject normal =
                browserObservation.value(u"normal").toObject();
            const quint32 firstHandler = static_cast<quint32>(
                state->handlerIndices.value(u"first").toInteger());
            const quint32 secondHandler = static_cast<quint32>(
                state->handlerIndices.value(u"second").toInteger());
            const quint32 exclusiveHandler = static_cast<quint32>(
                state->handlerIndices.value(u"exclusive").toInteger());
            const QJsonArray firstQueue =
                handlerQueue({firstHandler});
            const QJsonArray secondQueue =
                handlerQueue({firstHandler, secondHandler});
            const bool firstGuardValid =
                hasExactGuardShape(firstGuard)
                && firstGuard.value(u"ordinal").toInt() == 1
                && firstGuard.value(u"exclusiveBefore").toInt()
                    == static_cast<int>(exclusiveHandler)
                && firstGuard.value(u"exclusiveAfter").toInt()
                    == static_cast<int>(exclusiveHandler)
                && firstGuard.value(u"pendingBefore").toArray()
                    == firstQueue
                && firstGuard.value(u"pendingAfter").toArray()
                    == firstQueue
                && !firstGuard.value(u"result").toBool(true);
            const bool secondGuardValid =
                hasExactGuardShape(secondGuard)
                && secondGuard.value(u"ordinal").toInt() == 2
                && secondGuard.value(u"exclusiveBefore").toInt()
                    == static_cast<int>(exclusiveHandler)
                && secondGuard.value(u"exclusiveAfter").toInt()
                    == static_cast<int>(exclusiveHandler)
                && secondGuard.value(u"pendingBefore").toArray()
                    == secondQueue
                && secondGuard.value(u"pendingAfter").toArray()
                    == secondQueue
                && !secondGuard.value(u"result").toBool(true);
            const QJsonArray normalPendingBefore =
                normal.value(u"pendingBefore").toArray();
            const bool normalDrainOwnedQueue =
                normalPendingBefore == secondQueue;
            const bool earlierOrdinaryPumpOwnedQueue =
                normalPendingBefore.isEmpty();
            const bool normalValid =
                hasExactNormalShape(normal)
                && normal.value(u"exclusiveBefore").toInt() == 0
                && normal.value(u"exclusiveAfter").toInt() == 0
                // The scheduled drain and bootstrap's already-registered
                // idle frame are both valid post-owner ordinary pumps.
                && (normalDrainOwnedQueue
                    || earlierOrdinaryPumpOwnedQueue)
                && normal.value(u"pendingAfter").toArray().isEmpty()
                && normal.value(u"result").toBool(false);
            const bool queuePreserved =
                firstGuardValid && secondGuardValid && normalValid;
            const bool completionFinalizedAfterOwnerReturn =
                state->ownerReturned && state->completionDelivered;
            const QJsonArray expectedDeliveryOrder{
                QStringLiteral("exclusive"),
                QStringLiteral("first"),
                QStringLiteral("second"),
                QStringLiteral("completion"),
            };
            const QJsonArray foreignDrainResults{
                firstGuard.value(u"result"),
                secondGuard.value(u"result"),
            };

            result.detail = QJsonObject{
                {QStringLiteral("completionDrainCount"),
                 state->completionDrainCount},
                {QStringLiteral(
                     "completionFinalizedAfterOwnerReturn"),
                 completionFinalizedAfterOwnerReturn},
                {QStringLiteral("deliveryOrder"),
                 state->deliveryOrder},
                {QStringLiteral(
                     "exclusiveClearedBeforeNormalDrain"),
                 state->exclusiveClearedBeforeNormalDrain},
                {QStringLiteral("exclusiveDrainCount"),
                 state->exclusiveDrainCount},
                {QStringLiteral("exclusiveDomDispatch"),
                 exclusiveDomDispatch},
                {QStringLiteral("foreignDrainResults"),
                 foreignDrainResults},
                {QStringLiteral("guardObservations"), guards},
                {QStringLiteral("handlerIndices"),
                 state->handlerIndices},
                {QStringLiteral("normalDrainResult"),
                 normal.value(u"result")},
                {QStringLiteral("normalDrainArmed"),
                 state->normalDrainArmed},
                {QStringLiteral("normalObservation"), normal},
                {QStringLiteral("ownerResumedByExclusive"),
                 state->ownerResumedByExclusive},
                {QStringLiteral("payloadsValid"),
                 state->payloadsValid},
                {QStringLiteral("queuePreserved"),
                 queuePreserved},
            };
            result.passed =
                browserObservation.size() == 4
                && browserObservation.value(u"error").isNull()
                && exclusiveDomDispatch
                && guards.size() == 2
                && state->ownerResumedByExclusive
                && state->exclusiveDrainCount == 1
                && state->exclusiveClearedBeforeNormalDrain
                && state->normalDrainArmed
                && state->completionDrainCount == 1
                && completionFinalizedAfterOwnerReturn
                && state->payloadsValid
                && state->deliveryOrder == expectedDeliveryOrder
                && queuePreserved;
        }
    } catch (const std::exception &error) {
        result.detail = QJsonObject{
            {QStringLiteral("error"),
             QString::fromUtf8(error.what())}};
    } catch (...) {
        result.detail = QJsonObject{{
            QStringLiteral("error"),
            QStringLiteral("unknown-exclusive-probe-error")}};
    }

    auto callback = std::move(state->callback);
    releaseExclusiveSuspendGuardProbe(state);
    if (callback) {
        callback(std::move(result));
    }
#else
    Q_UNUSED(state);
#endif
}

void JspiNestedLoopProbe::releaseExclusiveSuspendGuardProbe(
    const std::shared_ptr<ExclusiveSuspendProbeState> &state)
{
#ifdef __EMSCRIPTEN__
    if (!state || m_exclusiveState != state) {
        return;
    }

    m_exclusiveFinalizationWatchdog->stop();
    cancelExclusiveSuspendGuardProbe();
    QWasmSuspendResumeControl *control =
        QWasmSuspendResumeControl::get();
    for (const quint32 handler : state->registeredHandlers) {
        control->removeEventHandler(handler);
    }
    state->registeredHandlers.clear();
    m_exclusiveState.reset();
#else
    Q_UNUSED(state);
#endif
}
