#include "BrowserRuntimeBridge.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QThread>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/bind.h>
#endif

namespace
{
constexpr unsigned secureContextCapability = 1U << 0U;
constexpr unsigned crossOriginIsolatedCapability = 1U << 1U;
constexpr unsigned sharedArrayBufferCapability = 1U << 2U;
constexpr unsigned jspiApiCapability = 1U << 3U;
constexpr unsigned webGl2ApiCapability = 1U << 4U;
constexpr unsigned audioWorkletCapability = 1U << 5U;
constexpr unsigned opfsCapability = 1U << 6U;
constexpr unsigned fileSystemAccessCapability = 1U << 7U;
constexpr int foregroundLatencySampleCount = 64;
constexpr std::size_t maximumRuntimeCommandNameBytes = 64;
constexpr std::size_t maximumRuntimeCommandPayloadBytes = 4096;
constexpr qsizetype maximumMediaCaptureRequestIdBytes = 64;
constexpr qsizetype maximumMediaCaptureRequestIds = 16;

struct RuntimeCommandDefinition
{
    BrowserRuntimeCommand command;
    QStringView name;
};

constexpr std::array<RuntimeCommandDefinition, 9>
    runtimeCommandDefinitions{{
        {
            BrowserRuntimeCommand::AcknowledgeMediaFrameCapture,
            u"ack-media-frame-capture",
        },
        {
            BrowserRuntimeCommand::ArmBfcacheResumeProbe,
            u"arm-bfcache-resume-probe",
        },
        {
            BrowserRuntimeCommand::ArmHiddenTimerProbe,
            u"arm-hidden-timer-probe",
        },
        {
            BrowserRuntimeCommand::ArmVisibleResumeTimerProbe,
            u"arm-visible-resume-timer-probe",
        },
        {
            BrowserRuntimeCommand::BeginForegroundLatencySampling,
            u"begin-foreground-latency-sampling",
        },
        {
            BrowserRuntimeCommand::ProbePing,
            u"probe-ping",
        },
        {
            BrowserRuntimeCommand::SetShaderPhase,
            u"set-shader-phase",
        },
        {
            BrowserRuntimeCommand::TriggerNativeDepthLimit,
            u"trigger-native-depth-limit",
        },
        {
            BrowserRuntimeCommand::TriggerNativeSuspensionTrap,
            u"trigger-native-suspension-trap",
        },
    }};

QPointer<QObject> commandOwner;
BrowserRuntimeCommandHandler commandHandler;
std::uint64_t commandHandlerGeneration = 0;

[[noreturn]] void rejectCommand(const char *code)
{
    throw std::runtime_error{code};
}

bool isFinitePhase(double phase)
{
    return std::isfinite(phase)
        && (phase == 0.20 || phase == 0.80);
}

#ifdef __EMSCRIPTEN__
template<typename Command>
QJsonObject mediaTrackerCommandResult(Command command)
{
    QByteArray encoded(4096, '\0');
    int byteLength = command(encoded.data(), encoded.size());
    if (byteLength < -1) {
        encoded.resize(-byteLength);
        byteLength = command(encoded.data(), encoded.size());
    }
    if (byteLength < 0 || byteLength >= encoded.size()) {
        return {};
    }
    encoded.resize(byteLength);
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()
        || document.toJson(QJsonDocument::Compact) != encoded) {
        return {};
    }
    return document.object();
}
#endif

BrowserRuntimeCommand parseRuntimeCommand(QStringView command)
{
    for (const RuntimeCommandDefinition &definition :
         runtimeCommandDefinitions) {
        if (definition.name == command) {
            return definition.command;
        }
    }
    rejectCommand("runtime-command-not-allowed");
}

void validateRuntimeCommandPayload(
    BrowserRuntimeCommand command,
    const QJsonObject &payload)
{
    if (command
        == BrowserRuntimeCommand::AcknowledgeMediaFrameCapture) {
        if (payload.size() != 2
            || !payload.contains(u"requestIds")
            || !payload.contains(u"runNonce")
            || !payload.value(u"requestIds").isArray()
            || !payload.value(u"runNonce").isDouble()) {
            rejectCommand("ack-media-frame-capture-payload-keys");
        }
        const double runNonce = payload.value(u"runNonce").toDouble();
        if (!std::isfinite(runNonce)
            || std::floor(runNonce) != runNonce
            || runNonce <= 0
            || runNonce > 4294967294.0) {
            rejectCommand("ack-media-frame-capture-run-nonce");
        }
        const QJsonArray requestIds = payload.value(u"requestIds").toArray();
        if (requestIds.isEmpty()
            || requestIds.size() > maximumMediaCaptureRequestIds) {
            rejectCommand("ack-media-frame-capture-request-id-count");
        }
        static const QRegularExpression requestIdPattern{
            QStringLiteral("^request-[1-9][0-9]*$")};
        QSet<QString> observedRequestIds;
        for (const QJsonValue &value : requestIds) {
            if (!value.isString()) {
                rejectCommand("ack-media-frame-capture-request-id");
            }
            const QString requestId = value.toString();
            if (requestId.toUtf8().size()
                    > maximumMediaCaptureRequestIdBytes
                || !requestIdPattern.match(requestId).hasMatch()
                || observedRequestIds.contains(requestId)) {
                rejectCommand("ack-media-frame-capture-request-id");
            }
            observedRequestIds.insert(requestId);
        }
        return;
    }

    if (command == BrowserRuntimeCommand::ArmBfcacheResumeProbe
        || command == BrowserRuntimeCommand::ArmHiddenTimerProbe
        || command
            == BrowserRuntimeCommand::ArmVisibleResumeTimerProbe
        || command == BrowserRuntimeCommand::ProbePing
        || command == BrowserRuntimeCommand::TriggerNativeDepthLimit
        || command
            == BrowserRuntimeCommand::TriggerNativeSuspensionTrap) {
        if (!payload.isEmpty()) {
            rejectCommand("runtime-command-payload-keys");
        }
        return;
    }

    if (command == BrowserRuntimeCommand::SetShaderPhase) {
        if (payload.size() != 1 || !payload.contains(u"phase")) {
            rejectCommand("set-shader-phase-payload-keys");
        }
        const QJsonValue phaseValue = payload.value(u"phase");
        if (!phaseValue.isDouble()
            || !isFinitePhase(phaseValue.toDouble())) {
            rejectCommand("set-shader-phase-value");
        }
        return;
    }

    if (command
        == BrowserRuntimeCommand::BeginForegroundLatencySampling) {
        if (payload.size() != 1
            || !payload.contains(u"sampleCount")
            || !payload.value(u"sampleCount").isDouble()
            || payload.value(u"sampleCount").toDouble()
                != foregroundLatencySampleCount) {
            rejectCommand(
                "begin-foreground-latency-sampling-payload");
        }
        return;
    }

    rejectCommand("runtime-command-not-allowed");
}

