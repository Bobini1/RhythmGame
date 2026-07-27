const mainJsRole = "mainJs";
const qtloaderRole = "qtloader";
const executableRoles = Object.freeze([
    "audioWorklet",
    "bootstrap",
    mainJsRole,
    "preflightWorker",
    qtloaderRole,
    "wasm",
    "wasmWorker",
]);
const expectedRoles = Object.freeze([
    ...executableRoles,
    "css",
    "html",
    "media",
].sort());
const expectedPolicy = Object.freeze({
    "cross-origin-embedder-policy": "require-corp",
    "cross-origin-opener-policy": "same-origin",
    "cross-origin-resource-policy": "same-origin",
    "permissions-policy":
        "fullscreen=(self), gamepad=(self), hid=(self), unload=()",
    "referrer-policy": "no-referrer",
    "x-content-type-options": "nosniff",
});
const expectedMimes = Object.freeze({
    audioWorklet: "text/javascript; charset=utf-8",
    bootstrap: "text/javascript; charset=utf-8",
    css: "text/css; charset=utf-8",
    html: "text/html; charset=utf-8",
    mainJs: "text/javascript; charset=utf-8",
    media: "video/webm",
    preflightWorker: "text/javascript; charset=utf-8",
    qtloader: "text/javascript; charset=utf-8",
    wasm: "application/wasm",
    wasmWorker: "text/javascript; charset=utf-8",
});
const artifactNames = Object.freeze({
    audioWorklet: ["RhythmGameWasmProbe.aw", "js"],
    bootstrap: ["bootstrap", "mjs"],
    css: ["probe", "css"],
    mainJs: ["RhythmGameWasmProbe", "js"],
    media: ["probe", "webm"],
    preflightWorker: ["preflight-worker", "mjs"],
    qtloader: ["qtloader", "js"],
    wasm: ["RhythmGameWasmProbe", "wasm"],
    wasmWorker: ["RhythmGameWasmProbe.ww", "js"],
});
const digestPattern = /^[0-9a-f]{64}$/;
const sriPattern = /^sha256-[A-Za-z0-9+/]{43}=$/;
const contentAddressPattern = /^[A-Za-z0-9][A-Za-z0-9._-]*$/;
const qtShadowHostId = "qt-shadow-container";
const qtStyleByteLength = 5238;
const qtStyleSha256 = "6b7168686da79590ea116889998716dfa624e1467411daa2bffee066a867d53e";
const qtStyleRuleCount = 37;
const qtStyleKeySelectors = Object.freeze([".qt-screen", ".qt-window"]);
const gate1bEventKeys = Object.freeze([
    "monotonicMicroseconds",
    "payload",
    "sequence",
    "type",
]);
const gate1bSnapshotKeys = Object.freeze([
    "authority",
    "capabilities",
    "checks",
    "cycleSummary",
    "failures",
    "phase",
]);
const gate1bAuthorityKeys = Object.freeze([
    "formalGate1EntryAuthorized",
    "gate0Satisfied",
    "gate1Passed",
    "gate1bTechnicalPassed",
    "productionPortAuthorized",
]);
const gate1bCapabilityKeys = Object.freeze([
    "audioWorklet",
    "crossOriginIsolated",
    "fileSystemAccess",
    "jspiApi",
    "opfs",
    "secureContext",
    "sharedArrayBuffer",
    "webGl2Api",
]);
const gate1bCheckKeys = Object.freeze(["detail", "passed"]);
const gate1bCycleSummaryKeys = Object.freeze(["completed", "status"]);
const gate1bFailureKeys = Object.freeze(["code", "detail"]);
const gate1bRejectionKeys = Object.freeze(["code", "detail"]);
const gate1bTypePattern = /^[a-z][a-z0-9]*(?:-[a-z0-9]+)*$/;
const ordinaryRuntimeCommandNames = Object.freeze([
    "ack-media-frame-capture",
    "arm-bfcache-resume-probe",
    "arm-hidden-timer-probe",
    "arm-visible-resume-timer-probe",
    "begin-foreground-latency-sampling",
    "probe-ping",
    "set-shader-phase",
]);
const adversarialRuntimeCommandModes = Object.freeze({
    "trigger-native-depth-limit": "native-depth-limit",
    "trigger-native-suspension-trap": "native-suspension-trap",
});
const runtimeCommandNames = Object.freeze([
    ...ordinaryRuntimeCommandNames,
    ...Object.keys(adversarialRuntimeCommandModes),
]);
const maxSynchronousNativePumpDepth = 4;
const foregroundInputSampleTarget = 64;
const hiddenIdleFallbackMilliseconds = 50;
const bfcacheExpectationStorageKey =
    "rhythmgame.gate1b.bfcache-expectation";
const nativeDepthLimitAttemptEventType =
    "rhythmgame-gate1b-native-depth-attempt";
const qtScreenNativeDispatchEventTypes = Object.freeze([
    "beforeinput",
    "click",
    "compositionend",
    "compositionstart",
    "compositionupdate",
    "contextmenu",
    "dblclick",
    "dragend",
    "dragenter",
    "dragleave",
    "dragover",
    "dragstart",
    "drop",
    "input",
    "keydown",
    "keyup",
    "pointercancel",
    "pointerdown",
    "pointermove",
    "pointerup",
    "wheel",
]);
const qtDocumentNativeDispatchEventTypes = Object.freeze([
    "copy",
    "cut",
    "paste",
]);
const qtWindowNativeDispatchEventTypes = Object.freeze([
    "pointerenter",
    "pointerleave",
]);
const routeParts = location.pathname.split("/");
const negativeMode = routeParts[1] === "negative" ? routeParts[2] : null;

let resolveReady;
let rejectReady;
let readySettled = false;
let runtimeCommandIngress = null;
let retainedRuntimeCommand = null;
let runtimeCommandInFlight = false;
let publishedGate1bSnapshot = null;
let readyResolutionRecord = null;
let styleAdoptionRecord = null;
let eventPumpTelemetryReader = null;
let applicationCycleEventObserver = null;
let bfcacheEventObserver = null;
let foregroundInputEventObserver = null;
let foregroundTimerEventObserver = null;
let hiddenTimerEventObserver = null;
const gate1bEventLog = [];
const gate1bEventView = new Proxy(gate1bEventLog, {
    defineProperty() {
        return false;
    },
    deleteProperty() {
        return false;
    },
    preventExtensions() {
        return false;
    },
    set() {
        return false;
    },
    setPrototypeOf() {
        return false;
    },
});
const ready = new Promise((resolve, reject) => {
    resolveReady = resolve;
    rejectReady = reject;
});
const report = {};
Object.defineProperties(report, {
    appendEvent: {
        configurable: false,
        enumerable: false,
        value: appendGate1bEvent,
        writable: false,
    },
    command: {
        configurable: false,
        enumerable: true,
        value: commandGate1bRuntime,
        writable: false,
    },
    eventPump: {
        configurable: false,
        enumerable: true,
        get() {
            return eventPumpTelemetryReader?.() ?? null;
        },
    },
    events: {
        configurable: false,
        enumerable: true,
        get() {
            return gate1bEventView;
        },
    },
    publishSnapshot: {
        configurable: false,
        enumerable: false,
        value: publishGate1bSnapshot,
        writable: false,
    },
    rejectReady: {
        configurable: false,
        enumerable: false,
        value: rejectGate1bReady,
        writable: false,
    },
    ready: {
        configurable: false,
        enumerable: true,
        value: ready,
        writable: false,
    },
    readyResolution: {
        configurable: false,
        enumerable: true,
        get() {
            return readyResolutionRecord;
        },
    },
    resolveReady: {
        configurable: false,
        enumerable: false,
        value: resolveGate1bReady,
        writable: false,
    },
    schemaVersion: {
        configurable: false,
        enumerable: true,
        value: 1,
        writable: false,
    },
    snapshot: {
        configurable: false,
        enumerable: true,
        get() {
            return publishedGate1bSnapshot;
        },
    },
    styleAdoption: {
        configurable: false,
        enumerable: true,
        get() {
            return styleAdoptionRecord;
        },
    },
});
Object.freeze(report);
Object.defineProperty(globalThis, "__rhythmGameGate1b", {
    configurable: false,
    enumerable: false,
    value: report,
    writable: false,
});
if (globalThis.__rhythmGameGate1b !== report) {
    throw new Error("gate1b-report-publication-failed");
}

function createMediaBackendTracker() {
    let active = null;
    let nextElementId = 0;

    const restoreCreateElement = (state) => {
        if (state.restored) {
            return;
        }
        state.restored = true;
        if (state.ownDescriptor === undefined) {
            delete document.createElement;
        } else {
            Object.defineProperty(
                document,
                "createElement",
                state.ownDescriptor,
            );
        }
    };

    const expectedMediaSource = (runNonce) => new URL(
        `/fixtures/probe.webm?nonce=${runNonce}`,
        location.href,
    );

    const matchingOwnedElements = (state) => {
        const expectedSource = expectedMediaSource(state.runNonce);
        const matchingElements = state.elements.filter(
            (record) => (
                record.tagName === "video"
                && record.element.src === expectedSource.href
                && record.element.currentSrc === expectedSource.href
            ),
        );
        return { expectedSource, matchingElements };
    };

    const bindOwnedRecord = (state) => {
        const { expectedSource, matchingElements } =
            matchingOwnedElements(state);
        if (matchingElements.length !== 1) {
            return null;
        }
        if (
            state.ownedRecord !== null
            && state.ownedRecord !== matchingElements[0]
        ) {
            return null;
        }
        state.expectedSource = expectedSource;
        state.ownedRecord = matchingElements[0];
        return state.ownedRecord;
    };

    const removeSeekListeners = (state) => {
        if (state.seek === null || state.seek.listenersRemoved) {
            return;
        }
        state.seek.listenersRemoved = true;
        state.seek.element.removeEventListener(
            "seeking",
            state.seek.onSeeking,
        );
        state.seek.element.removeEventListener(
            "seeked",
            state.seek.onSeeked,
        );
    };

    const removeRemovalListeners = (state) => {
        if (
            state.removalRecord === null
            || state.removalRecord.listenersRemoved
        ) {
            return;
        }
        state.removalRecord.listenersRemoved = true;
        state.ownedRecord.element.removeEventListener(
            "emptied",
            state.removalRecord.onEmptied,
        );
    };

    const mediaRemovalArmSnapshot = (state) => {
        if (
            state.removalRecord === null
            || state.ownedRecord === null
            || state.expectedSource === null
        ) {
            return null;
        }
        return {
            cleanupArmed: true,
            elementId: state.ownedRecord.elementId,
            exactSourceMatched: true,
            hadResourceBeforeDestruction:
                state.removalRecord.hadResourceBeforeDestruction,
            matchingElementCount: 1,
            observerMutatedElement: false,
            runNonce: state.runNonce,
            sourcePathAndQuery:
                state.expectedSource.pathname
                + state.expectedSource.search,
            trackedMediaElementCount: state.elements.length,
            wasConnected: state.removalRecord.wasConnected,
        };
    };

    const mediaRemovalSnapshot = (state) => {
        if (
            state.removalRecord === null
            || state.ownedRecord === null
            || state.expectedSource === null
        ) {
            return null;
        }
        const record = state.ownedRecord;
        const element = record.element;
        const currentSourceCleared = element.currentSrc.length === 0;
        const currentSourceOwnedOrEmpty = (
            currentSourceCleared
            || element.currentSrc === state.expectedSource.href
        );
        const domRemoved = record.element.isConnected === false;
        const mediaErrorCleared = element.error === null;
        const networkStateEmpty = (
            element.networkState === HTMLMediaElement.NETWORK_EMPTY
        );
        const paused = element.paused === true;
        const readyStateEmpty = (
            element.readyState === HTMLMediaElement.HAVE_NOTHING
        );
        const sourceAttributeCleared =
            element.getAttribute("src") === null;
        const sourceElementCount =
            element.querySelectorAll("source").length;
        const sourceObjectCleared = element.srcObject === null;
        const sourcePropertyCleared = element.src.length === 0;
        const currentTimeReset = element.currentTime === 0;
        const documentOwnedElementCount = [
            ...document.querySelectorAll("audio, video"),
        ].filter(
            (candidate) => (
                candidate.src === state.expectedSource.href
                || candidate.currentSrc === state.expectedSource.href
            ),
        ).length;
        const durationCleared = Number.isNaN(element.duration);
        const emptiedObserved =
            state.removalRecord.emptiedObserved;
        const seekingStopped = element.seeking === false;
        const mediaElementResourceReleased = (
            currentSourceOwnedOrEmpty
            && currentTimeReset
            && documentOwnedElementCount === 0
            && domRemoved
            && durationCleared
            && emptiedObserved
            && state.removalRecord.hadResourceBeforeDestruction
            && mediaErrorCleared
            && networkStateEmpty
            && paused
            && readyStateEmpty
            && seekingStopped
            && sourceAttributeCleared
            && sourceElementCount === 0
            && sourceObjectCleared
            && sourcePropertyCleared
        );
        return {
            cleanupArmed: true,
            currentSourceCleared,
            currentSourceOwnedOrEmpty,
            currentTimeReset,
            documentOwnedElementCount,
            domRemoved,
            durationCleared,
            elementId: record.elementId,
            emptiedObserved,
            exactSourceMatched: true,
            hadResourceBeforeDestruction:
                state.removalRecord.hadResourceBeforeDestruction,
            matchingElementCount: 1,
            mediaElementResourceReleased,
            mediaErrorCleared,
            networkStateEmpty,
            observerMutatedElement: false,
            paused,
            readyStateEmpty,
            runNonce: state.runNonce,
            seekingStopped,
            sourceAttributeCleared,
            sourceElementCount,
            sourceObjectCleared,
            sourcePathAndQuery:
                state.expectedSource.pathname
                + state.expectedSource.search,
            sourcePropertyCleared,
            trackedMediaElementCount: state.elements.length,
            wasConnected: state.removalRecord.wasConnected,
        };
    };

    const begin = (runNonce) => {
        if (
            active !== null
            || !Number.isSafeInteger(runNonce)
            || runNonce <= 0
            || runNonce > 0xFFFFFFFE
        ) {
            return false;
        }
        const ownDescriptor = Object.getOwnPropertyDescriptor(
            document,
            "createElement",
        );
        const original = document.createElement;
        if (typeof original !== "function") {
            return false;
        }
        const state = {
            elements: [],
            original,
            ownDescriptor,
            ownedRecord: null,
            restored: false,
            runNonce,
            removalRecord: null,
            expectedSource: null,
            seek: null,
        };
        const trackedCreateElement = function (...arguments_) {
            const element = Reflect.apply(
                state.original,
                this,
                arguments_,
            );
            if (element instanceof HTMLMediaElement) {
                state.elements.push({
                    element,
                    elementId: `media-element-${++nextElementId}`,
                    tagName: element.tagName.toLowerCase(),
                });
            }
            return element;
        };
        Object.defineProperty(document, "createElement", {
            configurable: true,
            enumerable: false,
            value: trackedCreateElement,
            writable: true,
        });
        active = state;
        return true;
    };

    const armSeek = (runNonce, targetPositionMilliseconds) => {
        if (
            active === null
            || active.runNonce !== runNonce
            || active.seek !== null
            || !Number.isSafeInteger(targetPositionMilliseconds)
            || targetPositionMilliseconds <= 0
            || targetPositionMilliseconds > 30_000
        ) {
            return null;
        }
        const state = active;
        const record = bindOwnedRecord(state);
        if (record === null) {
            return null;
        }
        const element = record.element;
        const preSeekPositionMilliseconds = element.currentTime * 1000;
        const requestMonotonicMilliseconds = performance.now();
        if (
            !Number.isFinite(preSeekPositionMilliseconds)
            || !Number.isFinite(requestMonotonicMilliseconds)
        ) {
            return null;
        }
        const seek = {
            element,
            listenersRemoved: false,
            onSeeked: null,
            onSeeking: null,
            preSeekPositionMilliseconds,
            requestMonotonicMilliseconds,
            seekedMonotonicMilliseconds: null,
            seekedPositionMilliseconds: null,
            seekingMonotonicMilliseconds: null,
            seekingPositionMilliseconds: null,
            targetPositionMilliseconds,
        };
        seek.onSeeking = () => {
            if (
                active !== state
                || state.seek !== seek
                || seek.seekingMonotonicMilliseconds !== null
            ) {
                return;
            }
            seek.seekingMonotonicMilliseconds = performance.now();
            seek.seekingPositionMilliseconds =
                element.currentTime * 1000;
        };
        seek.onSeeked = () => {
            if (
                active !== state
                || state.seek !== seek
                || seek.seekedMonotonicMilliseconds !== null
            ) {
                return;
            }
            seek.seekedMonotonicMilliseconds = performance.now();
            seek.seekedPositionMilliseconds =
                element.currentTime * 1000;
            removeSeekListeners(state);
        };
        state.seek = seek;
        element.addEventListener("seeking", seek.onSeeking);
        element.addEventListener("seeked", seek.onSeeked);
        return {
            elementId: record.elementId,
            preSeekPositionMilliseconds,
            requestMonotonicMilliseconds,
            runNonce,
            targetPositionMilliseconds,
        };
    };

    const finishSeek = (runNonce) => {
        if (
            active === null
            || active.runNonce !== runNonce
            || active.seek === null
            || active.seek.seekingMonotonicMilliseconds === null
            || active.seek.seekingPositionMilliseconds === null
            || active.seek.seekedMonotonicMilliseconds === null
            || active.seek.seekedPositionMilliseconds === null
        ) {
            return null;
        }
        const state = active;
        const seek = state.seek;
        return {
            elementId: state.ownedRecord.elementId,
            preSeekPositionMilliseconds:
                seek.preSeekPositionMilliseconds,
            requestMonotonicMilliseconds:
                seek.requestMonotonicMilliseconds,
            runNonce,
            seekedMonotonicMilliseconds:
                seek.seekedMonotonicMilliseconds,
            seekedPositionMilliseconds:
                seek.seekedPositionMilliseconds,
            seekingMonotonicMilliseconds:
                seek.seekingMonotonicMilliseconds,
            seekingPositionMilliseconds:
                seek.seekingPositionMilliseconds,
            targetPositionMilliseconds:
                seek.targetPositionMilliseconds,
        };
    };

    const armRemoval = (runNonce) => {
        if (active === null || active.runNonce !== runNonce) {
            return null;
        }
        const state = active;
        if (state.removalRecord !== null) {
            return null;
        }
        const record = bindOwnedRecord(state);
        const { matchingElements } = matchingOwnedElements(state);
        if (
            record === null
            || matchingElements.length !== 1
            || matchingElements[0] !== record
        ) {
            return null;
        }
        restoreCreateElement(state);
        removeSeekListeners(state);
        const element = record.element;
        const removalRecord = {
            emptiedObserved: false,
            hadResourceBeforeDestruction: (
                element.networkState !== HTMLMediaElement.NETWORK_EMPTY
                || element.readyState !== HTMLMediaElement.HAVE_NOTHING
            ),
            listenersRemoved: false,
            onEmptied: null,
            wasConnected: element.isConnected === true,
        };
        removalRecord.onEmptied = () => {
            if (
                active === state
                && state.removalRecord === removalRecord
            ) {
                removalRecord.emptiedObserved = true;
            }
        };
        state.removalRecord = removalRecord;
        element.addEventListener("emptied", removalRecord.onEmptied, {
            once: true,
        });
        return mediaRemovalArmSnapshot(state);
    };

    const finish = (runNonce, abandon = false) => {
        if (active === null || active.runNonce !== runNonce) {
            return null;
        }
        const state = active;
        restoreCreateElement(state);
        removeSeekListeners(state);
        const snapshot = mediaRemovalSnapshot(state);
        if (abandon === true) {
            removeRemovalListeners(state);
            active = null;
        }
        return snapshot;
    };

    const release = (runNonce) => {
        if (active === null || active.runNonce !== runNonce) {
            return null;
        }
        const state = active;
        const snapshot = mediaRemovalSnapshot(state);
        if (snapshot?.mediaElementResourceReleased !== true) {
            return null;
        }
        restoreCreateElement(state);
        removeSeekListeners(state);
        removeRemovalListeners(state);
        active = null;
        return snapshot;
    };

    return Object.freeze({
        armRemoval,
        armSeek,
        begin,
        finish,
        finishSeek,
        release,
    });
}

const mediaBackendTracker = createMediaBackendTracker();
Object.defineProperty(
    globalThis,
    "__rhythmGameGate1bMediaBackendTracker",
    {
        configurable: false,
        enumerable: false,
        value: mediaBackendTracker,
        writable: false,
    },
);

let terminalFailure = null;
let activeWorkerCspReject = null;
let qtEventPump = null;

function commandGate1bRuntime(name, payload) {
    if (runtimeCommandIngress === null) {
        throw new Error("gate1b-runtime-not-ready");
    }
    return runtimeCommandIngress(name, payload);
}