#ifdef __EMSCRIPTEN__
QString normalizedRuntimeCommandErrorCode(const char *message)
{
    const QString errorCode = QString::fromUtf8(message);
    static const QRegularExpression stableErrorCodePattern{
        QStringLiteral("^[a-z][a-z0-9-]{0,127}$")};
    if (!stableErrorCodePattern.match(errorCode).hasMatch()) {
        return QStringLiteral("runtime-command-native-failure");
    }
    return errorCode;
}

std::string encodeRuntimeCommandSuccess(const QJsonObject &reply)
{
    return QJsonDocument{QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("reply"), reply},
    }}.toJson(QJsonDocument::Compact).toStdString();
}

std::string encodeRuntimeCommandFailure(const QString &errorCode)
{
    return QJsonDocument{QJsonObject{
        {QStringLiteral("error"), errorCode},
        {QStringLiteral("ok"), false},
    }}.toJson(QJsonDocument::Compact).toStdString();
}

bool browserPathAuthorizesRuntimeCommand(
    BrowserRuntimeCommand command)
{
    const char *expectedPath = nullptr;
    switch (command) {
    case BrowserRuntimeCommand::TriggerNativeDepthLimit:
        expectedPath =
            "/negative/native-depth-limit/RhythmGameWasmProbe.html";
        break;
    case BrowserRuntimeCommand::TriggerNativeSuspensionTrap:
        expectedPath =
            "/negative/native-suspension-trap/RhythmGameWasmProbe.html";
        break;
    default:
        return true;
    }

    const emscripten::val location =
        emscripten::val::global("location");
    const emscripten::val pathname = location["pathname"];
    return pathname.isString()
        && pathname.as<std::string>() == expectedPath;
}

std::string rhythmGameGate1bCommand(
    emscripten::val commandValue,
    emscripten::val payloadValue)
{
    try {
        if (!commandValue.isString() || !payloadValue.isString()) {
            rejectCommand("runtime-command-arguments-not-strings");
        }
        const std::size_t commandLength =
            commandValue["length"].as<std::size_t>();
        const std::size_t payloadLength =
            payloadValue["length"].as<std::size_t>();
        if (commandLength > maximumRuntimeCommandNameBytes) {
            rejectCommand("runtime-command-name-too-large");
        }
        if (payloadLength > maximumRuntimeCommandPayloadBytes) {
            rejectCommand("runtime-command-payload-too-large");
        }
        if (commandOwner.isNull() || !commandHandler) {
            rejectCommand("runtime-command-handler-missing");
        }
        if (QThread::currentThread() != commandOwner->thread()) {
            rejectCommand("runtime-command-wrong-thread");
        }

        const std::string commandUtf8 =
            commandValue.as<std::string>();
        const std::string payloadUtf8 =
            payloadValue.as<std::string>();
        if (commandUtf8.size() > maximumRuntimeCommandNameBytes) {
            rejectCommand("runtime-command-name-too-large");
        }
        if (payloadUtf8.size() > maximumRuntimeCommandPayloadBytes) {
            rejectCommand("runtime-command-payload-too-large");
        }
        const QString commandName = QString::fromUtf8(
            commandUtf8.data(),
            static_cast<qsizetype>(commandUtf8.size()));
        const BrowserRuntimeCommand command =
            parseRuntimeCommand(commandName);
        if (!browserPathAuthorizesRuntimeCommand(command)) {
            rejectCommand("runtime-command-native-route");
        }

        const QByteArray payloadBytes = QByteArray{
            payloadUtf8.data(),
            static_cast<qsizetype>(payloadUtf8.size())};
        QJsonParseError parseError;
        const QJsonDocument payloadDocument = QJsonDocument::fromJson(
            payloadBytes,
            &parseError);
        if (parseError.error != QJsonParseError::NoError
            || !payloadDocument.isObject()) {
            rejectCommand("runtime-command-json-invalid");
        }
        if (payloadDocument.toJson(QJsonDocument::Compact)
            != payloadBytes) {
            rejectCommand("runtime-command-json-not-compact");
        }

        const QJsonObject payload = payloadDocument.object();
        validateRuntimeCommandPayload(command, payload);
        const QJsonObject reply = commandHandler(command, payload);
        return encodeRuntimeCommandSuccess(reply);
    } catch (const std::exception &error) {
        return encodeRuntimeCommandFailure(
            normalizedRuntimeCommandErrorCode(error.what()));
    } catch (...) {
        return encodeRuntimeCommandFailure(
            QStringLiteral("runtime-command-native-failure"));
    }
}
#endif
}