function retainRuntimeCommandAuthority(instance) {
    if (retainedRuntimeCommand !== null) {
        fail("runtime-command-authority-duplicate");
    }
    const descriptor = Object.getOwnPropertyDescriptor(
        instance,
        "rhythmGameGate1bCommand",
    );
    if (typeof descriptor?.value !== "function") {
        fail("runtime-command-entry-missing");
    }
    const boundCommand = descriptor.value.bind(instance);
    const eventCountBefore = report.events.length;
    const snapshotBefore = JSON.stringify(report.snapshot);
    const mismatchedDestructiveCommand = (
        negativeMode === "native-depth-limit"
            ? "trigger-native-suspension-trap"
            : "trigger-native-depth-limit"
    );
    const boundaryCases = [
        {
            expected: "runtime-command-name-too-large",
            name: "x".repeat(65),
            payload: "{}",
        },
        {
            expected: "runtime-command-payload-too-large",
            name: "probe-ping",
            payload: " ".repeat(4097),
        },
        {
            expected: "ack-media-frame-capture-request-id",
            name: "ack-media-frame-capture",
            payload: JSON.stringify({
                requestIds: [`request-${"1".repeat(80)}`],
                runNonce: 1,
            }),
        },
        {
            expected: "runtime-command-native-route",
            name: mismatchedDestructiveCommand,
            payload: "{}",
        },
    ];
    for (const boundaryCase of boundaryCases) {
        let result;
        try {
            result = parseRuntimeCommandResult(
                boundCommand(boundaryCase.name, boundaryCase.payload),
            );
        } catch (error) {
            fail("runtime-command-boundary-audit-threw", {
                message: String(error),
                name: boundaryCase.name,
            });
        }
        if (
            result.ok !== false
            || result.error !== boundaryCase.expected
        ) {
            fail("runtime-command-boundary-audit-mismatch", {
                actual: result,
                expected: boundaryCase.expected,
                name: boundaryCase.name,
            });
        }
    }
    if (
        report.events.length !== eventCountBefore
        || JSON.stringify(report.snapshot) !== snapshotBefore
    ) {
        fail("runtime-command-boundary-audit-mutated-report");
    }
    if (
        !Reflect.deleteProperty(instance, "rhythmGameGate1bCommand")
        || "rhythmGameGate1bCommand" in instance
    ) {
        fail("runtime-command-entry-scrub-failed");
    }
    retainedRuntimeCommand = boundCommand;
}

function isPlainObject(value) {
    if (
        value === null
        || typeof value !== "object"
        || Array.isArray(value)
    ) {
        return false;
    }
    const prototype = Object.getPrototypeOf(value);
    return prototype === Object.prototype || prototype === null;
}

function requireExactKeys(value, expected, description) {
    if (!isPlainObject(value)) {
        throw new TypeError(`${description}-not-object`);
    }
    const actual = Object.keys(value).sort();
    if (
        actual.length !== expected.length
        || actual.some((key, index) => key !== expected[index])
    ) {
        throw new TypeError(`${description}-keys`);
    }
}

function parseRuntimeCommandResult(encodedResult) {
    if (typeof encodedResult !== "string") {
        throw new TypeError("runtime-command-result-not-string");
    }
    const result = JSON.parse(encodedResult);
    if (!isPlainObject(result)) {
        throw new TypeError("runtime-command-result-not-object");
    }
    if (result.ok === true) {
        requireExactKeys(
            result,
            ["ok", "reply"],
            "runtime-command-success-result",
        );
        if (!isPlainObject(result.reply)) {
            throw new TypeError("runtime-command-reply-not-object");
        }
        return result;
    }
    if (result.ok === false) {
        requireExactKeys(
            result,
            ["error", "ok"],
            "runtime-command-failure-result",
        );
        if (
            typeof result.error !== "string"
            || !/^[a-z][a-z0-9-]{0,127}$/.test(result.error)
        ) {
            throw new TypeError("runtime-command-error-code-invalid");
        }
        return result;
    }
    throw new TypeError("runtime-command-result-ok-invalid");
}

function freezeJsonValue(value) {
    if (Array.isArray(value)) {
        for (const item of value) {
            freezeJsonValue(item);
        }
        return Object.freeze(value);
    }
    if (isPlainObject(value)) {
        for (const item of Object.values(value)) {
            freezeJsonValue(item);
        }
        return Object.freeze(value);
    }
    return value;
}

function cloneAndFreezeJson(value) {
    const encoded = JSON.stringify(value);
    if (encoded === undefined) {
        throw new TypeError("gate1b-json-value");
    }
    return freezeJsonValue(JSON.parse(encoded));
}

function normalizeFailureDetail(detail) {
    try {
        const normalized = cloneAndFreezeJson(detail);
        if (isPlainObject(normalized)) {
            return normalized;
        }
        return freezeJsonValue({ value: normalized });
    } catch {
        return Object.freeze({ message: "unserializable-failure-detail" });
    }
}

function validateGate1bEvent(event) {
    requireExactKeys(event, gate1bEventKeys, "gate1b-event");
    if (
        !Number.isSafeInteger(event.sequence)
        || event.sequence !== gate1bEventLog.length
    ) {
        throw new TypeError("gate1b-event-sequence");
    }
    if (
        !Number.isSafeInteger(event.monotonicMicroseconds)
        || event.monotonicMicroseconds < 0
    ) {
        throw new TypeError("gate1b-event-monotonic-time");
    }
    const previous = gate1bEventLog.at(-1)?.monotonicMicroseconds ?? -1;
    if (event.monotonicMicroseconds <= previous) {
        throw new TypeError("gate1b-event-monotonic-order");
    }
    if (
        typeof event.type !== "string"
        || !gate1bTypePattern.test(event.type)
    ) {
        throw new TypeError("gate1b-event-type");
    }
    if (!isPlainObject(event.payload)) {
        throw new TypeError("gate1b-event-payload");
    }
    return event;
}

function appendValidatedGate1bEvent(event) {
    validateGate1bEvent(event);
    event = cloneAndFreezeJson(event);
    gate1bEventLog.push(event);
    return event.sequence;
}

function validateGate1bSnapshot(snapshot) {
    requireExactKeys(snapshot, gate1bSnapshotKeys, "gate1b-snapshot");
    requireExactKeys(
        snapshot.authority,
        gate1bAuthorityKeys,
        "gate1b-authority",
    );
    for (const key of gate1bAuthorityKeys) {
        if (snapshot.authority[key] !== false) {
            throw new TypeError("gate1b-authority-value");
        }
    }
    requireExactKeys(
        snapshot.capabilities,
        gate1bCapabilityKeys,
        "gate1b-capabilities",
    );
    for (const key of gate1bCapabilityKeys) {
        if (typeof snapshot.capabilities[key] !== "boolean") {
            throw new TypeError("gate1b-capability-value");
        }
    }
    if (!isPlainObject(snapshot.checks)) {
        throw new TypeError("gate1b-checks");
    }
    for (const [name, check] of Object.entries(snapshot.checks)) {
        if (!gate1bTypePattern.test(name)) {
            throw new TypeError("gate1b-check-name");
        }
        requireExactKeys(check, gate1bCheckKeys, "gate1b-check");
        if (
            typeof check.passed !== "boolean"
            || !isPlainObject(check.detail)
        ) {
            throw new TypeError("gate1b-check-value");
        }
    }
    requireExactKeys(
        snapshot.cycleSummary,
        gate1bCycleSummaryKeys,
        "gate1b-cycle-summary",
    );
    if (
        typeof snapshot.cycleSummary.status !== "string"
        || !Number.isSafeInteger(snapshot.cycleSummary.completed)
        || snapshot.cycleSummary.completed < 0
    ) {
        throw new TypeError("gate1b-cycle-summary-value");
    }
    if (!Array.isArray(snapshot.failures)) {
        throw new TypeError("gate1b-failures");
    }
    for (const failure of snapshot.failures) {
        requireExactKeys(failure, gate1bFailureKeys, "gate1b-failure");
        if (
            typeof failure.code !== "string"
            || !gate1bTypePattern.test(failure.code)
            || typeof failure.detail !== "string"
        ) {
            throw new TypeError("gate1b-failure-value");
        }
    }
    if (typeof snapshot.phase !== "string" || snapshot.phase.length === 0) {
        throw new TypeError("gate1b-phase");
    }
    return snapshot;
}

function canonicalJson(value) {
    if (Array.isArray(value)) {
        return value.map(canonicalJson);
    }
    if (isPlainObject(value)) {
        return Object.fromEntries(
            Object.keys(value).sort().map(
                (key) => [key, canonicalJson(value[key])],
            ),
        );
    }
    return value;
}

function jsonEquivalent(left, right) {
    return JSON.stringify(canonicalJson(left))
        === JSON.stringify(canonicalJson(right));
}

function appendBrowserTerminalEvent(code, detail) {
    const previous = gate1bEventLog.at(-1)?.monotonicMicroseconds ?? -1;
    const sampled = Math.floor(performance.now() * 1000);
    const event = Object.freeze({
        sequence: gate1bEventLog.length,
        monotonicMicroseconds: Math.max(previous + 1, sampled, 0),
        type: "terminal-failure",
        payload: Object.freeze({
            code,
            detail,
        }),
    });
    validateGate1bEvent(event);
    return appendValidatedGate1bEvent(event);
}

function fail(code, detail = {}) {
    if (terminalFailure === null) {
        const normalizedDetail = normalizeFailureDetail(detail);
        terminalFailure = Object.freeze({
            code,
            detail: normalizedDetail,
        });
        qtEventPump?.stop(`terminal-${code}`);
        appendBrowserTerminalEvent(code, normalizedDetail);
        if (!readySettled) {
            readySettled = true;
            rejectReady(terminalFailure);
        }
    }
    const error = new Error(code);
    error.gate1bCode = code;
    throw error;
}

function appendGate1bEvent(event) {
    if (terminalFailure !== null) {
        throw new Error("gate1b-terminal-latched");
    }
    try {
        const sequence = appendValidatedGate1bEvent(event);
        applicationCycleEventObserver?.(gate1bEventLog.at(-1));
        bfcacheEventObserver?.(gate1bEventLog.at(-1));
        foregroundInputEventObserver?.(gate1bEventLog.at(-1));
        foregroundTimerEventObserver?.(gate1bEventLog.at(-1));
        hiddenTimerEventObserver?.(gate1bEventLog.at(-1));
        return sequence;
    } catch (error) {
        fail("gate1b-event-schema", {
            message: String(error),
        });
    }
}

function publishGate1bSnapshot(snapshot) {
    if (terminalFailure !== null) {
        throw new Error("gate1b-terminal-latched");
    }
    try {
        validateGate1bSnapshot(snapshot);
        publishedGate1bSnapshot = cloneAndFreezeJson(snapshot);
        return true;
    } catch (error) {
        fail("gate1b-snapshot-schema", {
            message: String(error),
        });
    }
}

function resolveGate1bReady(snapshot) {
    if (terminalFailure !== null) {
        throw new Error("gate1b-terminal-latched");
    }
    try {
        validateGate1bSnapshot(snapshot);
        if (
            publishedGate1bSnapshot === null
            || !jsonEquivalent(snapshot, publishedGate1bSnapshot)
        ) {
            throw new TypeError("gate1b-ready-snapshot-mismatch");
        }
        if (readySettled) {
            throw new TypeError("gate1b-ready-already-settled");
        }
        readyResolutionRecord = cloneAndFreezeJson({
            events: gate1bEventLog,
            snapshot: publishedGate1bSnapshot,
        });
        readySettled = true;
        resolveReady(publishedGate1bSnapshot);
        return true;
    } catch (error) {
        fail("gate1b-ready-schema", {
            message: String(error),
        });
    }
}

function rejectGate1bReady(payload) {
    try {
        requireExactKeys(
            payload,
            gate1bRejectionKeys,
            "gate1b-ready-rejection",
        );
        if (
            typeof payload.code !== "string"
            || !gate1bTypePattern.test(payload.code)
            || !isPlainObject(payload.detail)
        ) {
            throw new TypeError("gate1b-ready-rejection-value");
        }
    } catch (error) {
        fail("gate1b-ready-rejection-schema", {
            message: String(error),
        });
    }
    fail(payload.code, payload.detail);
}

function takeMissedBfcacheExpectation() {
    const encoded = sessionStorage.getItem(bfcacheExpectationStorageKey);
    if (encoded === null) {
        return null;
    }
    sessionStorage.removeItem(bfcacheExpectationStorageKey);
    const runNonce = Number(encoded);
    return Number.isSafeInteger(runNonce) && runNonce > 0
        ? runNonce
        : 0;
}

function installQtEventPump(instance, screen) {
    if (eventPumpTelemetryReader !== null) {
        fail("runtime-event-pump-duplicate");
    }
    if (
        typeof requestAnimationFrame !== "function"
        || typeof cancelAnimationFrame !== "function"
    ) {
        fail("preflight-qt-event-pump");
    }
    const nativeDescriptor = Object.getOwnPropertyDescriptor(
        instance,
        "qtSendPendingEvents",
    );
    const applicationDescriptor = Object.getOwnPropertyDescriptor(
        instance,
        "qtSendPendingApplicationEvents",
    );
    if (
        nativeDescriptor?.value !== instance.qtSendPendingEvents
        || typeof instance.qtSendPendingEvents !== "function"
        || (
            applicationDescriptor?.value
            !== instance.qtSendPendingApplicationEvents
        )
        || typeof instance.qtSendPendingApplicationEvents !== "function"
    ) {
        fail("runtime-event-pump-entry-missing");
    }

    const directQtSendPendingEvents =
        instance.qtSendPendingEvents.bind(instance);
    const directQtSendPendingApplicationEvents =
        instance.qtSendPendingApplicationEvents.bind(instance);
    const telemetry = {
        applicationCyclePumpSerial: 0,
        bfcacheRestores: 0,
        bfcacheResumePumpSerial: 0,
        bfcacheSentinelsQueued: 0,
        calls: 0,
        commandKicks: 0,
        exclusiveDeferrals: 0,
        foregroundInputLatencyMilliseconds: [],
        foregroundTimerPumpSerials: [],
        fullCycleDeferrals: 0,
        hiddenIdleTimers: 0,
        hiddenQtTimerPumpSerial: 0,
        hiddenQtTimerSentinels: 0,
        idleFrames: 0,
        inputKicks: 0,
        lifecyclePauses: 0,
        maxConcurrentCalls: 0,
        maxNativeDispatchDepth: 0,
        nonBubblingInputKicks: 0,
        reentrantInputCalls: 0,
        resumedQtTimerPumpSerial: 0,
        resumedQtTimerSentinels: 0,
        visibilityReschedules: 0,
    };
    let active = true;
    let activeApplicationPump = null;
    let idleFramesEnabled = true;
    let animationFrameId = null;
    let hiddenIdleTimerId = null;
    let lifecyclePaused = false;
    let bfcacheNavigationArmed = false;
    let bfcacheResumeProbe = null;
    let foregroundInputCapture = null;
    let foregroundInputExpectedSamples = 0;
    let foregroundInputNextOrdinal = 1;
    let concurrentCalls = 0;
    let nativeDispatchDepth = 0;
    let primaryPumpInFlight = null;
    let nextPumpSerial = 1;
    const pumpsInFlight = new Set();
    const ownerDocument = screen.ownerDocument;
    const qtShadowRoot = screen.querySelector(
        ":scope > #qt-shadow-container",
    )?.shadowRoot ?? null;
    if (!(qtShadowRoot instanceof ShadowRoot)) {
        fail("runtime-event-pump-shadow-root");
    }
    const qtWindowWakeTargets = new Set();

    const requirePumpPromise = (pending) => {
        if (
            pending === null
            || (
                typeof pending !== "object"
                && typeof pending !== "function"
            )
            || typeof pending.then !== "function"
        ) {
            throw new TypeError("runtime-event-pump-not-promise");
        }
        return pending;
    };
    const qtSendPendingEvents = () => {
        const drained = directQtSendPendingEvents();
        if (typeof drained !== "boolean") {
            throw new TypeError("runtime-native-event-pump-result");
        }
        return drained;
    };
    const qtSendPendingApplicationEvents = () => requirePumpPromise(
        directQtSendPendingApplicationEvents(),
    );

    const snapshotTelemetry = () => Object.freeze({
        applicationCyclePumpSerial:
            telemetry.applicationCyclePumpSerial,
        bfcacheRestores: telemetry.bfcacheRestores,
        bfcacheResumePumpSerial: telemetry.bfcacheResumePumpSerial,
        bfcacheSentinelsQueued: telemetry.bfcacheSentinelsQueued,
        calls: telemetry.calls,
        commandKicks: telemetry.commandKicks,
        exclusiveDeferrals: telemetry.exclusiveDeferrals,
        foregroundInputLatencyMilliseconds: Object.freeze(
            [...telemetry.foregroundInputLatencyMilliseconds],
        ),
        foregroundInputLatencySamples:
            telemetry.foregroundInputLatencyMilliseconds.length,
        foregroundTimerPumpSerials: Object.freeze(
            [...telemetry.foregroundTimerPumpSerials],
        ),
        fullCycleDeferrals: telemetry.fullCycleDeferrals,
        hiddenIdleTimers: telemetry.hiddenIdleTimers,
        hiddenQtTimerPumpSerial: telemetry.hiddenQtTimerPumpSerial,
        hiddenQtTimerSentinels: telemetry.hiddenQtTimerSentinels,
        idleFrames: telemetry.idleFrames,
        inFlight: pumpsInFlight.size !== 0,
        inputKicks: telemetry.inputKicks,
        lifecyclePaused,
        lifecyclePauses: telemetry.lifecyclePauses,
        maxConcurrentCalls: telemetry.maxConcurrentCalls,
        maxNativeDispatchDepth: telemetry.maxNativeDispatchDepth,
        nativeDispatchDepthLimit: maxSynchronousNativePumpDepth,
        nonBubblingInputKicks: telemetry.nonBubblingInputKicks,
        reentrantInputCalls: telemetry.reentrantInputCalls,
        resumedQtTimerPumpSerial: telemetry.resumedQtTimerPumpSerial,
        resumedQtTimerSentinels: telemetry.resumedQtTimerSentinels,
        stopped: !active,
        visibilityReschedules: telemetry.visibilityReschedules,
        windowWakeTargets: qtWindowWakeTargets.size,
    });

    applicationCycleEventObserver = (event) => {
        if (
            event.type !== "check-passed"
            || event.payload.check !== "qt-application-cycle-order"
        ) {
            return;
        }
        if (
            activeApplicationPump === null
            || activeApplicationPump.reason !== "runtime-command"
            || telemetry.applicationCyclePumpSerial !== 0
        ) {
            fail("application-cycle-pump-owner-mismatch");
        }
        telemetry.applicationCyclePumpSerial =
            activeApplicationPump.serial;
    };
    bfcacheEventObserver = (event) => {
        if (event.type !== "qt-bfcache-resume-sentinel") {
            return;
        }
        if (
            activeApplicationPump?.reason !== "lifecycle-resume"
            || telemetry.bfcacheResumePumpSerial !== 0
        ) {
            fail("bfcache-resume-pump-owner-mismatch");
        }
        telemetry.bfcacheResumePumpSerial =
            activeApplicationPump.serial;
    };
    const armForegroundInputSamples = (sampleCount) => {
        if (
            sampleCount !== foregroundInputSampleTarget
            || foregroundInputExpectedSamples !== 0
            || foregroundInputCapture !== null
            || telemetry.foregroundInputLatencyMilliseconds.length !== 0
        ) {
            throw new Error("foreground-input-sampling-arm-invalid");
        }
        foregroundInputExpectedSamples = sampleCount;
        foregroundInputNextOrdinal = 1;
    };
    const onTrustedForegroundInputCapture = (event) => {
        if (
            foregroundInputExpectedSamples === 0
            || event.type !== "pointermove"
            || event.isTrusted !== true
            || ownerDocument.visibilityState !== "visible"
        ) {
            return;
        }
        if (foregroundInputCapture !== null) {
            fail("foreground-input-capture-overlap");
        }
        foregroundInputCapture = Object.freeze({
            ordinal: foregroundInputNextOrdinal,
            startedAt: performance.now(),
        });
        foregroundInputNextOrdinal += 1;
    };
    foregroundInputEventObserver = (event) => {
        if (event.type !== "qt-foreground-input-sample") {
            return;
        }
        requireExactKeys(
            event.payload,
            ["ordinal", "spontaneous"],
            "qt-foreground-input-sample",
        );
        if (
            foregroundInputExpectedSamples === 0
            || foregroundInputCapture === null
            || event.payload.spontaneous !== true
            || event.payload.ordinal !== foregroundInputCapture.ordinal
        ) {
            fail("foreground-input-sample-unowned");
        }
        telemetry.foregroundInputLatencyMilliseconds.push(
            Math.max(
                0,
                performance.now() - foregroundInputCapture.startedAt,
            ),
        );
        foregroundInputCapture = null;
        if (
            telemetry.foregroundInputLatencyMilliseconds.length
            === foregroundInputExpectedSamples
        ) {
            foregroundInputExpectedSamples = 0;
        }
    };
    foregroundTimerEventObserver = (event) => {
        if (event.type !== "qt-foreground-timer-sample") {
            return;
        }
        requireExactKeys(
            event.payload,
            ["latenessMicroseconds", "ordinal"],
            "qt-foreground-timer-sample",
        );
        const reason = activeApplicationPump?.reason ?? null;
        if (
            !["idle-frame", "runtime-command"].includes(reason)
            || event.payload.ordinal
                !== telemetry.foregroundTimerPumpSerials.length + 1
        ) {
            fail("foreground-qt-timer-pump-owner-mismatch");
        }
        telemetry.foregroundTimerPumpSerials.push(
            activeApplicationPump.serial,
        );
    };
    hiddenTimerEventObserver = (event) => {
        if (
            event.type !== "qt-hidden-timer-sentinel"
            && event.type !== "qt-visible-resume-timer-sentinel"
        ) {
            return;
        }
        requireExactKeys(
            event.payload,
            ["intervalMilliseconds"],
            "qt-lifecycle-timer-sentinel",
        );
        if (event.type === "qt-hidden-timer-sentinel") {
            if (
                ownerDocument.visibilityState !== "hidden"
                || activeApplicationPump?.reason !== "hidden-idle-timer"
                || telemetry.hiddenQtTimerPumpSerial !== 0
            ) {
                fail("hidden-qt-timer-pump-owner-mismatch");
            }
            telemetry.hiddenQtTimerPumpSerial =
                activeApplicationPump.serial;
            telemetry.hiddenQtTimerSentinels += 1;
            return;
        }
        if (
            ownerDocument.visibilityState !== "visible"
            || activeApplicationPump?.reason !== "idle-frame"
            || telemetry.resumedQtTimerPumpSerial !== 0
        ) {
            fail("resumed-qt-timer-pump-owner-mismatch");
        }
        telemetry.resumedQtTimerPumpSerial =
            activeApplicationPump.serial;
        telemetry.resumedQtTimerSentinels += 1;
    };
    const onNativeDispatch = (event) => {
        try {
            void kick("input");
        } catch (error) {
            if (terminalFailure === null) {
                throw error;
            }
        }
        if (
            event.type === "pointermove"
            && event.isTrusted === true
            && foregroundInputCapture !== null
        ) {
            fail("foreground-input-not-drained-synchronously");
        }
    };
    const onQtWindowNativeDispatch = (event) => {
        telemetry.nonBubblingInputKicks += 1;
        onNativeDispatch(event);
    };
    const cancelIdleSchedule = () => {
        if (animationFrameId !== null) {
            cancelAnimationFrame(animationFrameId);
            animationFrameId = null;
        }
        if (hiddenIdleTimerId !== null) {
            clearTimeout(hiddenIdleTimerId);
            hiddenIdleTimerId = null;
        }
    };
    const onPageHide = (event) => {
        if (event.persisted === true) {
            lifecyclePaused = true;
            telemetry.lifecyclePauses += 1;
            cancelIdleSchedule();
            return;
        }
        stop("pagehide");
    };
    const onPageShow = (event) => {
        if (
            event.persisted !== true
            || !active
            || !lifecyclePaused
        ) {
            return;
        }
        lifecyclePaused = false;
        bfcacheNavigationArmed = false;
        telemetry.bfcacheRestores += 1;
        if (bfcacheResumeProbe !== null) {
            const storedNonce = sessionStorage.getItem(
                bfcacheExpectationStorageKey,
            );
            if (storedNonce !== String(bfcacheResumeProbe.runNonce)) {
                fail("bfcache-resume-expectation-mismatch");
            }
            sessionStorage.removeItem(bfcacheExpectationStorageKey);
            const handler = instance.qtSuspendResumeControl
                ?.eventHandlers?.[bfcacheResumeProbe.handlerIndex];
            if (typeof handler !== "function") {
                fail("bfcache-resume-handler-missing");
            }
            handler(Object.freeze({
                gate1bBfcacheNonce: bfcacheResumeProbe.runNonce,
            }));
            telemetry.bfcacheSentinelsQueued += 1;
            bfcacheResumeProbe = null;
        }
        void kick("lifecycle-resume");
    };
    const onVisibilityChange = () => {
        if (!active || lifecyclePaused) {
            return;
        }
        telemetry.visibilityReschedules += 1;
        cancelIdleSchedule();
        scheduleIdleFrame();
    };

    const forEachQtWindow = (node, callback) => {
        if (!(node instanceof Element)) {
            return;
        }
        if (node.matches(".qt-window")) {
            callback(node);
        }
        for (const windowElement of node.querySelectorAll(".qt-window")) {
            callback(windowElement);
        }
    };
    const attachQtWindowWakeTarget = (windowElement) => {
        if (qtWindowWakeTargets.has(windowElement)) {
            return;
        }
        for (const eventType of qtWindowNativeDispatchEventTypes) {
            // Qt registered its same-target listeners before qtLoad()
            // completed. This listener therefore drains only after Qt queues
            // a non-bubbling pointerenter/pointerleave event.
            windowElement.addEventListener(
                eventType,
                onQtWindowNativeDispatch,
            );
        }
        qtWindowWakeTargets.add(windowElement);
    };
    const detachQtWindowWakeTarget = (windowElement) => {
        if (!qtWindowWakeTargets.delete(windowElement)) {
            return;
        }
        for (const eventType of qtWindowNativeDispatchEventTypes) {
            windowElement.removeEventListener(
                eventType,
                onQtWindowNativeDispatch,
            );
        }
    };
    const qtWindowObserver = new MutationObserver((records) => {
        for (const record of records) {
            for (const removedNode of record.removedNodes) {
                forEachQtWindow(removedNode, detachQtWindowWakeTarget);
            }
            for (const addedNode of record.addedNodes) {
                forEachQtWindow(addedNode, attachQtWindowWakeTarget);
            }
        }
    });
    qtWindowObserver.observe(qtShadowRoot, {
        childList: true,
        subtree: true,
    });
    for (const windowElement of qtShadowRoot.querySelectorAll(".qt-window")) {
        attachQtWindowWakeTarget(windowElement);
    }
    // qtLoad() can resolve after the screen shadow root exists but before
    // Qt materializes its first platform window. The observer is installed
    // before this initial scan so that an empty scan has no attachment gap.

    const scheduleIdleFrame = () => {
        if (
            !active
            || !idleFramesEnabled
            || lifecyclePaused
            || bfcacheNavigationArmed
            || animationFrameId !== null
            || hiddenIdleTimerId !== null
        ) {
            return;
        }
        if (ownerDocument.visibilityState === "hidden") {
            hiddenIdleTimerId = setTimeout(() => {
                hiddenIdleTimerId = null;
                void kick("hidden-idle-timer").then((drained) => {
                    if (drained === true) {
                        telemetry.hiddenIdleTimers += 1;
                    }
                });
            }, hiddenIdleFallbackMilliseconds);
            return;
        }
        animationFrameId = requestAnimationFrame(() => {
            animationFrameId = null;
            void kick("idle-frame");
        });
    };

    function stop(_reason) {
        if (!active) {
            return;
        }
        active = false;
        cancelIdleSchedule();
        applicationCycleEventObserver = null;
        bfcacheEventObserver = null;
        foregroundInputEventObserver = null;
        foregroundTimerEventObserver = null;
        hiddenTimerEventObserver = null;
        screen.removeEventListener(
            "pointermove",
            onTrustedForegroundInputCapture,
            true,
        );
        for (const eventType of qtScreenNativeDispatchEventTypes) {
            screen.removeEventListener(eventType, onNativeDispatch);
        }
        if (negativeMode === "native-depth-limit") {
            screen.removeEventListener(
                nativeDepthLimitAttemptEventType,
                onNativeDispatch,
            );
        }
        for (const eventType of qtDocumentNativeDispatchEventTypes) {
            ownerDocument.removeEventListener(
                eventType,
                onNativeDispatch,
            );
        }
        qtWindowObserver.disconnect();
        for (const windowElement of qtWindowWakeTargets) {
            for (const eventType of qtWindowNativeDispatchEventTypes) {
                windowElement.removeEventListener(
                    eventType,
                    onQtWindowNativeDispatch,
                );
            }
        }
        qtWindowWakeTargets.clear();
        globalThis.removeEventListener("pagehide", onPageHide);
        globalThis.removeEventListener("pageshow", onPageShow);
        ownerDocument.removeEventListener(
            "visibilitychange",
            onVisibilityChange,
        );
    }

    function quiesceForTerminalProbe() {
        if (!active || pumpsInFlight.size !== 0) {
            throw new Error("runtime-terminal-probe-not-quiescent");
        }
        idleFramesEnabled = false;
        cancelIdleSchedule();
    }

    function armBfcacheResumeProbe(handlerIndex, runNonce) {
        const handler = instance.qtSuspendResumeControl
            ?.eventHandlers?.[handlerIndex];
        if (
            bfcacheResumeProbe !== null
            || !Number.isSafeInteger(handlerIndex)
            || handlerIndex <= 0
            || !Number.isSafeInteger(runNonce)
            || runNonce <= 0
            || typeof handler !== "function"
        ) {
            throw new Error("bfcache-resume-probe-arm-invalid");
        }
        bfcacheResumeProbe = Object.freeze({
            handlerIndex,
            runNonce,
        });
        if (
            sessionStorage.getItem(bfcacheExpectationStorageKey)
            !== null
        ) {
            throw new Error("bfcache-resume-expectation-duplicate");
        }
        sessionStorage.setItem(
            bfcacheExpectationStorageKey,
            String(runNonce),
        );
        bfcacheNavigationArmed = true;
        cancelIdleSchedule();
    }

    function handlePumpFailure(error, code = "runtime-event-pump-rejected") {
        stop("pump-rejection");
        if (terminalFailure === null) {
            try {
                fail(code, {
                    message: String(error),
                });
            } catch {
                // The terminal event owns this pump rejection.
            }
        }
        return false;
    }

    function startPumpCall(primary, reason) {
        if (
            primary
            && reason !== "input"
            && activeApplicationPump !== null
        ) {
            throw new Error("runtime-application-pump-owner-overlap");
        }
        const pumpSerial = nextPumpSerial;
        nextPumpSerial += 1;
        let settleCurrentPump;
        const currentPump = new Promise((resolve) => {
            settleCurrentPump = resolve;
        });
        pumpsInFlight.add(currentPump);
        if (primary) {
            primaryPumpInFlight = currentPump;
        }
        if (primary && reason !== "input") {
            activeApplicationPump = Object.freeze({
                reason,
                serial: pumpSerial,
            });
        }
        concurrentCalls += 1;
        telemetry.calls += 1;
        telemetry.maxConcurrentCalls = Math.max(
            telemetry.maxConcurrentCalls,
            concurrentCalls,
        );

        let completed = false;
        const complete = (result) => {
            if (completed) {
                return;
            }
            completed = true;
            concurrentCalls -= 1;
            pumpsInFlight.delete(currentPump);
            if (primaryPumpInFlight === currentPump) {
                primaryPumpInFlight = null;
            }
            if (activeApplicationPump?.serial === pumpSerial) {
                activeApplicationPump = null;
            }
            if (active) {
                scheduleIdleFrame();
            }
            settleCurrentPump(result);
        };

        try {
            // Native DOM dispatch is deliberately synchronous: nested input
            // drains stay LIFO while a promising application pump is suspended.
            // Browser-owned async APIs remain outside this pump boundary.
            if (reason === "input") {
                let drained;
                nativeDispatchDepth += 1;
                telemetry.maxNativeDispatchDepth = Math.max(
                    telemetry.maxNativeDispatchDepth,
                    nativeDispatchDepth,
                );
                try {
                    drained = qtSendPendingEvents();
                } catch (error) {
                    complete(handlePumpFailure(
                        error,
                        "runtime-native-event-pump-failed",
                    ));
                    return currentPump;
                } finally {
                    nativeDispatchDepth -= 1;
                }
                if (drained) {
                    complete(true);
                } else {
                    telemetry.exclusiveDeferrals += 1;
                    complete(false);
                }
                return currentPump;
            }

            const pending = qtSendPendingApplicationEvents();
            pending.then((drained) => {
                if (drained === true) {
                    complete(true);
                    return;
                }
                if (drained !== false) {
                    complete(handlePumpFailure(
                        new TypeError("runtime-event-pump-result"),
                    ));
                    return;
                }
                telemetry.fullCycleDeferrals += 1;
                complete(false);
            }, (error) => complete(handlePumpFailure(error)));
        } catch (error) {
            complete(handlePumpFailure(error));
        }
        return currentPump;
    }

    function whenIdle() {
        const pending = [...pumpsInFlight];
        if (pending.length === 0) {
            return Promise.resolve(true);
        }
        return Promise.all(pending).then(() => whenIdle());
    }

    function kick(reason) {
        if (!active) {
            return Promise.resolve(false);
        }
        switch (reason) {
        case "idle-frame":
            telemetry.idleFrames += 1;
            break;
        case "hidden-idle-timer":
        case "lifecycle-resume":
            telemetry.idleFrames += 1;
            break;
        case "input":
            telemetry.inputKicks += 1;
            break;
        case "runtime-command":
            telemetry.commandKicks += 1;
            break;
        default:
            throw new Error(`runtime-event-pump-reason:${String(reason)}`);
        }

        if (reason === "input") {
            if (nativeDispatchDepth >= maxSynchronousNativePumpDepth) {
                fail("runtime-native-event-pump-depth-limit", {
                    activeDepth: nativeDispatchDepth,
                    limit: maxSynchronousNativePumpDepth,
                });
            }
        }
        if (reason === "input" && pumpsInFlight.size !== 0) {
            telemetry.reentrantInputCalls += 1;
            return startPumpCall(false, reason);
        }
        if (pumpsInFlight.size !== 0) {
            return primaryPumpInFlight ?? whenIdle();
        }
        return startPumpCall(true, reason);
    }

    screen.addEventListener(
        "pointermove",
        onTrustedForegroundInputCapture,
        true,
    );
    for (const eventType of qtScreenNativeDispatchEventTypes) {
        screen.addEventListener(eventType, onNativeDispatch);
    }
    if (negativeMode === "native-depth-limit") {
        screen.addEventListener(
            nativeDepthLimitAttemptEventType,
            onNativeDispatch,
        );
    }
    // Chromium installs Qt's clipboard listeners on document. Registering
    // here, after qtLoad(), preserves same-target listener order: Qt queues
    // first, then this listener drains while clipboardData is still live.
    for (const eventType of qtDocumentNativeDispatchEventTypes) {
        ownerDocument.addEventListener(eventType, onNativeDispatch);
    }
    globalThis.addEventListener("pagehide", onPageHide);
    globalThis.addEventListener("pageshow", onPageShow);
    ownerDocument.addEventListener(
        "visibilitychange",
        onVisibilityChange,
    );
    eventPumpTelemetryReader = snapshotTelemetry;

    return Object.freeze({
        armBfcacheResumeProbe,
        armForegroundInputSamples,
        kick,
        quiesceForTerminalProbe,
        stop,
        whenIdle,
    });
}