#ifdef __EMSCRIPTEN__
EM_JS(unsigned, rgGate1bBrowserCapabilityBits, (), {
    let bits = 0;
    if (globalThis.isSecureContext === true) {
        bits |= 1 << 0;
    }
    if (globalThis.crossOriginIsolated === true) {
        bits |= 1 << 1;
    }
    if (typeof globalThis.SharedArrayBuffer === "function") {
        bits |= 1 << 2;
    }
    if (
        typeof WebAssembly.Suspending === "function"
        && typeof WebAssembly.promising === "function"
    ) {
        bits |= 1 << 3;
    }
    if (typeof globalThis.WebGL2RenderingContext === "function") {
        bits |= 1 << 4;
    }
    if (
        typeof globalThis.AudioWorkletNode === "function"
        && typeof globalThis.AudioContext === "function"
        && "audioWorklet" in globalThis.AudioContext.prototype
    ) {
        bits |= 1 << 5;
    }
    if (
        navigator.storage !== undefined
        && typeof navigator.storage.getDirectory === "function"
    ) {
        bits |= 1 << 6;
    }
    if (typeof globalThis.showDirectoryPicker === "function") {
        bits |= 1 << 7;
    }
    return bits >>> 0;
});

EM_JS(int, rgPublishGate1bEvent, (const char *json, int expectedSequence), {
    try {
        const event = JSON.parse(UTF8ToString(json));
        const acceptedSequence =
            globalThis.__rhythmGameGate1b.appendEvent(event);
        return acceptedSequence === expectedSequence ? 1 : 0;
    } catch {
        return 0;
    }
});

EM_JS(int, rgPublishGate1bSnapshot, (const char *json), {
    try {
        const snapshot = JSON.parse(UTF8ToString(json));
        return globalThis.__rhythmGameGate1b.publishSnapshot(snapshot)
            ? 1
            : 0;
    } catch {
        return 0;
    }
});

EM_JS(int, rgResolveGate1bReady, (const char *json), {
    try {
        const snapshot = JSON.parse(UTF8ToString(json));
        return globalThis.__rhythmGameGate1b.resolveReady(snapshot)
            ? 1
            : 0;
    } catch {
        return 0;
    }
});

EM_JS(void, rgRejectGate1bReady, (const char *json), {
    try {
        const payload = JSON.parse(UTF8ToString(json));
        globalThis.__rhythmGameGate1b.rejectReady(payload);
    } catch {
        // bootstrap owns the first terminal record and rejects later writes
    }
});

EM_JS(int, rgBrowserUserActivationIsActive, (), {
    return navigator.userActivation?.isActive === true ? 1 : 0;
});

EM_JS(int, rgBeginOwnedMediaBackendTracking, (uint32_t runNonce), {
    const tracker =
        globalThis.__rhythmGameGate1bMediaBackendTracker;
    return tracker !== null
        && typeof tracker === "object"
        && typeof tracker.begin === "function"
        && tracker.begin(runNonce >>> 0) === true
        ? 1
        : 0;
});

EM_JS(
    int,
    rgArmOwnedMediaSeekTracking,
    (
        uint32_t runNonce,
        int targetPositionMilliseconds,
        char *output,
        int capacity
    ),
    {
        const tracker =
            globalThis.__rhythmGameGate1bMediaBackendTracker;
        if (
            tracker === null
            || typeof tracker !== "object"
            || typeof tracker.armSeek !== "function"
        ) {
            return -1;
        }
        const result = tracker.armSeek(
            runNonce >>> 0,
            targetPositionMilliseconds,
        );
        if (result === null || typeof result !== "object") {
            return -1;
        }
        const json = JSON.stringify(result);
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        return byteLength;
    });

EM_JS(
    int,
    rgFinishOwnedMediaSeekTracking,
    (uint32_t runNonce, char *output, int capacity),
    {
        const tracker =
            globalThis.__rhythmGameGate1bMediaBackendTracker;
        if (
            tracker === null
            || typeof tracker !== "object"
            || typeof tracker.finishSeek !== "function"
        ) {
            return -1;
        }
        const result = tracker.finishSeek(runNonce >>> 0);
        if (result === null || typeof result !== "object") {
            return -1;
        }
        const json = JSON.stringify(result);
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        return byteLength;
    });

EM_JS(
    int,
    rgArmOwnedMediaBackendRemoval,
    (uint32_t runNonce, char *output, int capacity),
    {
        const tracker =
            globalThis.__rhythmGameGate1bMediaBackendTracker;
        if (
            tracker === null
            || typeof tracker !== "object"
            || typeof tracker.armRemoval !== "function"
        ) {
            return -1;
        }
        const result = tracker.armRemoval(runNonce >>> 0);
        if (result === null || typeof result !== "object") {
            return -1;
        }
        const json = JSON.stringify(result);
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        return byteLength;
    });

EM_JS(
    int,
    rgFinishOwnedMediaBackendRemoval,
    (
        uint32_t runNonce,
        int abandon,
        char *output,
        int capacity
    ),
    {
        const tracker =
            globalThis.__rhythmGameGate1bMediaBackendTracker;
        if (
            tracker === null
            || typeof tracker !== "object"
            || typeof tracker.finish !== "function"
        ) {
            return -1;
        }
        const result = tracker.finish(
            runNonce >>> 0,
            abandon === 1,
        );
        if (result === null || typeof result !== "object") {
            return -1;
        }
        const json = JSON.stringify(result);
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        return byteLength;
    });

EM_JS(
    int,
    rgReleaseOwnedMediaBackendRemoval,
    (uint32_t runNonce, char *output, int capacity),
    {
        const tracker =
            globalThis.__rhythmGameGate1bMediaBackendTracker;
        if (
            tracker === null
            || typeof tracker !== "object"
            || typeof tracker.release !== "function"
        ) {
            return -1;
        }
        const result = tracker.release(runNonce >>> 0);
        if (result === null || typeof result !== "object") {
            return -1;
        }
        const json = JSON.stringify(result);
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        return byteLength;
    });