function describeUnhandledRejection(event) {
    try {
        return String(event.reason);
    } catch {
        return "<unprintable-rejection-reason>";
    }
}

function onUnhandledRejection(event) {
    const reason = describeUnhandledRejection(event);
    try {
        fail("unhandled-rejection", {
            reason,
        });
    } catch {
        // The terminal record already owns this failure.
    }
}

globalThis.addEventListener("unhandledrejection", onUnhandledRejection);
globalThis.addEventListener("securitypolicyviolation", (event) => {
    try {
        if (
            negativeMode === "blocked-worker-src"
            && event.effectiveDirective.startsWith("worker-src")
        ) {
            if (activeWorkerCspReject !== null) {
                const error = new Error("worker-csp-violation");
                error.gate1bWorkerCsp = true;
                activeWorkerCspReject(error);
                return;
            }
            fail("preflight-worker-csp-blocked", {
                directive: event.effectiveDirective,
            });
        }
        fail("csp-violation", {
            directive: event.effectiveDirective,
        });
    } catch {
        // The terminal record already owns this failure.
    }
});

function bytesToHex(bytes) {
    return [...bytes].map(
        (value) => value.toString(16).padStart(2, "0"),
    ).join("");
}

function bytesToBase64(bytes) {
    let binary = "";
    for (const value of bytes) {
        binary += String.fromCharCode(value);
    }
    return btoa(binary);
}

async function digest(bytes) {
    const result = new Uint8Array(
        await crypto.subtle.digest("SHA-256", bytes),
    );
    return {
        hex: bytesToHex(result),
        sri: `sha256-${bytesToBase64(result)}`,
    };
}

function samePropertyDescriptor(left, right) {
    if (left === undefined || right === undefined) {
        return left === right;
    }
    return (
        left.configurable === right.configurable
        && left.enumerable === right.enumerable
        && left.get === right.get
        && left.set === right.set
        && left.value === right.value
        && left.writable === right.writable
    );
}

function installQtShadowStyleAdapter(screen) {
    const prototype = ShadowRoot.prototype;
    const propertyName = "appendChild";
    const originalOwnDescriptor = Object.getOwnPropertyDescriptor(
        prototype,
        propertyName,
    );
    const originalAppendChild = prototype.appendChild;
    if (
        typeof originalAppendChild !== "function"
        || (
            originalOwnDescriptor !== undefined
            && originalOwnDescriptor.configurable !== true
        )
        || (
            originalOwnDescriptor === undefined
            && !Object.isExtensible(prototype)
        )
    ) {
        fail("preflight-constructable-stylesheet");
    }

    const adoptedRoots = new WeakMap();
    const adoptionRecords = [];
    let adoptionCount = 0;
    let active = true;
    let ownershipMonitor = null;
    const installedOwnDescriptor = Object.freeze({
        configurable: true,
        enumerable: originalOwnDescriptor?.enumerable ?? false,
        value: adapterAppendChild,
        writable: true,
    });

    const reportOwnershipLoss = (phase) => {
        active = false;
        if (ownershipMonitor !== null) {
            clearInterval(ownershipMonitor);
            ownershipMonitor = null;
        }
        fail("qt-style-adapter-ownership-loss", { phase });
    };

    const ownsAppendChild = () => {
        const descriptor = Object.getOwnPropertyDescriptor(
            prototype,
            propertyName,
        );
        return samePropertyDescriptor(descriptor, installedOwnDescriptor);
    };

    const restore = (phase) => {
        if (!active) {
            return;
        }
        if (ownershipMonitor !== null) {
            clearInterval(ownershipMonitor);
            ownershipMonitor = null;
        }
        if (!ownsAppendChild()) {
            reportOwnershipLoss(phase);
        }
        if (originalOwnDescriptor === undefined) {
            let deleted = false;
            try {
                deleted = Reflect.deleteProperty(prototype, propertyName);
            } catch {
                reportOwnershipLoss(phase);
            }
            if (!deleted) {
                reportOwnershipLoss(phase);
            }
        } else {
            try {
                Object.defineProperty(
                    prototype,
                    propertyName,
                    originalOwnDescriptor,
                );
            } catch {
                reportOwnershipLoss(phase);
            }
        }
        const restored = Object.getOwnPropertyDescriptor(
            prototype,
            propertyName,
        );
        if (
            !samePropertyDescriptor(restored, originalOwnDescriptor)
            || prototype.appendChild !== originalAppendChild
        ) {
            reportOwnershipLoss(phase);
        }
        active = false;
    };

    const restoreAndFail = (code, detail) => {
        restore(code);
        fail(code, detail);
    };

    function adapterAppendChild(child) {
        const root = this;
        const matchesOwnedQtStyle = (
            root instanceof ShadowRoot
            && child instanceof HTMLStyleElement
            && root.host.id === qtShadowHostId
            && root.host.shadowRoot === root
            && root.ownerDocument === screen.ownerDocument
            && root.host.ownerDocument === screen.ownerDocument
            && child.ownerDocument === screen.ownerDocument
            && root.host.parentNode === screen
            && screen.id === "screen"
            && child.parentNode === null
            && !child.isConnected
        );
        if (!matchesOwnedQtStyle) {
            return Reflect.apply(originalAppendChild, root, arguments);
        }
        if (adoptionCount !== 0 || adoptedRoots.has(root)) {
            fail("qt-style-duplicate-root");
        }

        const text = child.textContent;
        const foldedText = text.toLowerCase();
        const bytes = new TextEncoder().encode(text);
        if (
            !text.startsWith("\n")
            || bytes.byteLength !== qtStyleByteLength
            || foldedText.includes("@import")
            || foldedText.includes("url(")
            || qtStyleKeySelectors.some(
                (selector) => !text.includes(selector),
            )
        ) {
            fail("qt-style-shape");
        }

        const sheet = new CSSStyleSheet();
        try {
            sheet.replaceSync(text);
        } catch {
            fail("qt-style-shape");
        }
        if (sheet.cssRules.length !== qtStyleRuleCount) {
            fail("qt-style-shape");
        }

        const previousSheets = [...root.adoptedStyleSheets];
        root.adoptedStyleSheets = [...previousSheets, sheet];
        const currentSheets = [...root.adoptedStyleSheets];
        if (
            currentSheets.length !== previousSheets.length + 1
            || previousSheets.some(
                (candidate, index) => currentSheets[index] !== candidate,
            )
            || currentSheets.at(-1) !== sheet
            || currentSheets.filter(
                (candidate) => candidate === sheet,
            ).length !== 1
            || child.parentNode !== null
            || child.isConnected
        ) {
            fail("qt-style-shape");
        }

        const verification = digest(bytes).then(
            (actual) => {
                if (actual.hex !== qtStyleSha256) {
                    return Object.freeze({
                        actual: actual.hex,
                        verified: false,
                    });
                }
                return Object.freeze({
                    actual,
                    verified: true,
                });
            },
            (error) => Object.freeze({
                error: String(error),
                verified: false,
            }),
        );
        const record = Object.freeze({
            bytes,
            child,
            previousSheets: Object.freeze(previousSheets),
            root,
            sheet,
            verification,
        });
        adoptedRoots.set(root, record);
        adoptionRecords.push(record);
        adoptionCount += 1;
        return child;
    }

    Object.defineProperty(
        prototype,
        propertyName,
        installedOwnDescriptor,
    );
    if (!ownsAppendChild()) {
        reportOwnershipLoss("installation");
    }
    ownershipMonitor = setInterval(() => {
        if (active && !ownsAppendChild()) {
            try {
                reportOwnershipLoss("lifetime-monitor");
            } catch {
                // The terminal report owns the detected replacement.
            }
        }
    }, 50);

    return Object.freeze({
        async requireInitialAdoption() {
            if (!ownsAppendChild()) {
                reportOwnershipLoss("initial-adoption");
            }
            if (adoptionCount === 0) {
                fail("qt-style-initial-missing");
            }
            if (adoptionCount !== 1) {
                fail("qt-style-duplicate-root");
            }
            const record = adoptionRecords[0];
            const verification = await record.verification;
            if (!verification.verified) {
                fail("qt-style-fingerprint", verification);
            }
            const bytes = record.bytes;
            const sheet = record.sheet;
            const actual = verification.actual;
            if (
                record.root.host.id !== qtShadowHostId
                || record.root.host.shadowRoot !== record.root
                || record.root.host.parentNode !== screen
                || record.root.querySelectorAll("style").length !== 0
                || screen.querySelectorAll("style").length !== 0
                || record.child.parentNode !== null
                || record.child.isConnected
                || sheet.cssRules.length !== qtStyleRuleCount
                || record.root.adoptedStyleSheets.filter(
                    (candidate) => candidate === record.sheet,
                ).length !== 1
            ) {
                fail("qt-style-shape");
            }
            if (!ownsAppendChild()) {
                reportOwnershipLoss("initial-adoption-complete");
            }
            if (styleAdoptionRecord !== null) {
                fail("qt-style-duplicate-root");
            }
            styleAdoptionRecord = Object.freeze({
                adoptionCount: 1,
                bytes: bytes.byteLength,
                hostId: qtShadowHostId,
                inlineStyleCount: 0,
                ruleCount: sheet.cssRules.length,
                sha256: actual.hex,
                stylesheetCount: 1,
            });
            return record;
        },
        restore,
        restoreAndFail,
    });
}