EM_JS(
    int,
    rgScheduleExclusiveSuspendGuardProbe,
    (
        uint32_t firstHandler,
        uint32_t secondHandler,
        uint32_t exclusiveHandler,
        uint32_t completionHandler
    ),
    {
        const stateKey = "__rhythmGameExclusiveSuspendGuardProbe";
        const exclusiveFinalizationWatchdogMilliseconds = 2000;
        if (Object.hasOwn(Module, stateKey)) {
            return 0;
        }
        const control = Module.qtSuspendResumeControl;
        const pump = Module.qtSendPendingEvents;
        if (
            control === null
            || typeof control !== "object"
            || !Array.isArray(control.pendingEvents)
            || control.eventHandlers === null
            || typeof control.eventHandlers !== "object"
            || typeof pump !== "function"
            || control.exclusiveEventHandler !== 0
            || control.resume !== null
        ) {
            return 0;
        }

        const state = {
            error: null,
            exclusiveDomDispatch: false,
            guards: [],
            normal: null,
            normalArmed: false,
            completionScheduled: false,
            timerIds: [],
        };
        Object.defineProperty(Module, stateKey, {
            configurable: true,
            enumerable: false,
            value: state,
            writable: false,
        });
        const boundPump = pump.bind(Module);
        const probeHandlers = new Set([
            firstHandler >>> 0,
            secondHandler >>> 0,
            exclusiveHandler >>> 0,
            completionHandler >>> 0,
        ]);
        const pendingIndices = () => control.pendingEvents
            .map((event) => event.index >>> 0)
            .filter((index) => probeHandlers.has(index));
        const invoke = (index, label, throughDom = false) => {
            const handler = control.eventHandlers[index];
            if (typeof handler !== "function") {
                throw new Error(`exclusive-probe-handler-${label}`);
            }
            if (throughDom) {
                const screen = document.getElementById("screen");
                if (!(screen instanceof HTMLElement)) {
                    throw new Error("exclusive-probe-screen");
                }
                let invoked = false;
                const capture = (event) => {
                    invoked = true;
                    handler(event);
                };
                const event = new MouseEvent("click", {
                    bubbles: true,
                    cancelable: true,
                    composed: true,
                });
                Object.defineProperty(event, "gate1bExclusiveProbe", {
                    configurable: false,
                    enumerable: true,
                    value: label,
                    writable: false,
                });
                screen.addEventListener("click", capture, {
                    capture: true,
                    once: true,
                });
                try {
                    screen.dispatchEvent(event);
                } finally {
                    screen.removeEventListener("click", capture, true);
                }
                if (!invoked) {
                    throw new Error("exclusive-probe-dom-dispatch");
                }
                state.exclusiveDomDispatch = true;
                return;
            }
            handler({ gate1bExclusiveProbe: label });
        };
        const guard = async (ordinal) => {
            const before = pendingIndices();
            const exclusiveBefore =
                control.exclusiveEventHandler >>> 0;
            const result = await boundPump();
            state.guards.push({
                exclusiveAfter:
                    control.exclusiveEventHandler >>> 0,
                exclusiveBefore,
                ordinal,
                pendingAfter: pendingIndices(),
                pendingBefore: before,
                result,
            });
        };
        const schedule = (callback, delay) => {
            state.timerIds.push(setTimeout(callback, delay));
        };
        let recordError = () => {};
        const scheduleCompletion = () => {
            if (state.completionScheduled) {
                return;
            }
            state.completionScheduled = true;
            // Let the ordinary promising export unwind before queuing the
            // completion event for Qt's normal idle pump. This is also the
            // bounded terminal path when any probe handler throws.
            schedule(() => {
                try {
                    invoke(completionHandler >>> 0, "completion");
                } catch (error) {
                    recordError(error);
                }
            }, 0);
        };
        recordError = (error) => {
            if (state.error === null) {
                state.error = String(error);
            }
            scheduleCompletion();
        };
        const normalDrain = async () => {
            try {
                const before = pendingIndices();
                const exclusiveBefore =
                    control.exclusiveEventHandler >>> 0;
                const result = await boundPump();
                state.normal = {
                    exclusiveAfter:
                        control.exclusiveEventHandler >>> 0,
                    exclusiveBefore,
                    pendingAfter: pendingIndices(),
                    pendingBefore: before,
                    result,
                };
            } catch (error) {
                recordError(error);
            } finally {
                scheduleCompletion();
            }
        };
        Object.defineProperty(state, "normalDrain", {
            configurable: false,
            enumerable: false,
            value: normalDrain,
            writable: false,
        });
        schedule(
            scheduleCompletion,
            exclusiveFinalizationWatchdogMilliseconds,
        );

        schedule(() => {
            try {
                invoke(firstHandler >>> 0, "first");
            } catch (error) {
                recordError(error);
            }
        }, 5);
        schedule(() => {
            void guard(1).catch(recordError);
        }, 20);
        schedule(() => {
            try {
                invoke(secondHandler >>> 0, "second");
            } catch (error) {
                recordError(error);
            }
        }, 35);
        schedule(() => {
            void guard(2).catch(recordError);
        }, 50);
        schedule(() => {
            try {
                invoke(exclusiveHandler >>> 0, "exclusive", true);
            } catch (error) {
                recordError(error);
            }
        }, 75);
        return 1;
    });

EM_JS(int, rgArmExclusiveSuspendGuardNormalDrain, (), {
    const stateKey = "__rhythmGameExclusiveSuspendGuardProbe";
    const state = Module[stateKey];
    const control = Module.qtSuspendResumeControl;
    if (
        state === null
        || typeof state !== "object"
        || state.normalArmed !== false
        || typeof state.normalDrain !== "function"
        || control === null
        || typeof control !== "object"
        || control.exclusiveEventHandler !== 0
        || control.resume !== null
    ) {
        return 0;
    }
    state.normalArmed = true;
    state.timerIds.push(setTimeout(() => {
        void state.normalDrain();
    }, 0));
    return 1;
});

EM_JS(
    int,
    rgTakeExclusiveSuspendGuardProbeResult,
    (char *output, int capacity),
    {
        const stateKey = "__rhythmGameExclusiveSuspendGuardProbe";
        const state = Module[stateKey];
        if (
            state === null
            || typeof state !== "object"
            || !Array.isArray(state.guards)
            || !Array.isArray(state.timerIds)
            || state.normalArmed !== true
        ) {
            return -1;
        }
        const json = JSON.stringify({
            error: state.error,
            exclusiveDomDispatch: state.exclusiveDomDispatch,
            guards: state.guards,
            normal: state.normal,
        });
        const byteLength = lengthBytesUTF8(json);
        if (byteLength + 1 > capacity) {
            return -(byteLength + 1);
        }
        stringToUTF8(json, output, capacity);
        for (const timerId of state.timerIds) {
            clearTimeout(timerId);
        }
        delete Module[stateKey];
        return byteLength;
    });

EM_JS(void, rgCancelExclusiveSuspendGuardProbe, (), {
    const stateKey = "__rhythmGameExclusiveSuspendGuardProbe";
    const state = Module[stateKey];
    if (state !== null && typeof state === "object") {
        if (Array.isArray(state.timerIds)) {
            for (const timerId of state.timerIds) {
                clearTimeout(timerId);
            }
        }
        delete Module[stateKey];
    }
});

EM_JS(uint32_t, rgGenerateOwnedBrowserNonce, (), {
    if (
        globalThis.crypto === undefined
        || typeof globalThis.crypto.getRandomValues !== "function"
    ) {
        return 0;
    }
    const nonce = new Uint32Array(1);
    for (let attempt = 0; attempt < 8; ++attempt) {
        crypto.getRandomValues(nonce);
        if (nonce[0] !== 0 && nonce[0] !== 0xFFFFFFFF) {
            return nonce[0] >>> 0;
        }
    }
    return 0;
});

EM_ASYNC_JS(
    uint32_t,
    rgGate1bAwaitOwnedNonce,
    (
        uint32_t requestedNonce,
        uint32_t firstNativeDepthHandler,
        const uint8_t *primaryStackCanary,
        int primaryStackCanaryLength,
        int primaryStackCanarySeed,
        int *fullPumpDeferred,
        int *primaryStackCanaryObservedIntact
    ),
    {
        const jspiWatchdogSentinel = 0xFFFFFFFF;
        const report = globalThis.__rhythmGameGate1b;
        HEAP32[fullPumpDeferred >> 2] = 0;
        HEAP32[primaryStackCanaryObservedIntact >> 2] = 0;
        const stackCanaryIntact = () => {
            if (
                primaryStackCanaryLength <= 0
                || primaryStackCanarySeed < 0
                || primaryStackCanarySeed > 255
            ) {
                return false;
            }
            for (let index = 0; index < primaryStackCanaryLength; ++index) {
                const expected =
                    (primaryStackCanarySeed + (index * 37)) & 0xFF;
                if (
                    HEAPU8[(primaryStackCanary + index) >>> 0]
                    !== expected
                ) {
                    return false;
                }
            }
            return true;
        };
        const dispatchNativeDepthEvent = (handlerIndex, ordinal) => {
            const control = Module.qtSuspendResumeControl;
            const handler = control?.eventHandlers?.[handlerIndex >>> 0];
            const screen = document.getElementById("screen");
            if (
                typeof handler !== "function"
                || !(screen instanceof HTMLElement)
            ) {
                return false;
            }
            let invoked = false;
            const capture = (event) => {
                invoked = true;
                handler(event);
            };
            const event = new MouseEvent("click", {
                bubbles: true,
                cancelable: true,
                composed: true,
            });
            Object.defineProperty(event, "gate1bNativeDepthOrdinal", {
                configurable: false,
                enumerable: true,
                value: ordinal,
                writable: false,
            });
            screen.addEventListener("click", capture, {
                capture: true,
                once: true,
            });
            try {
                screen.dispatchEvent(event);
            } finally {
                screen.removeEventListener("click", capture, true);
            }
            return invoked;
        };
        const fullPump = Module.qtSendPendingApplicationEvents;
        if (
            typeof fullPump !== "function"
            || !stackCanaryIntact()
            || firstNativeDepthHandler === 0
        ) {
            return jspiWatchdogSentinel;
        }
        const fullPumpResult = await fullPump.call(Module);
        if (fullPumpResult !== false) {
            return jspiWatchdogSentinel;
        }
        HEAP32[fullPumpDeferred >> 2] = 1;
        const resumeWatchdogTimer = setTimeout(() => {
            const nestedLoopReturned = Array.from(report.events).some(
                (event) => (
                    event?.type === "jspi-after-exec"
                    && event?.payload?.postLoopSentinel === true
                    && event?.payload?.requestedNonce
                        === (requestedNonce >>> 0)
                    && event?.payload?.resolvedNonce
                        === (requestedNonce >>> 0)
                ),
            );
            const alreadyFailed = Array.isArray(report.snapshot?.failures)
                && report.snapshot.failures.length !== 0;
            if (nestedLoopReturned || alreadyFailed) {
                return;
            }
            try {
                report.rejectReady({
                    code: "jspi-resume-watchdog-timeout",
                    detail: {
                        requestedNonce: requestedNonce >>> 0,
                    },
                });
            } catch {
                // The first terminal record owns a concurrent failure.
            }
        }, 7500);

        let successTimer = 0;
        let sentinelTimer = 0;
        const screen = document.querySelector("#screen");
        let removeTrustedPointerListener = () => {};
        let importCanceled = false;
        const cancelOwnedImport = (terminal) => {
            if (terminal) {
                clearTimeout(resumeWatchdogTimer);
            }
            if (importCanceled) {
                return;
            }
            importCanceled = true;
            removeTrustedPointerListener();
            clearTimeout(successTimer);
            clearTimeout(sentinelTimer);
        };
        let settleTerminalCancellation = () => {};
        const terminalCancellation = new Promise((resolve) => {
            settleTerminalCancellation = () => {
                resolve(jspiWatchdogSentinel);
            };
        });
        void Promise.resolve(report.ready).catch(() => {
            cancelOwnedImport(true);
            settleTerminalCancellation();
        });
        const success = new Promise((resolve) => {
            if (!(screen instanceof HTMLElement)) {
                return;
            }
            const onTrustedPointerUp = (event) => {
                if (
                    importCanceled
                    || event.isTrusted !== true
                    || event.button !== 0
                ) {
                    return;
                }
                removeTrustedPointerListener();
                if (
                    !dispatchNativeDepthEvent(
                        firstNativeDepthHandler,
                        1,
                    )
                    || !stackCanaryIntact()
                ) {
                    HEAP32[
                        primaryStackCanaryObservedIntact >> 2
                    ] = 0;
                    resolve(jspiWatchdogSentinel);
                    return;
                }
                HEAP32[primaryStackCanaryObservedIntact >> 2] = 1;
                successTimer = setTimeout(
                    () => {
                        if (!importCanceled) {
                            resolve(requestedNonce >>> 0);
                        }
                    },
                    4,
                );
            };
            removeTrustedPointerListener = () => {
                screen.removeEventListener(
                    "pointerup",
                    onTrustedPointerUp,
                    true,
                );
                removeTrustedPointerListener = () => {};
            };
            screen.addEventListener(
                "pointerup",
                onTrustedPointerUp,
                { capture: true },
            );
        });
        const watchdog = new Promise((resolve) => {
            sentinelTimer = setTimeout(
                () => resolve(jspiWatchdogSentinel),
                5000,
            );
        });
        const result = await Promise.race([
            success,
            watchdog,
            terminalCancellation,
        ]);
        cancelOwnedImport(false);
        return result >>> 0;
    });