function expectedCspFor(role) {
    const expected = [
        "default-src 'self'",
        "script-src 'self' 'wasm-unsafe-eval'",
        "worker-src 'self'",
        `connect-src 'self' wss://127.0.0.1:${location.port}`,
        "img-src 'self' data:",
        "media-src 'self'",
        "font-src 'self'",
        "style-src 'self'",
        "object-src 'none'",
        "base-uri 'none'",
        "frame-ancestors 'none'",
        "form-action 'self'",
        "manifest-src 'self'",
    ].join("; ");
    if (negativeMode === "blocked-worker-src" && role === "html") {
        return expected.replace("worker-src 'self'", "worker-src 'none'");
    }
    return expected;
}

function expectedArtifactUrl(role, digest) {
    if (role === "html") {
        return "RhythmGameWasmProbe.html";
    }
    const [stem, extension] = artifactNames[role];
    return `${stem}.${digest}.${extension}`;
}

function inspectPolicy(response, role = null) {
    for (const [name, expected] of Object.entries(expectedPolicy)) {
        const actual = response.headers.get(name);
        if (actual === expected) {
            continue;
        }
        if (name === "cross-origin-opener-policy" && actual === null) {
            fail("policy-coop-missing");
        }
        if (name === "cross-origin-embedder-policy" && actual === null) {
            fail("policy-coep-missing");
        }
        fail("policy-header-mismatch", { actual, expected, name });
    }
    const csp = response.headers.get("content-security-policy");
    if (csp === null) {
        fail("policy-csp-missing");
    }
    if (!csp.includes("script-src 'self' 'wasm-unsafe-eval'")) {
        fail("policy-wasm-eval-missing");
    }
    const expectedCsp = expectedCspFor(role);
    if (csp !== expectedCsp) {
        fail("policy-csp-mismatch");
    }
    return csp;
}

function validateManifest(manifest) {
    if (
        manifest?.schemaVersion !== 1
        || !digestPattern.test(manifest?.buildId ?? "")
        || manifest?.artifacts === null
        || typeof manifest?.artifacts !== "object"
    ) {
        fail("artifact-manifest-shape");
    }
    const actualRoles = Object.keys(manifest.artifacts).sort();
    if (
        actualRoles.length !== expectedRoles.length
        || actualRoles.some((role, index) => role !== expectedRoles[index])
    ) {
        fail("artifact-manifest-roles", { actualRoles });
    }
    const urls = new Set();
    for (const [role, artifact] of Object.entries(manifest.artifacts)) {
        if (typeof artifact?.url !== "string") {
            continue;
        }
        const foldedUrl = artifact.url.toLowerCase();
        if (urls.has(foldedUrl)) {
            fail("artifact-duplicate-url", { role });
        }
        urls.add(foldedUrl);
    }
    for (const [role, artifact] of Object.entries(manifest.artifacts)) {
        if (
            artifact.buildId !== manifest.buildId
            || !Number.isSafeInteger(artifact.bytes)
            || artifact.bytes < 0
            || !digestPattern.test(artifact.sha256 ?? "")
            || !sriPattern.test(artifact.sri ?? "")
            || !contentAddressPattern.test(artifact.url ?? "")
            || artifact.mime !== expectedMimes[role]
        ) {
            fail("artifact-manifest-entry", { role });
        }
        if (artifact.url !== expectedArtifactUrl(role, artifact.sha256)) {
            fail("artifact-role-url", { role });
        }
    }
}

async function fetchOwned(url, role = null) {
    const response = await fetch(url, {
        cache: "no-store",
        credentials: "same-origin",
        redirect: "error",
    });
    if (!response.ok) {
        fail("artifact-http-status", {
            status: response.status,
            url: new URL(url).pathname,
        });
    }
    inspectPolicy(response, role);
    return {
        bytes: await response.arrayBuffer(),
        contentType: response.headers.get("content-type"),
        response,
    };
}

async function auditArtifact(role, artifact) {
    const url = new URL(artifact.url, document.baseURI);
    const fetched = await fetchOwned(url, role);
    if (fetched.contentType !== artifact.mime) {
        if (role === "wasm") {
            fail("artifact-wasm-mime", {
                actual: fetched.contentType,
            });
        }
        fail("artifact-mime", {
            actual: fetched.contentType,
            expected: artifact.mime,
            role,
        });
    }
    const actual = await digest(fetched.bytes);
    if (
        actual.hex !== artifact.sha256
        || fetched.bytes.byteLength !== artifact.bytes
    ) {
        fail(
            role === "wasm"
                ? "artifact-wasm-digest"
                : "artifact-digest",
            { role },
        );
    }
    if (actual.sri !== artifact.sri) {
        fail("artifact-sri", { role });
    }
    return Object.freeze({
        bytes: fetched.bytes,
        sri: artifact.sri,
        url: url.href,
    });
}

function preflightFeatures() {
    if (!isSecureContext) {
        fail("preflight-secure-context");
    }
    if (!crossOriginIsolated) {
        fail("preflight-cross-origin-isolated");
    }
    if (typeof SharedArrayBuffer !== "function") {
        fail("preflight-shared-array-buffer");
    }
    if (
        typeof WebAssembly.Suspending !== "function"
        || typeof WebAssembly.promising !== "function"
    ) {
        fail("preflight-jspi-api");
    }
    if (
        typeof CSSStyleSheet !== "function"
        || typeof CSSStyleSheet.prototype.replaceSync !== "function"
        || typeof ShadowRoot !== "function"
        || typeof HTMLStyleElement !== "function"
        || typeof TextEncoder !== "function"
        || typeof ShadowRoot.prototype.appendChild !== "function"
        || !("adoptedStyleSheets" in ShadowRoot.prototype)
    ) {
        fail("preflight-constructable-stylesheet");
    }
    const canvas = document.createElement("canvas");
    const context = canvas.getContext("webgl2");
    if (context === null) {
        fail("preflight-webgl2");
    }
    context.getExtension("WEBGL_lose_context")?.loseContext();
    const hasAudioWorklet = (
        typeof AudioWorklet === "function"
        || (
            typeof AudioContext === "function"
            && "audioWorklet" in AudioContext.prototype
        )
    );
    if (!hasAudioWorklet) {
        fail("preflight-audio-worklet");
    }
}

async function preflightWorker(workerUrl) {
    const nonce = crypto.randomUUID();
    const cspViolation = new Promise((_resolve, reject) => {
        activeWorkerCspReject = reject;
    });
    let worker;
    try {
        worker = new Worker(workerUrl, {
            name: "rhythmgame-gate1b-preflight",
            type: "module",
        });
    } catch (error) {
        activeWorkerCspReject = null;
        if (
            negativeMode === "blocked-worker-src"
            && error?.name === "SecurityError"
        ) {
            fail("preflight-worker-csp-blocked");
        }
        fail("preflight-worker-failed");
    }
    try {
        const reply = await Promise.race([
            new Promise((resolve, reject) => {
                const timer = setTimeout(
                    () => reject(new Error("worker-timeout")),
                    5000,
                );
                worker.addEventListener("message", (event) => {
                    clearTimeout(timer);
                    resolve(event.data);
                }, { once: true });
                worker.addEventListener("error", () => {
                    clearTimeout(timer);
                    reject(new Error("worker-error"));
                }, { once: true });
                worker.postMessage({ nonce });
            }),
            cspViolation,
        ]);
        if (
            reply?.nonce !== nonce
            || reply?.transport !== "dedicated-worker"
        ) {
            fail("preflight-worker-nonce");
        }
    } catch (error) {
        if (error?.gate1bWorkerCsp === true) {
            fail("preflight-worker-csp-blocked");
        }
        fail("preflight-worker-failed");
    } finally {
        activeWorkerCspReject = null;
        worker.terminate();
    }
}

async function preflightWebSocket() {
    const url = new URL("/probe/ws", location.href);
    url.protocol = "wss:";
    const entropy = crypto.getRandomValues(new Uint32Array(1))[0];
    const runNonce = (entropy % 0xFFFFFFFE) + 1;
    url.searchParams.set("nonce", String(runNonce));
    await new Promise((resolve, reject) => {
        const socket = new WebSocket(url);
        socket.binaryType = "arraybuffer";
        let protocolStep = "server-message";
        let settled = false;
        const settle = (error) => {
            if (settled) {
                return;
            }
            settled = true;
            clearTimeout(timer);
            if (error === undefined) {
                resolve();
                return;
            }
            try {
                socket.close();
            } catch {
                // The original protocol error remains authoritative.
            }
            reject(error);
        };
        const hasExactKeys = (value, expectedKeys) => {
            if (
                value === null
                || typeof value !== "object"
                || Array.isArray(value)
            ) {
                return false;
            }
            const keys = Object.keys(value).sort();
            return keys.length === expectedKeys.length
                && expectedKeys.every((key, index) => keys[index] === key);
        };
        const timer = setTimeout(() => {
            settle(new Error("wss-timeout"));
        }, 5000);
        socket.addEventListener("message", (event) => {
            try {
                if (protocolStep === "server-message") {
                    if (typeof event.data !== "string") {
                        throw new Error("wss-server-message-type");
                    }
                    const message = JSON.parse(event.data);
                    if (
                        !hasExactKeys(
                            message,
                            ["connectionId", "nonce", "type", "value"],
                        )
                        || typeof message.connectionId !== "string"
                        || message.connectionId.length === 0
                        || message.nonce !== runNonce
                        || message.type !== "server-message"
                        || message.value !== "connected"
                    ) {
                        throw new Error("wss-server-message");
                    }
                    protocolStep = "text-echo";
                    socket.send(`text-echo:${runNonce}`);
                    return;
                }
                if (protocolStep === "text-echo") {
                    if (event.data !== `text-echo:${runNonce}`) {
                        throw new Error("wss-text-echo");
                    }
                    protocolStep = "binary-echo";
                    socket.send(Uint8Array.of(1, 2, 3));
                    return;
                }
                if (protocolStep === "binary-echo") {
                    if (
                        !(event.data instanceof ArrayBuffer)
                        || ![1, 2, 3].every(
                            (value, index) => (
                                new Uint8Array(event.data)[index] === value
                            ),
                        )
                        || event.data.byteLength !== 3
                    ) {
                        throw new Error("wss-binary-echo");
                    }
                    protocolStep = "heartbeat";
                    socket.send(`heartbeat:${runNonce}`);
                    return;
                }
                if (protocolStep === "heartbeat") {
                    if (typeof event.data !== "string") {
                        throw new Error("wss-heartbeat-type");
                    }
                    const heartbeat = JSON.parse(event.data);
                    if (
                        !hasExactKeys(heartbeat, ["nonce", "type"])
                        || heartbeat.nonce !== runNonce
                        || heartbeat.type !== "heartbeat"
                    ) {
                        throw new Error("wss-heartbeat");
                    }
                    protocolStep = "close";
                    socket.send("close");
                    return;
                }
                throw new Error("wss-message-order");
            } catch (error) {
                settle(error);
            }
        });
        socket.addEventListener("close", (event) => {
            if (
                protocolStep === "close"
                && event.code === 1000
                && event.reason === "probe-complete"
            ) {
                settle();
            } else {
                settle(new Error(`wss-close-${event.code}`));
            }
        }, { once: true });
        socket.addEventListener("error", () => {
            settle(new Error("wss-error"));
        }, { once: true });
    }).catch(() => fail("preflight-wss"));
}