EM_JS(
    int,
    rgDispatchNativeDepthProbeEvent,
    (uint32_t handlerIndex, int ordinal),
    {
        const control = Module.qtSuspendResumeControl;
        const handler = control?.eventHandlers?.[handlerIndex >>> 0];
        const screen = document.getElementById("screen");
        if (
            typeof handler !== "function"
            || !(screen instanceof HTMLElement)
            || ordinal <= 0
        ) {
            return 0;
        }
        let invoked = false;
        const capture = (event) => {
            invoked = true;
            handler(event);
        };
        const event = new MouseEvent("click", {
            bubbles: true,
            cancelable: true,
            composed: true,
        });
        Object.defineProperty(event, "gate1bNativeDepthOrdinal", {
            configurable: false,
            enumerable: true,
            value: ordinal,
            writable: false,
        });
        screen.addEventListener("click", capture, {
            capture: true,
            once: true,
        });
        try {
            screen.dispatchEvent(event);
        } finally {
            screen.removeEventListener("click", capture, true);
        }
        return invoked ? 1 : 0;
    });

EM_JS(
    int,
    rgScheduleNativeDepthProbeEvent,
    (uint32_t handlerIndex, int ordinal),
    {
        const control = Module.qtSuspendResumeControl;
        const handler = control?.eventHandlers?.[handlerIndex >>> 0];
        const screen = document.getElementById("screen");
        if (
            typeof handler !== "function"
            || !(screen instanceof HTMLElement)
            || ordinal <= 0
        ) {
            return 0;
        }
        queueMicrotask(() => {
            let invoked = false;
            const capture = (event) => {
                invoked = true;
                handler(event);
            };
            const event = new MouseEvent("click", {
                bubbles: true,
                cancelable: true,
                composed: true,
            });
            Object.defineProperty(event, "gate1bNativeDepthOrdinal", {
                configurable: false,
                enumerable: true,
                value: ordinal,
                writable: false,
            });
            screen.addEventListener("click", capture, {
                capture: true,
                once: true,
            });
            try {
                screen.dispatchEvent(event);
            } finally {
                screen.removeEventListener("click", capture, true);
            }
            if (!invoked) {
                throw new Error(
                    "native-depth-probe-scheduled-dispatch-failed",
                );
            }
        });
        return 1;
    });

EM_JS(int, rgDispatchNativeDepthLimitAttemptEvent, (), {
    const screen = document.getElementById("screen");
    if (!(screen instanceof HTMLElement)) {
        return 0;
    }
    const event = new Event("rhythmgame-gate1b-native-depth-attempt", {
        bubbles: true,
        composed: true,
    });
    Object.defineProperty(event, "gate1bNativeDepthOrdinal", {
        configurable: false,
        enumerable: true,
        value: 5,
        writable: false,
    });
    screen.dispatchEvent(event);
    return 1;
});

EM_JS(
    int,
    rgVerifyNativeStackCanary,
    (const uint8_t *bytes, int length, int seed),
    {
        if (length <= 0 || seed < 0 || seed > 255) {
            return 0;
        }
        for (let index = 0; index < length; ++index) {
            const expected = (seed + (index * 37)) & 0xFF;
            if (HEAPU8[(bytes + index) >>> 0] !== expected) {
                return 0;
            }
        }
        return 1;
    });

EM_ASYNC_JS(int, rgAwaitUnmatchedNativeSuspension, (), {
    return await new Promise(() => {});
});

EMSCRIPTEN_BINDINGS(rhythm_game_gate1b_runtime)
{
    emscripten::function(
        "rhythmGameGate1bCommand",
        &rhythmGameGate1bCommand);
}
#endif

BrowserCapabilities browserCapabilities()
{
#ifdef __EMSCRIPTEN__
    const unsigned bits = rgGate1bBrowserCapabilityBits();
    return {
        .secureContext = (bits & secureContextCapability) != 0,
        .crossOriginIsolated =
            (bits & crossOriginIsolatedCapability) != 0,
        .sharedArrayBuffer =
            (bits & sharedArrayBufferCapability) != 0,
        .jspiApi = (bits & jspiApiCapability) != 0,
        .webGl2Api = (bits & webGl2ApiCapability) != 0,
        .audioWorklet = (bits & audioWorkletCapability) != 0,
        .opfs = (bits & opfsCapability) != 0,
        .fileSystemAccess =
            (bits & fileSystemAccessCapability) != 0,
    };
#else
    return {};
#endif
}

QStringView browserRuntimeCommandName(BrowserRuntimeCommand command)
{
    for (const RuntimeCommandDefinition &definition :
         runtimeCommandDefinitions) {
        if (definition.command == command) {
            return definition.name;
        }
    }
    rejectCommand("runtime-command-enum-invalid");
}

bool publishGate1bEvent(const QJsonObject &event)
{
#ifdef __EMSCRIPTEN__
    const QByteArray json =
        QJsonDocument{event}.toJson(QJsonDocument::Compact);
    const int expectedSequence = event.value(u"sequence").toInt(-1);
    if (rgPublishGate1bEvent(
            json.constData(),
            expectedSequence) != 1) {
        return false;
    }
#else
    Q_UNUSED(event);
#endif
    return true;
}

bool publishGate1bSnapshot(const QJsonObject &snapshot)
{
#ifdef __EMSCRIPTEN__
    const QByteArray json =
        QJsonDocument{snapshot}.toJson(QJsonDocument::Compact);
    if (rgPublishGate1bSnapshot(json.constData()) != 1) {
        return false;
    }
#else
    Q_UNUSED(snapshot);
#endif
    return true;
}

bool resolveGate1bReady(const QJsonObject &snapshot)
{
#ifdef __EMSCRIPTEN__
    const QByteArray json =
        QJsonDocument{snapshot}.toJson(QJsonDocument::Compact);
    if (rgResolveGate1bReady(json.constData()) != 1) {
        return false;
    }
#else
    Q_UNUSED(snapshot);
#endif
    return true;
}

void rejectGate1bReady(QStringView code, const QJsonObject &detail)
{
#ifdef __EMSCRIPTEN__
    const QJsonObject payload{
        {QStringLiteral("code"), code.toString()},
        {QStringLiteral("detail"), detail},
    };
    const QByteArray json =
        QJsonDocument{payload}.toJson(QJsonDocument::Compact);
    rgRejectGate1bReady(json.constData());
#else
    Q_UNUSED(code);
    Q_UNUSED(detail);
#endif
}

QUrl BrowserRuntimeBridge::sameOriginUrl(QStringView pathAndQuery)
{
    const QString encoded = pathAndQuery.toString();
    QUrl relative(encoded, QUrl::StrictMode);
    if (!relative.isValid()
        || !relative.isRelative()
        || !relative.path().startsWith(u'/')
        || relative.hasFragment()
        || encoded.contains(u'\\')
        || encoded.contains(u"//")) {
        throw std::runtime_error{"same-origin-relative-url-invalid"};
    }
#ifdef __EMSCRIPTEN__
    const QString origin = QString::fromStdString(
        emscripten::val::global("location")["origin"].as<std::string>());
    const QUrl base{origin + QStringLiteral("/"), QUrl::StrictMode};
    QUrl resolved = base.resolved(relative);
    if (!base.isValid()
        || base.scheme() != u"https"
        || resolved.scheme() != base.scheme()
        || resolved.host() != base.host()
        || resolved.port() != base.port()
        || resolved.userInfo().isEmpty() == false) {
        throw std::runtime_error{"same-origin-url-mismatch"};
    }
    return resolved;
#else
    return relative;
#endif
}

bool BrowserRuntimeBridge::beginOwnedMediaBackendTracking(
    quint32 runNonce)
{
#ifdef __EMSCRIPTEN__
    return runNonce != 0
        && runNonce != jspiWatchdogSentinel()
        && rgBeginOwnedMediaBackendTracking(runNonce) == 1;
#else
    Q_UNUSED(runNonce);
    return false;
#endif
}

QJsonObject BrowserRuntimeBridge::armOwnedMediaSeekTracking(
    quint32 runNonce,
    qint64 targetPositionMilliseconds)
{
#ifdef __EMSCRIPTEN__
    if (targetPositionMilliseconds <= 0
        || targetPositionMilliseconds > 30000) {
        return {};
    }
    return mediaTrackerCommandResult(
        [runNonce, targetPositionMilliseconds](
            char *output,
            int capacity) {
            return rgArmOwnedMediaSeekTracking(
                runNonce,
                static_cast<int>(targetPositionMilliseconds),
                output,
                capacity);
        });
#else
    Q_UNUSED(runNonce);
    Q_UNUSED(targetPositionMilliseconds);
    return {};
#endif
}

QJsonObject BrowserRuntimeBridge::finishOwnedMediaSeekTracking(
    quint32 runNonce)
{
#ifdef __EMSCRIPTEN__
    return mediaTrackerCommandResult(
        [runNonce](char *output, int capacity) {
            return rgFinishOwnedMediaSeekTracking(
                runNonce,
                output,
                capacity);
        });
#else
    Q_UNUSED(runNonce);
    return {};
#endif
}

QJsonObject BrowserRuntimeBridge::armOwnedMediaBackendRemoval(
    quint32 runNonce)
{
#ifdef __EMSCRIPTEN__
    return mediaTrackerCommandResult(
        [runNonce](char *output, int capacity) {
            return rgArmOwnedMediaBackendRemoval(
                runNonce,
                output,
                capacity);
        });
#else
    Q_UNUSED(runNonce);
    return {};
#endif
}

QJsonObject BrowserRuntimeBridge::finishOwnedMediaBackendRemoval(
    quint32 runNonce,
    bool abandon)
{
#ifdef __EMSCRIPTEN__
    return mediaTrackerCommandResult(
        [runNonce, abandon](char *output, int capacity) {
            return rgFinishOwnedMediaBackendRemoval(
                runNonce,
                abandon ? 1 : 0,
                output,
                capacity);
        });
#else
    Q_UNUSED(runNonce);
    Q_UNUSED(abandon);
    return {};
#endif
}