async function injectVerifiedScript(asset, failureCode) {
    await new Promise((resolve, reject) => {
        const script = document.createElement("script");
        script.src = asset.url;
        script.integrity = asset.sri;
        script.crossOrigin = "anonymous";
        script.addEventListener("load", resolve, { once: true });
        script.addEventListener("error", reject, { once: true });
        document.head.append(script);
    }).catch(() => fail(failureCode));
}

async function bootstrap() {
    const missedBfcacheRunNonce = takeMissedBfcacheExpectation();
    if (missedBfcacheRunNonce !== null) {
        fail("bfcache-restoration-missed", {
            runNonce: missedBfcacheRunNonce,
        });
    }
    const bootstrapResponse = await fetchOwned(import.meta.url);
    const manifestUrl = new URL("runtime-artifacts.json", document.baseURI);
    const manifestResponse = await fetchOwned(manifestUrl);
    let manifest;
    try {
        manifest = JSON.parse(
            new TextDecoder("utf-8", { fatal: true }).decode(
                manifestResponse.bytes,
            ),
        );
    } catch {
        fail("artifact-manifest-json");
    }
    validateManifest(manifest);

    const bootstrapArtifact = manifest.artifacts.bootstrap;
    if (
        new URL(import.meta.url).pathname.split("/").at(-1)
            !== bootstrapArtifact.url
    ) {
        fail("artifact-bootstrap-url");
    }
    if (bootstrapResponse.contentType !== bootstrapArtifact.mime) {
        fail("artifact-bootstrap-mime", {
            actual: bootstrapResponse.contentType,
            expected: bootstrapArtifact.mime,
        });
    }
    if (
        bootstrapResponse.bytes.byteLength
        !== bootstrapArtifact.bytes
    ) {
        fail("artifact-bootstrap-bytes", {
            actual: bootstrapResponse.bytes.byteLength,
            expected: bootstrapArtifact.bytes,
        });
    }
    const bootstrapDigest = await digest(bootstrapResponse.bytes);
    if (
        bootstrapDigest.hex !== bootstrapArtifact.sha256
        || bootstrapDigest.sri !== bootstrapArtifact.sri
    ) {
        fail("artifact-bootstrap-digest");
    }

    const audited = {};
    for (const role of expectedRoles) {
        if (role === "bootstrap") {
            audited[role] = Object.freeze({
                bytes: bootstrapResponse.bytes,
                sri: bootstrapArtifact.sri,
                url: import.meta.url,
            });
        } else {
            audited[role] = await auditArtifact(
                role,
                manifest.artifacts[role],
            );
        }
    }

    preflightFeatures();
    await preflightWorker(audited.preflightWorker.url);
    await preflightWebSocket();

    await injectVerifiedScript(
        audited.mainJs,
        "sri-main-js-rejected",
    );
    await injectVerifiedScript(
        audited.qtloader,
        "sri-qtloader-rejected",
    );
    if (
        typeof window.RhythmGameWasmProbe_entry !== "function"
        || typeof window.qtLoad !== "function"
    ) {
        fail("runtime-entry-missing");
    }

    const locateFile = (filename) => {
        if (filename.endsWith(".wasm")) {
            return audited.wasm.url;
        }
        if (filename.endsWith(".aw.js")) {
            return audited.audioWorklet.url;
        }
        if (filename.endsWith(".ww.js")) {
            return audited.wasmWorker.url;
        }
        return new URL(filename, document.baseURI).href;
    };
    const screen = document.querySelector("#screen");
    if (screen === null || document.querySelectorAll("#screen").length !== 1) {
        fail("runtime-screen-contract");
    }
    // This adapter deliberately owns only the single initial Qt container.
    // Dynamic qtAddContainerElement screens are outside the Task 2 contract.
    const qtStyleAdapter = installQtShadowStyleAdapter(screen);
    let instance;
    try {
        instance = await window.qtLoad({
            locateFile,
            onAbort: (text) => {
                qtEventPump?.stop("runtime-abort");
                qtStyleAdapter.restoreAndFail(
                    "runtime-abort",
                    { text: String(text) },
                );
            },
            onExit: (code) => {
                qtEventPump?.stop("runtime-exit");
                qtStyleAdapter.restoreAndFail(
                    "runtime-exit",
                    { code },
                );
            },
            qt: {
                containerElements: [screen],
                entryFunction: window.RhythmGameWasmProbe_entry,
                onExit: (exit) => {
                    qtEventPump?.stop("runtime-exit");
                    qtStyleAdapter.restoreAndFail(
                        "runtime-exit",
                        exit,
                    );
                },
            },
        });
    } catch (error) {
        qtEventPump?.stop("qtLoad-rejection");
        qtStyleAdapter.restore("qtLoad-rejection");
        throw error;
    }
    try {
        await qtStyleAdapter.requireInitialAdoption();
    } catch (error) {
        qtEventPump?.stop("qt-style-initial-rejection");
        qtStyleAdapter.restore("qt-style-initial-rejection");
        throw error;
    }
    try {
        retainRuntimeCommandAuthority(instance);
        qtEventPump = installQtEventPump(instance, screen);
        runtimeCommandIngress = async (name, payload) => {
            if (!runtimeCommandNames.includes(name)) {
                throw new Error(
                    `gate1b-command-not-allowed:${String(name)}`,
                );
            }
            const requiredNegativeMode =
                adversarialRuntimeCommandModes[name] ?? null;
            if (
                requiredNegativeMode !== null
                && negativeMode !== requiredNegativeMode
            ) {
                throw new Error(
                    `gate1b-command-mode:${String(name)}`,
                );
            }
            if (!isPlainObject(payload)) {
                throw new TypeError("gate1b-command-payload");
            }
            if (name === "ack-media-frame-capture") {
                if (
                    Object.keys(payload).sort().join(",")
                        !== "requestIds,runNonce"
                    || !Number.isSafeInteger(payload.runNonce)
                    || payload.runNonce <= 0
                    || payload.runNonce > 0xFFFFFFFE
                    || !Array.isArray(payload.requestIds)
                    || payload.requestIds.length < 1
                    || payload.requestIds.length > 16
                    || payload.requestIds.some(
                        (requestId) => (
                            typeof requestId !== "string"
                            || !/^request-[1-9][0-9]*$/.test(requestId)
                        ),
                    )
                    || new Set(payload.requestIds).size
                        !== payload.requestIds.length
                ) {
                    throw new TypeError(
                        "ack-media-frame-capture-payload",
                    );
                }
            }
            if (
                name === "arm-hidden-timer-probe"
                && document.visibilityState !== "hidden"
            ) {
                throw new Error("hidden-timer-probe-requires-hidden-page");
            }
            if (
                name === "arm-visible-resume-timer-probe"
                && document.visibilityState !== "visible"
            ) {
                throw new Error(
                    "visible-resume-timer-probe-requires-visible-page",
                );
            }
            if (
                retainedRuntimeCommand === null
                || qtEventPump === null
            ) {
                throw new Error("gate1b-runtime-not-ready");
            }
            if (terminalFailure !== null) {
                throw new Error("gate1b-terminal-latched");
            }
            const encodedPayload = JSON.stringify(payload);
            if (runtimeCommandInFlight) {
                throw new Error("gate1b-command-in-flight");
            }
            runtimeCommandInFlight = true;

            try {
                await qtEventPump.whenIdle();
                if (terminalFailure !== null) {
                    throw new Error("gate1b-terminal-latched");
                }
                const terminalProbeCommand = requiredNegativeMode !== null;
                if (terminalProbeCommand) {
                    qtEventPump.quiesceForTerminalProbe();
                }

                let encodedResult;
                try {
                    encodedResult = retainedRuntimeCommand(
                        name,
                        encodedPayload,
                    );
                } catch (error) {
                    fail("runtime-command-boundary-threw", {
                        message: String(error),
                        name,
                    });
                }
                let commandResult;
                try {
                    commandResult =
                        parseRuntimeCommandResult(encodedResult);
                } catch (error) {
                    fail("runtime-command-result-invalid", {
                        message: String(error),
                        name,
                    });
                }
                if (commandResult.ok === false) {
                    fail("runtime-command-failed", {
                        error: commandResult.error,
                        name,
                    });
                }
                const reply = commandResult.reply;
                if (!terminalProbeCommand) {
                    const commandPumpResult = await qtEventPump.kick(
                        "runtime-command",
                    );
                    if (commandPumpResult !== true) {
                        fail("runtime-command-pump-deferred", { name });
                    }
                    if (
                        name === "probe-ping"
                        && (
                            report.snapshot?.checks?.[
                                "qt-application-cycle-order"
                            ]?.passed !== true
                        )
                    ) {
                        fail("runtime-command-cycle-order-missing", { name });
                    }
                }
                if (name === "begin-foreground-latency-sampling") {
                    qtEventPump.armForegroundInputSamples(
                        payload.sampleCount,
                    );
                } else if (name === "arm-bfcache-resume-probe") {
                    qtEventPump.armBfcacheResumeProbe(
                        reply.handlerIndex,
                        reply.runNonce,
                    );
                }
                return cloneAndFreezeJson(reply);
            } finally {
                runtimeCommandInFlight = false;
            }
        };
        const compiledIdentity = await report.command("probe-ping", {});
        requireExactKeys(
            compiledIdentity,
            ["command", "inputBuildId"],
            "runtime-compiled-identity",
        );
        if (
            compiledIdentity.command !== "probe-ping"
            || compiledIdentity.inputBuildId !== manifest.buildId
        ) {
            fail("runtime-compiled-input-digest-mismatch", {
                actual: compiledIdentity.inputBuildId,
                expected: manifest.buildId,
            });
        }
    } catch (error) {
        runtimeCommandIngress = null;
        qtEventPump?.stop("bootstrap-rejection");
        qtStyleAdapter.restore("bootstrap-rejection");
        throw error;
    }
}

bootstrap().catch((error) => {
    qtEventPump?.stop("bootstrap-rejection");
    if (terminalFailure === null) {
        try {
            fail(error?.gate1bCode ?? "bootstrap-unowned-failure", {
                message: String(error),
            });
        } catch {
            // The terminal record already owns this failure.
        }
    }
});