QJsonObject BrowserRuntimeBridge::releaseOwnedMediaBackendRemoval(
    quint32 runNonce)
{
#ifdef __EMSCRIPTEN__
    return mediaTrackerCommandResult(
        [runNonce](char *output, int capacity) {
            return rgReleaseOwnedMediaBackendRemoval(
                runNonce,
                output,
                capacity);
        });
#else
    Q_UNUSED(runNonce);
    return {};
#endif
}

bool browserUserActivationIsActive()
{
#ifdef __EMSCRIPTEN__
    return rgBrowserUserActivationIsActive() != 0;
#else
    return false;
#endif
}

quint32 generateOwnedBrowserNonce()
{
#ifdef __EMSCRIPTEN__
    return rgGenerateOwnedBrowserNonce();
#else
    return 1U;
#endif
}

quint32 awaitOwnedBrowserNonce(
    quint32 requestedNonce,
    quint32 firstNativeDepthHandler,
    const std::uint8_t *primaryStackCanary,
    int primaryStackCanaryLength,
    int primaryStackCanarySeed,
    bool &fullPumpDeferred,
    bool &primaryStackCanaryObservedIntact)
{
#ifdef __EMSCRIPTEN__
    int fullPumpDeferredValue = 0;
    int primaryStackCanaryObservedIntactValue = 0;
    const quint32 resolved = rgGate1bAwaitOwnedNonce(
        requestedNonce,
        firstNativeDepthHandler,
        primaryStackCanary,
        primaryStackCanaryLength,
        primaryStackCanarySeed,
        &fullPumpDeferredValue,
        &primaryStackCanaryObservedIntactValue);
    fullPumpDeferred = fullPumpDeferredValue == 1;
    primaryStackCanaryObservedIntact =
        primaryStackCanaryObservedIntactValue == 1;
    return resolved;
#else
    Q_UNUSED(firstNativeDepthHandler);
    Q_UNUSED(primaryStackCanary);
    Q_UNUSED(primaryStackCanaryLength);
    Q_UNUSED(primaryStackCanarySeed);
    fullPumpDeferred = true;
    primaryStackCanaryObservedIntact = true;
    return requestedNonce;
#endif
}

bool dispatchNativeDepthProbeEvent(quint32 handler, int ordinal)
{
#ifdef __EMSCRIPTEN__
    return rgDispatchNativeDepthProbeEvent(handler, ordinal) == 1;
#else
    Q_UNUSED(handler);
    Q_UNUSED(ordinal);
    return false;
#endif
}

bool scheduleNativeDepthProbeEvent(quint32 handler, int ordinal)
{
#ifdef __EMSCRIPTEN__
    return rgScheduleNativeDepthProbeEvent(handler, ordinal) == 1;
#else
    Q_UNUSED(handler);
    Q_UNUSED(ordinal);
    return false;
#endif
}

bool dispatchNativeDepthLimitAttemptEvent()
{
#ifdef __EMSCRIPTEN__
    return rgDispatchNativeDepthLimitAttemptEvent() == 1;
#else
    return false;
#endif
}

bool verifyNativeStackCanary(
    const std::uint8_t *bytes,
    int length,
    int seed)
{
#ifdef __EMSCRIPTEN__
    return rgVerifyNativeStackCanary(bytes, length, seed) == 1;
#else
    Q_UNUSED(bytes);
    Q_UNUSED(length);
    Q_UNUSED(seed);
    return true;
#endif
}

int awaitUnmatchedNativeSuspension()
{
#ifdef __EMSCRIPTEN__
    return rgAwaitUnmatchedNativeSuspension();
#else
    return 1;
#endif
}

bool scheduleExclusiveSuspendGuardProbe(
    quint32 firstHandler,
    quint32 secondHandler,
    quint32 exclusiveHandler,
    quint32 completionHandler)
{
#ifdef __EMSCRIPTEN__
    return rgScheduleExclusiveSuspendGuardProbe(
        firstHandler,
        secondHandler,
        exclusiveHandler,
        completionHandler) == 1;
#else
    Q_UNUSED(firstHandler);
    Q_UNUSED(secondHandler);
    Q_UNUSED(exclusiveHandler);
    Q_UNUSED(completionHandler);
    return false;
#endif
}

bool armExclusiveSuspendGuardNormalDrain()
{
#ifdef __EMSCRIPTEN__
    return rgArmExclusiveSuspendGuardNormalDrain() == 1;
#else
    return false;
#endif
}

QJsonObject takeExclusiveSuspendGuardProbeResult()
{
#ifdef __EMSCRIPTEN__
    QByteArray encoded(4096, '\0');
    const int byteLength = rgTakeExclusiveSuspendGuardProbeResult(
        encoded.data(),
        encoded.size());
    if (byteLength < 0 || byteLength >= encoded.size()) {
        return {};
    }
    encoded.resize(byteLength);
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(encoded, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()
        || document.toJson(QJsonDocument::Compact) != encoded) {
        return {};
    }
    return document.object();
#else
    return {};
#endif
}

void cancelExclusiveSuspendGuardProbe()
{
#ifdef __EMSCRIPTEN__
    rgCancelExclusiveSuspendGuardProbe();
#endif
}

void installBrowserRuntimeCommandHandler(
    QObject *owner,
    BrowserRuntimeCommandHandler handler)
{
    if (owner == nullptr || QThread::currentThread() != owner->thread()) {
        rejectCommand("runtime-command-owner-invalid");
    }
    commandOwner = owner;
    commandHandler = std::move(handler);
    const std::uint64_t installationGeneration =
        ++commandHandlerGeneration;
    QObject::connect(
        owner,
        &QObject::destroyed,
        [installationGeneration] {
            if (commandHandlerGeneration == installationGeneration) {
                commandOwner.clear();
                commandHandler = {};
            }
        });
}
