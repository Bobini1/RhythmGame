import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { expect, test } from "@playwright/test";
import { PNG } from "pngjs";

import { auditLifecycleArguments } from "../lib/chromium-lifecycle-policy.mjs";
import {
    findAllowedConsoleRecordIndex,
} from "../lib/console-policy.mjs";
import {
    launchExternalLifecycleBrowser,
} from "../lib/external-lifecycle-browser.mjs";
import { startProbeServer } from "../server/probe-server.mjs";

const browserDirectory = path.dirname(
    fileURLToPath(new URL("../package.json", import.meta.url)),
);
const repositoryRoot = path.resolve(browserDirectory, "..", "..", "..");
const inputManifestPath = path.join(
    repositoryRoot,
    "tools",
    "wasm-probe",
    "input-manifest.txt",
);
const runtimeDirectory = path.resolve(
    browserDirectory,
    "..",
    "build",
    "wasm-release",
    "runtime",
);
const runtimeLeaf = "tools/wasm-probe/build/wasm-release/runtime";

const EVENT_KEYS = Object.freeze([
    "monotonicMicroseconds",
    "payload",
    "sequence",
    "type",
]);
const EVENT_PUMP_KEYS = Object.freeze([
    "applicationCyclePumpSerial",
    "bfcacheRestores",
    "bfcacheResumePumpSerial",
    "bfcacheSentinelsQueued",
    "calls",
    "commandKicks",
    "exclusiveDeferrals",
    "foregroundInputLatencyMilliseconds",
    "foregroundInputLatencySamples",
    "foregroundTimerPumpSerials",
    "fullCycleDeferrals",
    "hiddenIdleTimers",
    "hiddenQtTimerPumpSerial",
    "hiddenQtTimerSentinels",
    "idleFrames",
    "inFlight",
    "inputKicks",
    "lifecyclePaused",
    "lifecyclePauses",
    "maxConcurrentCalls",
    "maxNativeDispatchDepth",
    "nativeDispatchDepthLimit",
    "nonBubblingInputKicks",
    "reentrantInputCalls",
    "resumedQtTimerPumpSerial",
    "resumedQtTimerSentinels",
    "stopped",
    "visibilityReschedules",
    "windowWakeTargets",
]);
const FRAME_PAYLOAD_KEYS = Object.freeze([
    "capture",
    "captureFrameCount",
    "contextAttributesResult",
    "contextHandle",
    "frameSequence",
    "generation",
    "graphicsApi",
    "majorVersion",
]);
const CYCLE_SUMMARY_KEYS = Object.freeze(["completed", "status"]);
const POST_MAIN_APPLICATION_STATE_KEYS = Object.freeze([
    "applicationFilePathStable",
    "argumentCount",
    "argumentsMatchRetainedCopy",
]);
const SNAPSHOT_KEYS = Object.freeze([
    "authority",
    "capabilities",
    "checks",
    "cycleSummary",
    "failures",
    "phase",
]);
const STYLE_ADOPTION_KEYS = Object.freeze([
    "adoptionCount",
    "bytes",
    "hostId",
    "inlineStyleCount",
    "ruleCount",
    "sha256",
    "stylesheetCount",
]);
const REPORT_FIXED_METHODS = Object.freeze([
    "appendEvent",
    "command",
    "publishSnapshot",
    "rejectReady",
    "resolveReady",
]);
const REPORT_READ_ONLY_FIELDS = Object.freeze([
    "eventPump",
    "events",
    "ready",
    "readyResolution",
    "schemaVersion",
    "snapshot",
    "styleAdoption",
]);
const REPORT_OWN_KEYS = Object.freeze(
    [...REPORT_FIXED_METHODS, ...REPORT_READ_ONLY_FIELDS].sort(),
);
const REPORT_ENUMERABLE_KEYS = Object.freeze([
    "command",
    "eventPump",
    "events",
    "ready",
    "readyResolution",
    "schemaVersion",
    "snapshot",
    "styleAdoption",
]);
const AUTHORITY_FIELDS = Object.freeze([
    "formalGate1EntryAuthorized",
    "gate0Satisfied",
    "gate1Passed",
    "gate1bTechnicalPassed",
    "productionPortAuthorized",
]);
const CAPABILITY_FIELDS = Object.freeze([
    "audioWorklet",
    "crossOriginIsolated",
    "fileSystemAccess",
    "jspiApi",
    "opfs",
    "secureContext",
    "sharedArrayBuffer",
    "webGl2Api",
]);
const CORE_CHECKS = Object.freeze([
    "explicit-pthread",
    "jspi-nested-loop",
    "qt-application-cycle-order",
    "qt-concurrent",
    "qt-exclusive-suspend-guard",
    "qt-render-webgl2",
    "static-library-exception",
]);
const TASK4_CHECKS = Object.freeze([
    "qt-media-clean-teardown",
    "qt-media-natural-end",
    "qt-qnam-same-origin",
    "qt-wss-main-thread",
]);
const JSPI_EVENTS = Object.freeze([
    "jspi-before-exec",
    "jspi-before-import",
    "jspi-promise-resolved",
    "jspi-quit-delivered",
    "jspi-after-exec",
]);
const OWNED_ASYNC_IMPORT = "__asyncjs__rgGate1bAwaitOwnedNonce";
const PTHREAD_NONCE_XOR = 0xa5a55a5a;
const PHASES = Object.freeze([0.20, 0.80]);
const BUTTON_POINT = Object.freeze({ x: 124, y: 316 });
const CAPTURE_RECT = Object.freeze({
    height: 160,
    width: 160,
    x: 440,
    y: 40,
});
const MEDIA_CAPTURE_RECT = Object.freeze({
    height: 135,
    width: 240,
    x: 24,
    y: 24,
});
const SAMPLE_COORDINATES = Object.freeze([
    Object.freeze([20, 20]),
    Object.freeze([80, 80]),
    Object.freeze([140, 140]),
]);
const PIXEL_TOLERANCE = 4;
const EXPECT_TIMEOUT_MS = 30_000;
const CORE_TEST_TIMEOUT_MS = 120_000;
const ADVERSARIAL_MEDIA_CAPTURE_HOLD_MS = 2_500;
const FAILURE_EVENT_TAIL_LENGTH = 8;
const FOREGROUND_INPUT_LATENCY_BUDGET_MS = 8;
const FOREGROUND_INPUT_SAMPLE_COUNT = 64;
const FOREGROUND_TIMER_INTERVAL_MS = 5;
const FOREGROUND_TIMER_LATENESS_BUDGET_MS = 34;
const FOREGROUND_TIMER_SAMPLE_COUNT = 32;
const HIDDEN_TIMER_INTERVAL_MS = 125;
const VISIBLE_RESUME_TIMER_INTERVAL_MS = 125;

test.use({
    colorScheme: "dark",
    deviceScaleFactor: 1,
    ignoreHTTPSErrors: true,
    viewport: { height: 360, width: 640 },
});

test.describe.configure({ mode: "serial" });

let probeServer;
let actualRuntimeBuildId;
let expectedRuntimeBuildId;

async function auditBrowserLifecycleLaunch(browser, projectName) {
    const session = await browser.newBrowserCDPSession();
    let commandLine;
    try {
        commandLine = await session.send("Browser.getBrowserCommandLine");
    } finally {
        await session.detach();
    }
    if (
        !Array.isArray(commandLine.arguments)
        || commandLine.arguments.length === 0
    ) {
        throw new Error("Chromium did not expose its effective command line");
    }
    const audit = auditLifecycleArguments(
        commandLine.arguments.slice(1),
        projectName,
    );
    validateExactKeys(
        audit,
        ["requiredAbsent", "requiredPresent", "verifiedVia"],
        "Chromium lifecycle argument audit",
    );
    expect(audit.requiredPresent).toEqual(["--enable-automation"]);
    expect(audit.verifiedVia).toBe("Browser.getBrowserCommandLine");
    return audit;
}

function sha256(bytes) {
    return createHash("sha256").update(bytes).digest("hex");
}

async function computeCurrentInputBuildId() {
    const manifestBytes = await readFile(inputManifestPath);
    const manifestText = manifestBytes.toString("utf8");
    if (!manifestText.endsWith("\n") || manifestText.includes("\r")) {
        throw new Error("input-manifest.txt must be UTF-8 with LF terminators");
    }
    const relativePaths = manifestText.slice(0, -1).split("\n");
    const caseFoldedPaths = relativePaths.map((entry) => entry.toLowerCase());
    const sortedPaths = [...relativePaths].sort((left, right) => {
        const leftFolded = left.toLowerCase();
        const rightFolded = right.toLowerCase();
        if (leftFolded < rightFolded) {
            return -1;
        }
        if (leftFolded > rightFolded) {
            return 1;
        }
        return 0;
    });
    if (
        relativePaths.length === 0
        || relativePaths.some(
            (entry) => (
                entry.length === 0
                || entry !== entry.trim()
                || path.posix.isAbsolute(entry)
                || entry.includes("\\")
                || entry.split("/").includes("..")
            ),
        )
        || new Set(caseFoldedPaths).size !== relativePaths.length
        || relativePaths.join("\n") !== sortedPaths.join("\n")
    ) {
        throw new Error("input-manifest.txt is not canonical and sorted");
    }

    let digestPayload = `manifest=${sha256(manifestBytes)}\n`;
    for (const relativePath of relativePaths) {
        const inputBytes = await readFile(
            path.resolve(repositoryRoot, ...relativePath.split("/")),
        );
        digestPayload += `${relativePath}=${sha256(inputBytes)}\n`;
    }
    return sha256(Buffer.from(digestPayload, "utf8"));
}

test.beforeAll(async () => {
    expectedRuntimeBuildId = await computeCurrentInputBuildId();
    const runtimeManifest = JSON.parse(
        await readFile(
            path.join(runtimeDirectory, "runtime-artifacts.json"),
            "utf8",
        ),
    );
    actualRuntimeBuildId = runtimeManifest.buildId;
    if (actualRuntimeBuildId !== expectedRuntimeBuildId) {
        throw new Error(
            "stale Gate 1B runtime: expected current input buildId "
            + `${expectedRuntimeBuildId}, got ${String(actualRuntimeBuildId)}`,
        );
    }
    probeServer = await startProbeServer({
        runtimeDirectory,
    });
});

test.afterAll(async () => {
    if (probeServer) {
        await probeServer.close();
    }
});

function expectedRgba(phase) {
    return [
        Math.round((0.10 + 0.50 * phase) * 255),
        Math.round((0.20 + 0.25 * phase) * 255),
        Math.round((0.80 - 0.50 * phase) * 255),
        Math.round(1.00 * 255),
    ];
}

function validateExactKeys(value, expected, description) {
    expect(value, description).not.toBeNull();
    expect(Array.isArray(value), description).toBe(false);
    expect(typeof value, description).toBe("object");
    expect(Object.keys(value).sort(), description).toEqual(
        [...expected].sort(),
    );
}

function validateObject(value, description) {
    expect(value, description).not.toBeNull();
    expect(Array.isArray(value), description).toBe(false);
    expect(typeof value, description).toBe("object");
}

function validateEventStream(events) {
    expect(Array.isArray(events)).toBe(true);
    expect(events.length).toBeGreaterThan(0);
    let previousMicros = -1;
    for (const [index, event] of events.entries()) {
        validateExactKeys(event, EVENT_KEYS, `event ${index}`);
        expect(event.sequence, `event ${index} sequence`).toBe(index);
        expect(
            Number.isSafeInteger(event.monotonicMicroseconds),
            `event ${index} monotonic time`,
        ).toBe(true);
        expect(event.monotonicMicroseconds).toBeGreaterThanOrEqual(0);
        expect(event.monotonicMicroseconds).toBeGreaterThan(
            previousMicros,
        );
        expect(event.type).toMatch(
            /^[a-z][a-z0-9]*(?:-[a-z0-9]+)*$/,
        );
        validateObject(event.payload, `event ${index} payload`);
        previousMicros = event.monotonicMicroseconds;
    }
}

function validateNoPrivateMaterial(value) {
    const encoded = JSON.stringify(value);
    expect(encoded).not.toMatch(/[A-Za-z]:[\\/]/);
    expect(encoded).not.toMatch(/-----BEGIN [A-Z ]*PRIVATE KEY-----/);
    expect(encoded).not.toContain(".profiles/");
    expect(encoded).not.toContain("certificatePrivateKey");
    expect(encoded).not.toContain("environmentDump");
    expect(encoded).not.toContain("profilePath");
    expect(encoded).not.toContain("selectedDirectoryPath");
}

function validateSnapshot(snapshot) {
    validateExactKeys(snapshot, SNAPSHOT_KEYS, "Gate 1B snapshot");
    validateExactKeys(
        snapshot.authority,
        AUTHORITY_FIELDS,
        "Gate 1B authority",
    );
    for (const field of AUTHORITY_FIELDS) {
        expect(
            snapshot.authority[field],
            `Task 3 has no authority to set ${field}`,
        ).toBe(false);
    }
    validateExactKeys(
        snapshot.capabilities,
        CAPABILITY_FIELDS,
        "browser capabilities",
    );
    for (const field of CAPABILITY_FIELDS) {
        expect(snapshot.capabilities[field], `capability ${field}`).toBe(true);
    }
    expect(snapshot.failures).toEqual([]);
    validateExactKeys(
        snapshot.cycleSummary,
        CYCLE_SUMMARY_KEYS,
        "Gate 1B cycle summary",
    );
    expect(snapshot.cycleSummary.status).toBe("not-started");
    expect(snapshot.cycleSummary.completed).toBe(0);
    expect(snapshot.phase).not.toMatch(/pass|passed|qualified/i);
    validateNoPrivateMaterial(snapshot);
}

function checkDetail(snapshot, checkName) {
    const check = snapshot.checks[checkName];
    validateExactKeys(
        check,
        ["detail", "passed"],
        `check ${checkName}`,
    );
    expect(check.passed, `check ${checkName}`).toBe(true);
    validateObject(check.detail, `check ${checkName} detail`);
    return check.detail;
}

function validateStyleAdoption(styleAdoption) {
    validateExactKeys(
        styleAdoption,
        STYLE_ADOPTION_KEYS,
        "Qt constructable stylesheet adoption",
    );
    expect(styleAdoption).toEqual({
        adoptionCount: 1,
        bytes: 5_238,
        hostId: "qt-shadow-container",
        inlineStyleCount: 0,
        ruleCount: 37,
        sha256:
            "6b7168686da79590ea116889998716dfa"
            + "624e1467411daa2bffee066a867d53e",
        stylesheetCount: 1,
    });
}

function nearestRankP95(samples) {
    expect(samples.length).toBeGreaterThan(0);
    const sorted = [...samples].sort((left, right) => left - right);
    return sorted[Math.max(0, Math.ceil(sorted.length * 0.95) - 1)];
}

function validateEventPump(
    eventPump,
    { requireHiddenFallback = false } = {},
) {
    validateExactKeys(
        eventPump,
        EVENT_PUMP_KEYS,
        "Qt event pump",
    );
    expect(eventPump.calls).toBeGreaterThan(0);
    expect(eventPump.applicationCyclePumpSerial).toBeGreaterThan(0);
    expect(eventPump.commandKicks).toBeGreaterThan(0);
    expect(eventPump.exclusiveDeferrals).toBeGreaterThanOrEqual(1);
    expect(eventPump.foregroundInputLatencySamples).toBe(
        FOREGROUND_INPUT_SAMPLE_COUNT,
    );
    expect(eventPump.foregroundInputLatencyMilliseconds).toHaveLength(
        FOREGROUND_INPUT_SAMPLE_COUNT,
    );
    expect(eventPump.foregroundTimerPumpSerials).toHaveLength(
        FOREGROUND_TIMER_SAMPLE_COUNT,
    );
    for (const serial of eventPump.foregroundTimerPumpSerials) {
        expect(Number.isSafeInteger(serial)).toBe(true);
        expect(serial).toBeGreaterThan(0);
    }
    for (const latency of eventPump.foregroundInputLatencyMilliseconds) {
        expect(Number.isFinite(latency)).toBe(true);
        expect(latency).toBeGreaterThanOrEqual(0);
    }
    expect(nearestRankP95(
        eventPump.foregroundInputLatencyMilliseconds,
    )).toBeLessThanOrEqual(FOREGROUND_INPUT_LATENCY_BUDGET_MS);
    expect(Number.isSafeInteger(eventPump.fullCycleDeferrals)).toBe(true);
    expect(eventPump.fullCycleDeferrals).toBeGreaterThanOrEqual(0);
    expect(eventPump.idleFrames).toBeGreaterThan(0);
    expect(eventPump.inputKicks).toBeGreaterThan(0);
    // The workload dispatches real DOM input while the full promising pump is
    // suspended; one full owner plus four synchronous native levels is valid.
    expect(eventPump.maxConcurrentCalls).toBeGreaterThanOrEqual(2);
    expect(eventPump.maxNativeDispatchDepth).toBe(4);
    expect(eventPump.nativeDispatchDepthLimit).toBe(4);
    expect(eventPump.maxNativeDispatchDepth).toBeLessThanOrEqual(
        eventPump.nativeDispatchDepthLimit,
    );
    expect(eventPump.maxConcurrentCalls).toBeLessThanOrEqual(
        eventPump.nativeDispatchDepthLimit + 1,
    );
    expect(eventPump.nonBubblingInputKicks).toBeGreaterThanOrEqual(2);
    expect(eventPump.reentrantInputCalls).toBeGreaterThan(0);
    expect(eventPump.bfcacheRestores).toBeGreaterThanOrEqual(1);
    expect(eventPump.bfcacheResumePumpSerial).toBeGreaterThan(0);
    expect(eventPump.lifecyclePaused).toBe(false);
    expect(eventPump.lifecyclePauses).toBeGreaterThanOrEqual(1);
    expect(eventPump.stopped).toBe(false);
    if (requireHiddenFallback) {
        expect(eventPump.hiddenIdleTimers).toBeGreaterThanOrEqual(1);
        expect(eventPump.hiddenQtTimerPumpSerial).toBeGreaterThan(0);
        expect(eventPump.hiddenQtTimerSentinels).toBe(1);
        expect(eventPump.resumedQtTimerPumpSerial).toBeGreaterThan(0);
        expect(eventPump.resumedQtTimerSentinels).toBe(1);
        expect(eventPump.visibilityReschedules).toBeGreaterThanOrEqual(2);
    } else {
        expect(eventPump.hiddenIdleTimers).toBeGreaterThanOrEqual(0);
        expect(eventPump.hiddenQtTimerPumpSerial).toBe(0);
        expect(eventPump.hiddenQtTimerSentinels).toBe(0);
        expect(eventPump.resumedQtTimerPumpSerial).toBe(0);
        expect(eventPump.resumedQtTimerSentinels).toBe(0);
        expect(eventPump.visibilityReschedules).toBeGreaterThanOrEqual(0);
    }
    expect(eventPump.windowWakeTargets).toBe(1);
    expect(typeof eventPump.inFlight).toBe("boolean");
}

function eventIndex(events, type, predicate = () => true) {
    return events.findIndex(
        (event) => event.type === type && predicate(event.payload),
    );
}

function validateRenderFrames(
    events,
    {
        capture,
        eventSequenceBaseline,
        frameSequenceBaseline,
        generation,
    },
) {
    const frames = events.filter(
        (event) => (
            event.type === "qt-render-frame"
            && event.payload.capture === capture
            && event.payload.generation === generation
            && event.payload.frameSequence > frameSequenceBaseline
        ),
    );
    expect(frames, `${capture} generation ${generation} frame count`)
        .toHaveLength(2);
    expect(frames.map((event) => event.payload.captureFrameCount))
        .toEqual([1, 2]);
    expect(frames[0].sequence).toBeGreaterThan(eventSequenceBaseline);
    expect(frames[1].sequence).toBeGreaterThan(frames[0].sequence);

    let previousFrameSequence = frameSequenceBaseline;
    for (const [index, event] of frames.entries()) {
        const payload = event.payload;
        validateExactKeys(
            payload,
            FRAME_PAYLOAD_KEYS,
            `${capture} generation ${generation} frame ${index}`,
        );
        expect(Number.isSafeInteger(payload.contextHandle)).toBe(true);
        expect(payload.contextHandle).toBeGreaterThan(0);
        expect(Number.isSafeInteger(payload.frameSequence)).toBe(true);
        expect(payload.frameSequence).toBeGreaterThan(previousFrameSequence);
        expect(Number.isSafeInteger(payload.generation)).toBe(true);
        expect(payload.generation).toBeGreaterThanOrEqual(0);
        expect(payload.graphicsApi).toBe("OpenGL");
        expect(payload.contextAttributesResult).toBe(0);
        expect(payload.majorVersion).toBe(2);
        previousFrameSequence = payload.frameSequence;
    }
    for (const field of [
        "contextAttributesResult",
        "contextHandle",
        "graphicsApi",
        "majorVersion",
    ]) {
        expect(frames[1].payload[field]).toBe(frames[0].payload[field]);
    }
    return frames;
}

function validateJspiEvents(events, postMainIndex, jspi) {
    const observed = events.filter(
        (event) => JSPI_EVENTS.includes(event.type),
    );
    expect(observed.map((event) => event.type)).toEqual(JSPI_EVENTS);
    for (const event of observed) {
        expect(event.sequence).toBeGreaterThan(postMainIndex);
    }
    validateExactKeys(
        observed[0].payload,
        ["requestedNonce"],
        "jspi-before-exec payload",
    );
    validateExactKeys(
        observed[1].payload,
        ["requestedNonce"],
        "jspi-before-import payload",
    );
    validateExactKeys(
        observed[2].payload,
        ["resolvedNonce"],
        "jspi-promise-resolved payload",
    );
    validateExactKeys(
        observed[3].payload,
        ["requestedNonceMatches"],
        "jspi-quit-delivered payload",
    );
    validateExactKeys(
        observed[4].payload,
        [
            "elapsedMicroseconds",
            "postLoopSentinel",
            "requestedNonce",
            "resolvedNonce",
        ],
        "jspi-after-exec payload",
    );
    expect(observed[0].payload.requestedNonce)
        .toBe(jspi.requestedNonce);
    expect(observed[1].payload.requestedNonce)
        .toBe(jspi.requestedNonce);
    expect(observed[2].payload.resolvedNonce)
        .toBe(jspi.requestedNonce);
    expect(observed[3].payload.requestedNonceMatches).toBe(true);
    expect(observed[4].payload).toEqual({
        elapsedMicroseconds: jspi.elapsedMicroseconds,
        postLoopSentinel: true,
        requestedNonce: jspi.requestedNonce,
        resolvedNonce: jspi.requestedNonce,
    });
    return observed;
}

function validatePostMainApplicationState(events, postMainIndex) {
    const applicationEvents = events.filter(
        (event) => event.type === "post-main-application-state",
    );
    expect(applicationEvents).toHaveLength(1);
    expect(applicationEvents[0].sequence).toBeGreaterThan(postMainIndex);
    validateExactKeys(
        applicationEvents[0].payload,
        POST_MAIN_APPLICATION_STATE_KEYS,
        "post-main Qt application state",
    );
    expect(applicationEvents[0].payload.applicationFilePathStable)
        .toBe(true);
    expect(applicationEvents[0].payload.argumentsMatchRetainedCopy)
        .toBe(true);
    expect(
        Number.isSafeInteger(applicationEvents[0].payload.argumentCount),
    ).toBe(true);
    expect(applicationEvents[0].payload.argumentCount).toBeGreaterThan(0);
}

async function readReport(page) {
    return page.evaluate(() => {
        const report = globalThis.__rhythmGameGate1b;
        return {
            eventPump: structuredClone(report.eventPump),
            events: structuredClone([...report.events]),
            instanceAbsent: !Object.hasOwn(report, "instance"),
            schemaVersion: report.schemaVersion,
            snapshot: structuredClone(report.snapshot),
            styleAdoption: structuredClone(report.styleAdoption),
        };
    });
}

async function inspectReadOnlyReportAuthority(page) {
    return page.evaluate(
        ({
            enumerableKeys,
            fixedMethods,
            ownKeys,
            readOnlyFields,
        }) => {
            const report = globalThis.__rhythmGameGate1b;
            const reportReference = report;
            const eventsReference = report.events;
            const eventCount = eventsReference.length;
            const firstEvent = eventsReference[0];
            const readyReference = report.ready;
            const readyResolutionReference = report.readyResolution;
            const snapshotReference = report.snapshot;
            const eventPumpReference = report.eventPump;
            const styleAdoptionReference = report.styleAdoption;
            const isDeepFrozen = (value) => {
                if (
                    value === null
                    || (typeof value !== "object" && typeof value !== "function")
                ) {
                    return true;
                }
                return (
                    Object.isFrozen(value)
                    && Reflect.ownKeys(value).every(
                        (key) => isDeepFrozen(value[key]),
                    )
                );
            };

            let eventPushRejected = false;
            try {
                eventsReference.push({});
            } catch {
                eventPushRejected = true;
            }
            let eventPreventExtensionsRejected = false;
            try {
                Object.preventExtensions(eventsReference);
            } catch {
                eventPreventExtensionsRejected = true;
            }

            const globalDescriptor = Object.getOwnPropertyDescriptor(
                globalThis,
                "__rhythmGameGate1b",
            );
            const methodDescriptorsAreFixed = fixedMethods.every((name) => {
                const descriptor = Object.getOwnPropertyDescriptor(
                    report,
                    name,
                );
                return (
                    descriptor?.configurable === false
                    && typeof descriptor.value === "function"
                    && descriptor.writable === false
                    && descriptor.enumerable === (name === "command")
                );
            });
            const fieldsAreReadOnly = readOnlyFields.every((name) => {
                const descriptor = Object.getOwnPropertyDescriptor(
                    report,
                    name,
                );
                return (
                    descriptor?.configurable === false
                    && descriptor.enumerable === true
                    && (
                        (
                            typeof descriptor.get === "function"
                            && descriptor.set === undefined
                            && !["ready", "schemaVersion"].includes(name)
                        )
                        || (
                            Object.hasOwn(descriptor, "value")
                            && descriptor.writable === false
                            && ["ready", "schemaVersion"].includes(name)
                        )
                    )
                );
            });
            const replacement = Object.freeze({});
            const mutationResults = {
                deleteEvent: Reflect.deleteProperty(
                    eventsReference,
                    "0",
                ),
                deleteGlobal: Reflect.deleteProperty(
                    globalThis,
                    "__rhythmGameGate1b",
                ),
                deleteSnapshot: Reflect.deleteProperty(
                    report,
                    "snapshot",
                ),
                replaceEvent: Reflect.set(
                    eventsReference,
                    "0",
                    replacement,
                ),
                replaceEvents: Reflect.set(
                    report,
                    "events",
                    replacement,
                ),
                replaceEventPump: Reflect.set(
                    report,
                    "eventPump",
                    replacement,
                ),
                replaceGlobal: Reflect.set(
                    globalThis,
                    "__rhythmGameGate1b",
                    replacement,
                ),
                addInstance: Reflect.set(
                    report,
                    "instance",
                    replacement,
                ),
                replaceReadyResolution: Reflect.set(
                    report,
                    "readyResolution",
                    replacement,
                ),
                replaceSnapshot: Reflect.set(
                    report,
                    "snapshot",
                    replacement,
                ),
                replaceStyleAdoption: Reflect.set(
                    report,
                    "styleAdoption",
                    replacement,
                ),
            };
            const eventPumpReportDescriptor = Object.getOwnPropertyDescriptor(
                report,
                "eventPump",
            );
            return {
                eventEntriesAreDeepFrozen: eventsReference.every(
                    isDeepFrozen,
                ),
                eventLogUnchanged: (
                    report.events === eventsReference
                    && report.events.length === eventCount
                    && report.events[0] === firstEvent
                ),
                eventPreventExtensionsRejected,
                eventPushRejected,
                eventViewIsArray: Array.isArray(eventsReference),
                eventPumpDescriptorIsGetter: (
                    eventPumpReportDescriptor?.configurable === false
                    && typeof eventPumpReportDescriptor.get === "function"
                    && eventPumpReportDescriptor.set === undefined
                    && !Object.hasOwn(eventPumpReportDescriptor, "value")
                ),
                eventPumpIsDeepFrozen: isDeepFrozen(eventPumpReference),
                fieldsAreReadOnly,
                globalIsFixed: (
                    globalDescriptor?.configurable === false
                    && globalDescriptor.enumerable === false
                    && globalDescriptor.value === report
                    && globalDescriptor.writable === false
                ),
                instanceAbsent: !Object.hasOwn(report, "instance"),
                methodDescriptorsAreFixed,
                mutationResults,
                nestedStateUnchanged: (
                    report.ready === readyReference
                    && report.readyResolution === readyResolutionReference
                    && report.snapshot === snapshotReference
                    && report.styleAdoption === styleAdoptionReference
                ),
                reportIsFrozen: Object.isFrozen(report),
                reportEnumerableKeysAreExact: (
                    Object.keys(report).sort().join("\n")
                    === enumerableKeys.join("\n")
                ),
                reportOwnKeysAreExact: (
                    Reflect.ownKeys(report).sort().join("\n")
                    === ownKeys.join("\n")
                ),
                readyIsPromise: readyReference instanceof Promise,
                readyResolutionIsDeepFrozen: isDeepFrozen(
                    readyResolutionReference,
                ),
                reportReferenceUnchanged: (
                    globalThis.__rhythmGameGate1b === reportReference
                ),
                schemaVersion: report.schemaVersion,
                snapshotIsDeepFrozen: isDeepFrozen(snapshotReference),
                styleAdoptionIsDeepFrozen: isDeepFrozen(
                    styleAdoptionReference,
                ),
            };
        },
        {
            enumerableKeys: REPORT_ENUMERABLE_KEYS,
            fixedMethods: REPORT_FIXED_METHODS,
            ownKeys: REPORT_OWN_KEYS,
            readOnlyFields: REPORT_READ_ONLY_FIELDS,
        },
    );
}

async function verifyRetainedRuntimeCommand(page) {
    return page.evaluate(async () => {
        const report = globalThis.__rhythmGameGate1b;
        const reply = await report.command("probe-ping", {});
        return {
            command: reply.command,
            inputBuildId: reply.inputBuildId,
            instanceAbsent: !Object.hasOwn(report, "instance"),
        };
    });
}

function createDiagnosticFailureLatch() {
    let firstFailure;
    let resolveFailure;
    const promise = new Promise((resolve) => {
        resolveFailure = resolve;
    });
    return Object.freeze({
        fail(channel, record) {
            if (firstFailure !== undefined) {
                return;
            }
            firstFailure = Object.freeze({
                channel,
                record: structuredClone(record),
            });
            resolveFailure(firstFailure);
        },
        promise,
    });
}

async function compactEventTail(page) {
    try {
        return await page.evaluate((eventTailLength) => {
            const events = globalThis.__rhythmGameGate1b?.events;
            return Array.isArray(events)
                ? [...events].slice(-eventTailLength).map((event) => ({
                    payload: event?.payload,
                    sequence: event?.sequence,
                    type: event?.type,
                }))
                : [];
        }, FAILURE_EVENT_TAIL_LENGTH);
    } catch (error) {
        return [{
            payload: { message: String(error) },
            sequence: -1,
            type: "event-tail-unavailable",
        }];
    }
}

async function waitForReportOutcome(
    page,
    description,
    criteria,
    diagnosticFailure,
) {
    const pageOutcome = (async () => {
        const outcomeHandle = await page.waitForFunction(
            ({ eventTailLength, requested }) => {
            const report = globalThis.__rhythmGameGate1b;
            if (report === undefined) {
                return undefined;
            }
            const events = Array.isArray(report.events)
                ? report.events
                : [];
            const terminalFailure = events.find(
                (event) => event?.type === "terminal-failure",
            );
            const snapshotFailure = Array.isArray(
                report.snapshot?.failures,
            )
                ? report.snapshot.failures[0]
                : undefined;
            if (
                requested.kind === "ready"
                && globalThis.__rhythmGameGate1bReadyObservation
                    === undefined
            ) {
                const observation = {
                    failure: undefined,
                    resolvedValue: undefined,
                    resolvedValueIsDeepFrozen: false,
                    state: "pending",
                };
                Object.defineProperty(
                    globalThis,
                    "__rhythmGameGate1bReadyObservation",
                    {
                        configurable: false,
                        enumerable: false,
                        value: observation,
                        writable: false,
                    },
                );
                Promise.resolve(report.ready).then(
                    (resolvedValue) => {
                        const isDeepFrozen = (value) => {
                            if (
                                value === null
                                || (
                                    typeof value !== "object"
                                    && typeof value !== "function"
                                )
                            ) {
                                return true;
                            }
                            return (
                                Object.isFrozen(value)
                                && Reflect.ownKeys(value).every(
                                    (key) => isDeepFrozen(value[key]),
                                )
                            );
                        };
                        observation.resolvedValue = structuredClone(
                            resolvedValue,
                        );
                        observation.resolvedValueIsDeepFrozen =
                            isDeepFrozen(resolvedValue);
                        observation.state = "success";
                    },
                    (failure) => {
                        observation.failure = failure;
                        observation.state = "failure";
                    },
                );
            }
            const readyObservation = (
                globalThis.__rhythmGameGate1bReadyObservation
            );
            const failure = (
                terminalFailure?.payload
                ?? snapshotFailure
                ?? (
                    requested.kind === "ready"
                        && readyObservation?.state === "failure"
                        ? readyObservation.failure
                        : undefined
                )
            );
            const eventTail = events.slice(-eventTailLength).map((event) => ({
                payload: event?.payload,
                sequence: event?.sequence,
                type: event?.type,
            }));
            if (failure !== undefined) {
                return {
                    eventTail,
                    failure,
                    state: "failure",
                };
            }

            let succeeded = false;
            switch (requested.kind) {
            case "ready":
                succeeded = (
                    readyObservation?.state === "success"
                    && report.readyResolution !== null
                );
                break;
            case "core-checks":
                succeeded = requested.checks.every(
                    (name) => (
                        report.snapshot?.checks?.[name]?.passed === true
                    ),
                );
                break;
            case "jspi-suspended":
                succeeded = (
                    events.filter(
                        (event) => event.type === "jspi-before-import",
                    ).length === 1
                    && events.every(
                        (event) => event.type !== "jspi-promise-resolved",
                    )
                    && report.eventPump?.inFlight === true
                );
                break;
            case "pump-idle":
                succeeded = report.eventPump?.inFlight === false;
                break;
            case "render-generation":
            {
                const matchingFrames = events.filter(
                    (event) => (
                        event.type === "qt-render-frame"
                        && event.payload.capture === "phase"
                        && event.payload.generation
                            === requested.generation
                        && event.payload.frameSequence
                            > requested.frameBaseline
                    ),
                );
                succeeded = (
                    matchingFrames.length >= 2
                    && matchingFrames[0].payload.captureFrameCount === 1
                    && matchingFrames[1].payload.captureFrameCount === 2
                    && matchingFrames[1].payload.frameSequence
                        > matchingFrames[0].payload.frameSequence
                    && new Set(
                        matchingFrames.map(
                            (event) => event.payload.frameSequence,
                        ),
                    ).size === matchingFrames.length
                );
                break;
            }
            case "user-activation":
            {
                const activationEvents = events.filter(
                    (event) => (
                        event.type === "user-activation-sampled"
                        && event.sequence > requested.sequenceBaseline
                        && Object.keys(event.payload).length === 1
                        && Object.hasOwn(event.payload, "active")
                        && event.payload.active === true
                    ),
                );
                succeeded = activationEvents.length === 1;
                break;
            }
            case "media-device-browser-enumeration":
            {
                const scenario =
                    globalThis.__gate1bMediaDeviceScenario;
                succeeded = (
                    typeof scenario?.snapshot === "function"
                    && scenario.snapshot().enumerateResolvedCount
                        === requested.resolvedCount
                );
                break;
            }
            case "media-device-settled":
            {
                const snapshots = events.filter(
                    (event) => (
                        event.type === "qt-media-device-snapshot"
                    ),
                );
                const settlements = events.filter(
                    (event) => (
                        event.type === "qt-media-device-batch-settled"
                        && event.sequence > requested.sequenceBaseline
                    ),
                );
                const latest = settlements.at(-1);
                const inputHashes = new Set(
                    latest?.payload?.audioInputs?.map(
                        (device) => device.idSha256,
                    ) ?? [],
                );
                const outputHashes = new Set(
                    latest?.payload?.audioOutputs?.map(
                        (device) => device.idSha256,
                    ) ?? [],
                );
                succeeded = (
                    latest !== undefined
                    && latest.payload.snapshotOrdinal === snapshots.length
                    && latest.payload.audioInputSignalCount
                        >= requested.minimumAudioInputSignalCount
                    && latest.payload.audioOutputSignalCount
                        >= requested.minimumAudioOutputSignalCount
                    && latest.payload.audioOutputs.length
                        >= requested.minimumAudioOutputs
                    && requested.requiredInputHashes.every(
                        (hash) => inputHashes.has(hash),
                    )
                    && requested.requiredOutputHashes.every(
                        (hash) => outputHashes.has(hash),
                    )
                    && requested.forbiddenInputHashes.every(
                        (hash) => !inputHashes.has(hash),
                    )
                    && requested.forbiddenOutputHashes.every(
                        (hash) => !outputHashes.has(hash),
                    )
                );
                break;
            }
            default:
                throw new Error(
                    `gate1b-wait-kind-not-allowed:${requested.kind}`,
                );
            }
            return succeeded
                ? {
                    eventTail,
                    readyObservation: requested.kind === "ready"
                        ? structuredClone(readyObservation)
                        : undefined,
                    state: "success",
                }
                : undefined;
            },
            {
                eventTailLength: FAILURE_EVENT_TAIL_LENGTH,
                requested: criteria,
            },
            {
                polling: "raf",
                timeout: EXPECT_TIMEOUT_MS,
            },
        );
        try {
            return await outcomeHandle.jsonValue();
        } finally {
            await outcomeHandle.dispose();
        }
    })().catch(async (error) => ({
        eventTail: await compactEventTail(page),
        failure: {
            message: String(error),
        },
        state: "wait-error",
    }));
    const diagnosticOutcome = diagnosticFailure.promise.then(
        async (diagnostic) => ({
            diagnostic,
            eventTail: await compactEventTail(page),
            state: "diagnostic-failure",
        }),
    );
    const outcome = await Promise.race([pageOutcome, diagnosticOutcome]);
    if (
        outcome.state === "failure"
        || outcome.state === "wait-error"
        || outcome.state === "diagnostic-failure"
    ) {
        const frozenFailure = Object.freeze(
            structuredClone(outcome.failure ?? outcome.diagnostic),
        );
        const frozenEventTail = Object.freeze(
            structuredClone(outcome.eventTail),
        );
        throw new Error(
            `${description}: Task 3 runtime/diagnostic failure `
            + `${JSON.stringify(frozenFailure)}; compact event tail `
            + JSON.stringify(frozenEventTail),
        );
    }
    return outcome;
}

async function waitForCoreChecks(page, diagnosticFailure) {
    await waitForReportOutcome(
        page,
        "Task 3 core checks did not complete",
        {
            checks: CORE_CHECKS,
            kind: "core-checks",
        },
        diagnosticFailure,
    );
}

async function waitForTask4Checks(page, diagnosticFailure) {
    await waitForReportOutcome(
        page,
        "Task 4 network or media checks did not complete",
        {
            checks: TASK4_CHECKS,
            kind: "core-checks",
        },
        diagnosticFailure,
    );
}

async function captureAndAcknowledgeMedia(
    page,
    testInfo,
    diagnosticFailure,
    { holdMilliseconds = 0 } = {},
) {
    const captureReadyHandle = await page.waitForFunction(
        () => {
            const report = globalThis.__rhythmGameGate1b;
            const terminal = report.events.find(
                (event) => event.type === "terminal-failure",
            );
            if (terminal !== undefined) {
                return { state: "failure", terminal };
            }
            const event = report.events.find(
                (candidate) => (
                    candidate.type === "qt-media-capture-ready"
                ),
            );
            return event === undefined
                ? undefined
                : { event, state: "ready" };
        },
        undefined,
        { polling: "raf", timeout: EXPECT_TIMEOUT_MS },
    );
    let captureReady;
    try {
        captureReady = await captureReadyHandle.jsonValue();
    } finally {
        await captureReadyHandle.dispose();
    }
    if (captureReady.state !== "ready") {
        throw new Error(
            "media reached terminal failure before visual capture: "
            + JSON.stringify(captureReady.terminal),
        );
    }
    const { runNonce } = captureReady.event.payload;
    expect(Number.isSafeInteger(runNonce)).toBe(true);
    expect(runNonce).toBeGreaterThan(0);
    validateExactKeys(
        captureReady.event.payload,
        [
            "framePositionSamples",
            "paused",
            "positionMilliseconds",
            "runNonce",
        ],
        "paused media capture readiness",
    );
    expect(captureReady.event.payload.paused).toBe(true);
    expect(captureReady.event.payload.framePositionSamples.length)
        .toBeGreaterThanOrEqual(2);
    expect(captureReady.event.payload.positionMilliseconds)
        .toBeGreaterThanOrEqual(500);

    const mediaLogs = probeServer.probeLogs.filter(
        (entry) => entry.kind === "media"
            && entry.runNonce === runNonce
            && [200, 206].includes(entry.status),
    );
    expect(mediaLogs.length).toBeGreaterThanOrEqual(1);
    const requestIds = [...new Set(
        mediaLogs.map((entry) => entry.requestId),
    )];
    expect(requestIds).toHaveLength(mediaLogs.length);

    const observeHeldMedia = async () => page.evaluate((nonce) => {
        const source = new URL(
            `/fixtures/probe.webm?nonce=${nonce}`,
            location.href,
        ).href;
        const matching = [...document.querySelectorAll("video")].filter(
            (element) => (
                element.src === source
                || element.currentSrc === source
            ),
        );
        const element = matching[0] ?? null;
        return {
            currentTimeMilliseconds: element === null
                ? -1
                : element.currentTime * 1000,
            durationMilliseconds: element === null
                ? -1
                : element.duration * 1000,
            ended: element?.ended ?? true,
            matchingElementCount: matching.length,
            paused: element?.paused ?? false,
            readyState: element?.readyState ?? -1,
            terminalFailureCount:
                globalThis.__rhythmGameGate1b.events.filter(
                    (event) => event.type === "terminal-failure",
                ).length,
        };
    }, runNonce);
    let holdObservation = null;
    if (holdMilliseconds > 0) {
        const before = await observeHeldMedia();
        const holdStartedAt = Date.now();
        await page.evaluate(
            (milliseconds) => new Promise(
                (resolve) => setTimeout(resolve, milliseconds),
            ),
            holdMilliseconds,
        );
        const elapsedMilliseconds = Date.now() - holdStartedAt;
        const after = await observeHeldMedia();
        for (const observation of [before, after]) {
            validateExactKeys(
                observation,
                [
                    "currentTimeMilliseconds",
                    "durationMilliseconds",
                    "ended",
                    "matchingElementCount",
                    "paused",
                    "readyState",
                    "terminalFailureCount",
                ],
                "adversarial held media observation",
            );
            expect(observation.matchingElementCount).toBe(1);
            expect(Number.isFinite(observation.durationMilliseconds))
                .toBe(true);
            expect(observation.durationMilliseconds).toBeGreaterThan(0);
            expect(observation.paused).toBe(true);
            expect(observation.ended).toBe(false);
            expect(observation.readyState).toBeGreaterThanOrEqual(2);
            expect(observation.terminalFailureCount).toBe(0);
        }
        expect(elapsedMilliseconds).toBeGreaterThanOrEqual(
            holdMilliseconds,
        );
        expect(holdMilliseconds).toBeGreaterThan(
            before.durationMilliseconds,
        );
        expect(holdMilliseconds).toBeGreaterThan(
            before.durationMilliseconds
                - before.currentTimeMilliseconds,
        );
        expect(after.durationMilliseconds).toBe(
            before.durationMilliseconds,
        );
        expect(Math.abs(
            after.currentTimeMilliseconds
                - before.currentTimeMilliseconds,
        )).toBeLessThanOrEqual(50);
        holdObservation = {
            after,
            before,
            elapsedMilliseconds,
            requestedMilliseconds: holdMilliseconds,
        };
    }

    const screenshot = await page.screenshot({
        animations: "disabled",
        clip: MEDIA_CAPTURE_RECT,
        type: "png",
    });
    const png = PNG.sync.read(screenshot);
    expect(png.width).toBe(MEDIA_CAPTURE_RECT.width);
    expect(png.height).toBe(MEDIA_CAPTURE_RECT.height);
    let nonBlackPixels = 0;
    const colors = new Set();
    for (let offset = 0; offset < png.data.length; offset += 4) {
        const red = png.data[offset];
        const green = png.data[offset + 1];
        const blue = png.data[offset + 2];
        if (red > 8 || green > 8 || blue > 8) {
            ++nonBlackPixels;
        }
        colors.add(`${red >> 4}:${green >> 4}:${blue >> 4}`);
    }
    expect(nonBlackPixels).toBeGreaterThan(
        Math.floor(png.width * png.height * 0.10),
    );
    expect(colors.size).toBeGreaterThan(4);
    if (testInfo !== null) {
        await testInfo.attach("gate1b-media-video-output", {
            body: screenshot,
            contentType: "image/png",
        });
    }

    const acknowledgement = await page.evaluate(
        async (payload) => globalThis.__rhythmGameGate1b.command(
            "ack-media-frame-capture",
            payload,
        ),
        { requestIds, runNonce },
    );
    validateExactKeys(
        acknowledgement,
        [
            "command",
            "elementId",
            "preSeekPositionMilliseconds",
            "requestIds",
            "requestMonotonicMilliseconds",
            "runNonce",
            "seekPositionMilliseconds",
        ],
        "media capture acknowledgement",
    );
    expect(acknowledgement).toEqual({
        command: "ack-media-frame-capture",
        elementId: acknowledgement.elementId,
        preSeekPositionMilliseconds:
            acknowledgement.preSeekPositionMilliseconds,
        requestMonotonicMilliseconds:
            acknowledgement.requestMonotonicMilliseconds,
        requestIds,
        runNonce,
        seekPositionMilliseconds: 1000,
    });
    expect(acknowledgement.elementId).toMatch(
        /^media-element-[1-9][0-9]*$/,
    );
    expect(
        Number.isFinite(
            acknowledgement.preSeekPositionMilliseconds,
        ),
    ).toBe(true);
    expect(
        Number.isFinite(
            acknowledgement.requestMonotonicMilliseconds,
        ),
    ).toBe(true);
    return {
        captureReadySequence: captureReady.event.sequence,
        elementId: acknowledgement.elementId,
        preSeekPositionMilliseconds:
            acknowledgement.preSeekPositionMilliseconds,
        requestMonotonicMilliseconds:
            acknowledgement.requestMonotonicMilliseconds,
        requestIds,
        runNonce,
        holdObservation,
    };
}

function validateTask4Report(report, capture) {
    const qnam = checkDetail(report.snapshot, "qt-qnam-same-origin");
    expect(qnam).toEqual({
        contentType: "application/json; charset=utf-8",
        corsHeaderPresent: false,
        redirected: false,
        requestId: qnam.requestId,
        runNonce: capture.runNonce,
        status: 200,
    });
    expect(qnam.requestId).toMatch(/^request-[1-9][0-9]*$/);
    const qnamLogs = probeServer.probeLogs.filter(
        (entry) => entry.kind === "qnam"
            && entry.runNonce === capture.runNonce
            && entry.requestId === qnam.requestId,
    );
    expect(qnamLogs).toHaveLength(1);

    const wss = checkDetail(report.snapshot, "qt-wss-main-thread");
    expect(wss.allHandlersOnMainThread).toBe(true);
    expect(wss.closeCode).toBe(1000);
    expect(wss.closeReason).toBe("probe-complete");
    expect(wss.runNonce).toBe(capture.runNonce);
    expect(wss.connectionId).toMatch(/^connection-[1-9][0-9]*$/);
    const wssLogs = probeServer.probeLogs.filter(
        (entry) => entry.kind === "wss"
            && entry.runNonce === capture.runNonce
            && entry.connectionId === wss.connectionId,
    );
    expect(wssLogs.map((entry) => entry.event)).toEqual([
        "opened",
        "server-message",
        "text-received",
        "text-echoed",
        "binary-received",
        "binary-echoed",
        "heartbeat-received",
        "heartbeat-sent",
        "close-requested",
        "closed",
    ]);
    expect(wssLogs[0].origin).toBe(probeServer.origin);
    const nativeWebSocketEvents = report.events.filter(
        (event) => event.type.startsWith("qt-wss-"),
    );
    expect(nativeWebSocketEvents).toHaveLength(7);
    expect(nativeWebSocketEvents.every(
        (event) => (
            event.payload.connectionId === wss.connectionId
            && event.payload.runNonce === capture.runNonce
        ),
    )).toBe(true);
    expect(nativeWebSocketEvents.map((event) => event.type)).toEqual([
        "qt-wss-handler",
        "qt-wss-opened",
        "qt-wss-handler",
        "qt-wss-handler",
        "qt-wss-handler",
        "qt-wss-handler",
        "qt-wss-handler",
    ]);

    const naturalEnd = checkDetail(
        report.snapshot,
        "qt-media-natural-end",
    );
    validateExactKeys(
        naturalEnd,
        [
            "durationMilliseconds",
            "endPositionMilliseconds",
            "framePositionSamples",
            "postSeekFramePositionSamples",
            "requestIds",
            "resumeObserved",
            "resumePositionMilliseconds",
            "runNonce",
            "seekObserved",
            "seekProof",
        ],
        "nonce-owned media natural end",
    );
    expect(naturalEnd.runNonce).toBe(capture.runNonce);
    expect(naturalEnd.requestIds).toEqual(capture.requestIds);
    expect(naturalEnd.resumeObserved).toBe(true);
    expect(Math.abs(
        naturalEnd.resumePositionMilliseconds - 1000,
    )).toBeLessThanOrEqual(125);
    expect(naturalEnd.seekObserved).toBe(true);
    validateExactKeys(
        naturalEnd.seekProof,
        [
            "elementId",
            "jumpMilliseconds",
            "nativeResponseElapsedMilliseconds",
            "preSeekPositionMilliseconds",
            "requestMonotonicMilliseconds",
            "responseMilliseconds",
            "runNonce",
            "seekedMonotonicMilliseconds",
            "seekedPositionMilliseconds",
            "seekingMonotonicMilliseconds",
            "seekingPositionMilliseconds",
            "targetErrorMilliseconds",
            "targetPositionMilliseconds",
        ],
        "nonce-owned media seek proof",
    );
    expect(naturalEnd.seekProof.elementId).toBe(capture.elementId);
    expect(naturalEnd.seekProof.runNonce).toBe(capture.runNonce);
    expect(naturalEnd.seekProof.preSeekPositionMilliseconds).toBe(
        capture.preSeekPositionMilliseconds,
    );
    expect(naturalEnd.seekProof.requestMonotonicMilliseconds).toBe(
        capture.requestMonotonicMilliseconds,
    );
    expect(naturalEnd.seekProof.targetPositionMilliseconds).toBe(1000);
    expect(naturalEnd.seekProof.jumpMilliseconds).toBeGreaterThanOrEqual(
        100,
    );
    expect(naturalEnd.seekProof.targetErrorMilliseconds)
        .toBeLessThanOrEqual(125);
    expect(naturalEnd.seekProof.responseMilliseconds)
        .toBeLessThanOrEqual(1000);
    expect(naturalEnd.seekProof.nativeResponseElapsedMilliseconds)
        .toBeLessThanOrEqual(1000);
    expect(naturalEnd.seekProof.seekingMonotonicMilliseconds)
        .toBeGreaterThanOrEqual(
            naturalEnd.seekProof.requestMonotonicMilliseconds,
        );
    expect(naturalEnd.seekProof.seekedMonotonicMilliseconds)
        .toBeGreaterThanOrEqual(
            naturalEnd.seekProof.seekingMonotonicMilliseconds,
        );
    expect(naturalEnd.framePositionSamples.length).toBeGreaterThanOrEqual(2);
    for (
        let index = 1;
        index < naturalEnd.framePositionSamples.length;
        ++index
    ) {
        expect(naturalEnd.framePositionSamples[index]).toBeGreaterThan(
            naturalEnd.framePositionSamples[index - 1],
        );
    }
    expect(naturalEnd.framePositionSamples.at(-1)).toBeGreaterThanOrEqual(
        500,
    );
    expect(naturalEnd.postSeekFramePositionSamples.length)
        .toBeGreaterThanOrEqual(2);
    expect(naturalEnd.postSeekFramePositionSamples.length)
        .toBeLessThanOrEqual(32);
    for (
        let index = 1;
        index < naturalEnd.postSeekFramePositionSamples.length;
        ++index
    ) {
        expect(naturalEnd.postSeekFramePositionSamples[index])
            .toBeGreaterThan(
                naturalEnd.postSeekFramePositionSamples[index - 1],
            );
    }
    expect(naturalEnd.postSeekFramePositionSamples[0])
        .toBeGreaterThanOrEqual(1000);
    expect(naturalEnd.postSeekFramePositionSamples.at(-1))
        .toBeGreaterThanOrEqual(
            naturalEnd.resumePositionMilliseconds + 100,
        );
    expect(naturalEnd.durationMilliseconds).toBeGreaterThan(0);
    expect(Math.abs(
        naturalEnd.endPositionMilliseconds
            - naturalEnd.durationMilliseconds,
    )).toBeLessThanOrEqual(125);
    const postSeekFrameEvents = report.events.filter(
        (event) => event.type === "qt-media-post-seek-video-frame",
    );
    expect(postSeekFrameEvents.length).toBeGreaterThanOrEqual(2);
    const retainedPostSeekPrefixLength =
        naturalEnd.postSeekFramePositionSamples.length - 1;
    expect(postSeekFrameEvents.slice(
        0,
        retainedPostSeekPrefixLength,
    ).map(
        (event) => event.payload.positionMilliseconds,
    )).toEqual(
        naturalEnd.postSeekFramePositionSamples.slice(
            0,
            retainedPostSeekPrefixLength,
        ),
    );
    expect(naturalEnd.postSeekFramePositionSamples.at(-1))
        .toBeGreaterThanOrEqual(
            postSeekFrameEvents.at(-1).payload.positionMilliseconds,
        );
    const createdEvents = report.events.filter(
        (event) => event.type === "qt-media-player-output-created",
    );
    expect(createdEvents).toHaveLength(1);
    validateExactKeys(
        createdEvents[0].payload,
        [
            "audioOutputConstructed",
            "mediaPlayerConstructed",
            "outputDeviceIdSha256",
            "outputDeviceIsDefault",
            "runNonce",
        ],
        "fresh media player and output construction",
    );
    expect(createdEvents[0].payload).toMatchObject({
        audioOutputConstructed: true,
        mediaPlayerConstructed: true,
        outputDeviceIsDefault: true,
        runNonce: capture.runNonce,
    });
    expect(createdEvents[0].payload.outputDeviceIdSha256)
        .toMatch(/^[0-9a-f]{64}$/);
    const mediaLogs = probeServer.probeLogs.filter(
        (entry) => entry.kind === "media"
            && entry.runNonce === capture.runNonce
            && [200, 206, 416].includes(entry.status),
    );
    expect(mediaLogs.length).toBeGreaterThanOrEqual(1);
    expect(new Set(mediaLogs.map((entry) => entry.requestId))).toEqual(
        new Set(capture.requestIds),
    );
    expect(mediaLogs.every(
        (entry) => entry.route === "/fixtures/probe.webm",
    )).toBe(true);

    const capturePauseRequestedIndex = eventIndex(
        report.events,
        "qt-media-capture-pause-requested",
    );
    const captureReadyIndex = eventIndex(
        report.events,
        "qt-media-capture-ready",
    );
    const captureAckIndex = eventIndex(
        report.events,
        "qt-media-capture-acknowledged",
    );
    const seekIndex = eventIndex(
        report.events,
        "qt-media-seek-requested",
    );
    const seekObservedIndex = eventIndex(
        report.events,
        "qt-media-seek-observed",
    );
    const resumeRequestedIndex = eventIndex(
        report.events,
        "qt-media-resume-requested",
    );
    const playbackResumedIndex = eventIndex(
        report.events,
        "qt-media-playback-resumed",
    );
    const playbackResumed = report.events[playbackResumedIndex];
    validateExactKeys(
        playbackResumed.payload,
        ["positionMilliseconds", "runNonce"],
        "post-seek media playback resume",
    );
    expect(playbackResumed.payload.runNonce).toBe(capture.runNonce);
    expect(Math.abs(
        playbackResumed.payload.positionMilliseconds - 1000,
    )).toBeLessThanOrEqual(125);
    expect(playbackResumed.payload.positionMilliseconds).toBe(
        naturalEnd.resumePositionMilliseconds,
    );
    const playerOutputCreatedIndex = eventIndex(
        report.events,
        "qt-media-player-output-created",
    );
    const sourceSetIndex = eventIndex(
        report.events,
        "qt-media-source-set",
    );
    const naturalEndIndex = eventIndex(
        report.events,
        "check-passed",
        (payload) => payload.check === "qt-media-natural-end",
    );
    const backendRemovalArmedIndex = eventIndex(
        report.events,
        "qt-media-backend-removal-armed",
    );
    const resourceReleasedIndex = eventIndex(
        report.events,
        "qt-media-element-resource-released",
    );
    const backendRemovedIndex = eventIndex(
        report.events,
        "qt-media-backend-removed",
    );
    const playerDestroyedIndex = eventIndex(
        report.events,
        "qt-media-player-destroyed",
    );
    const audioOutputDestroyedIndex = eventIndex(
        report.events,
        "qt-media-audio-output-destroyed",
    );
    const cleanTeardownIndex = eventIndex(
        report.events,
        "check-passed",
        (payload) => payload.check === "qt-media-clean-teardown",
    );
    const firstPostSeekFrameIndex = eventIndex(
        report.events,
        "qt-media-post-seek-video-frame",
    );
    expect(captureReadyIndex).toBe(capture.captureReadySequence);
    expect(playerOutputCreatedIndex).toBeLessThan(sourceSetIndex);
    expect(captureReadyIndex).toBeGreaterThan(
        capturePauseRequestedIndex,
    );
    expect(captureAckIndex).toBeGreaterThan(captureReadyIndex);
    expect(seekIndex).toBeGreaterThan(captureAckIndex);
    expect(seekObservedIndex).toBeGreaterThan(seekIndex);
    expect(resumeRequestedIndex).toBeGreaterThan(seekObservedIndex);
    expect(playbackResumedIndex).toBeGreaterThan(resumeRequestedIndex);
    expect(firstPostSeekFrameIndex).toBeGreaterThan(playbackResumedIndex);
    expect(naturalEndIndex).toBeGreaterThan(firstPostSeekFrameIndex);
    expect(backendRemovalArmedIndex).toBeGreaterThan(naturalEndIndex);
    expect(playerDestroyedIndex).toBeGreaterThan(
        backendRemovalArmedIndex,
    );
    expect(resourceReleasedIndex).toBeGreaterThan(playerDestroyedIndex);
    expect(backendRemovedIndex).toBeGreaterThan(resourceReleasedIndex);
    expect(audioOutputDestroyedIndex).toBeGreaterThan(
        backendRemovalArmedIndex,
    );
    expect(cleanTeardownIndex).toBeGreaterThan(playerDestroyedIndex);
    expect(cleanTeardownIndex).toBeGreaterThan(backendRemovedIndex);
    expect(cleanTeardownIndex).toBeGreaterThan(
        audioOutputDestroyedIndex,
    );
    const mediaRemovalKeys = [
        "cleanupArmed",
        "currentSourceCleared",
        "currentSourceOwnedOrEmpty",
        "currentTimeReset",
        "documentOwnedElementCount",
        "domRemoved",
        "durationCleared",
        "elementId",
        "emptiedObserved",
        "exactSourceMatched",
        "hadResourceBeforeDestruction",
        "matchingElementCount",
        "mediaElementResourceReleased",
        "mediaErrorCleared",
        "networkStateEmpty",
        "observerMutatedElement",
        "paused",
        "readyStateEmpty",
        "runNonce",
        "seekingStopped",
        "sourceAttributeCleared",
        "sourceElementCount",
        "sourceObjectCleared",
        "sourcePathAndQuery",
        "sourcePropertyCleared",
        "trackedMediaElementCount",
        "wasConnected",
    ];
    const backendRemovalArm =
        report.events[backendRemovalArmedIndex].payload;
    validateExactKeys(
        backendRemovalArm,
        [
            "cleanupArmed",
            "elementId",
            "exactSourceMatched",
            "hadResourceBeforeDestruction",
            "matchingElementCount",
            "observerMutatedElement",
            "runNonce",
            "sourcePathAndQuery",
            "trackedMediaElementCount",
            "wasConnected",
        ],
        "media backend removal arm",
    );
    expect(backendRemovalArm).toMatchObject({
        cleanupArmed: true,
        exactSourceMatched: true,
        hadResourceBeforeDestruction: true,
        matchingElementCount: 1,
        observerMutatedElement: false,
        runNonce: capture.runNonce,
        sourcePathAndQuery:
            `/fixtures/probe.webm?nonce=${capture.runNonce}`,
        wasConnected: true,
    });
    expect(backendRemovalArm.trackedMediaElementCount)
        .toBeGreaterThanOrEqual(1);
    const resourceReleased =
        report.events[resourceReleasedIndex].payload;
    const backendRemoved =
        report.events[backendRemovedIndex].payload;
    for (const [description, payload] of [
        ["media element resource release", resourceReleased],
        ["media backend removal", backendRemoved],
    ]) {
        validateExactKeys(payload, mediaRemovalKeys, description);
        expect(typeof payload.currentSourceCleared).toBe("boolean");
        expect(payload).toMatchObject({
            cleanupArmed: true,
            currentSourceOwnedOrEmpty: true,
            currentTimeReset: true,
            documentOwnedElementCount: 0,
            domRemoved: true,
            durationCleared: true,
            exactSourceMatched: true,
            hadResourceBeforeDestruction: true,
            matchingElementCount: 1,
            mediaErrorCleared: true,
            observerMutatedElement: false,
            paused: true,
            runNonce: capture.runNonce,
            seekingStopped: true,
            sourceAttributeCleared: true,
            sourceElementCount: 0,
            sourceObjectCleared: true,
            sourcePathAndQuery:
                `/fixtures/probe.webm?nonce=${capture.runNonce}`,
            sourcePropertyCleared: true,
            wasConnected: true,
        });
        expect(payload.trackedMediaElementCount).toBeGreaterThanOrEqual(1);
        for (const key of [
            "cleanupArmed",
            "elementId",
            "exactSourceMatched",
            "hadResourceBeforeDestruction",
            "matchingElementCount",
            "observerMutatedElement",
            "runNonce",
            "sourcePathAndQuery",
            "trackedMediaElementCount",
            "wasConnected",
        ]) {
            expect(payload[key]).toEqual(backendRemovalArm[key]);
        }
    }
    expect(resourceReleased).toMatchObject({
        emptiedObserved: true,
        mediaElementResourceReleased: true,
        networkStateEmpty: true,
        readyStateEmpty: true,
    });
    expect(backendRemoved).toEqual(resourceReleased);
    expect(checkDetail(
        report.snapshot,
        "qt-media-clean-teardown",
    )).toEqual({
        audioOutputDestructionRecorded: true,
        backendRemovalRecorded: true,
        playerDestructionRecorded: true,
        runNonce: capture.runNonce,
    });
}

async function waitForJspiSuspension(page, diagnosticFailure) {
    await waitForReportOutcome(
        page,
        "Task 3 JSPI import did not suspend before trusted input",
        {
            kind: "jspi-suspended",
        },
        diagnosticFailure,
    );
}

async function waitForEventPumpIdle(page, diagnosticFailure) {
    await waitForReportOutcome(
        page,
        "Task 3 event pump did not become idle",
        {
            kind: "pump-idle",
        },
        diagnosticFailure,
    );
}

async function waitForUserActivation(
    page,
    sequenceBaseline,
    diagnosticFailure,
) {
    await waitForReportOutcome(
        page,
        "trusted QML click did not report active user activation",
        {
            kind: "user-activation",
            sequenceBaseline,
        },
        diagnosticFailure,
    );
}

async function verifyContextMenuSuppression(page) {
    const baseline = await page.evaluate(
        () => globalThis.__gate1bContextMenuObservations.length,
    );
    await page.mouse.click(
        BUTTON_POINT.x,
        BUTTON_POINT.y,
        { button: "right" },
    );
    await page.waitForFunction(
        (expectedCount) => (
            globalThis.__gate1bContextMenuObservations.length
                === expectedCount
        ),
        baseline + 1,
        {
            polling: "raf",
            timeout: EXPECT_TIMEOUT_MS,
        },
    );
    return page.evaluate(() => structuredClone(
        globalThis.__gate1bContextMenuObservations.at(-1),
    ));
}

async function consumeTransientUserActivation(page) {
    const observation = await page.evaluate(() => {
        const activeBefore = navigator.userActivation.isActive;
        const popup = globalThis.open(
            "about:blank",
            "gate1b-activation-consumer",
            "popup,width=1,height=1",
        );
        const popupOpened = popup !== null;
        popup?.close();
        return {
            activeAfter: navigator.userActivation.isActive,
            activeBefore,
            popupOpened,
        };
    });
    validateExactKeys(
        observation,
        ["activeAfter", "activeBefore", "popupOpened"],
        "transient user-activation consumption",
    );
    return observation;
}

async function verifyNonBubblingWindowWakeups(page) {
    return page.evaluate(() => {
        const report = globalThis.__rhythmGameGate1b;
        const inputKicksBefore = report.eventPump.inputKicks;
        const nonBubblingInputKicksBefore =
            report.eventPump.nonBubblingInputKicks;
        const reentrantInputCallsBefore =
            report.eventPump.reentrantInputCalls;
        const qtWindow = document.querySelector("#screen")
            ?.querySelector(":scope > #qt-shadow-container")
            ?.shadowRoot
            ?.querySelector(".qt-window") ?? null;
        if (qtWindow === null) {
            throw new Error("gate1b-qt-window-missing");
        }
        for (const type of ["pointerenter", "pointerleave"]) {
            const dispatched = qtWindow.dispatchEvent(new PointerEvent(type, {
                bubbles: false,
                cancelable: true,
                clientX: 124,
                clientY: 316,
                composed: false,
                pointerId: 91,
                pointerType: "mouse",
            }));
            if (!dispatched) {
                throw new Error(`gate1b-${type}-unexpectedly-cancelled`);
            }
        }
        return {
            inFlightAfter: report.eventPump.inFlight,
            inputKickDelta:
                report.eventPump.inputKicks - inputKicksBefore,
            nonBubblingInputKickDelta:
                report.eventPump.nonBubblingInputKicks
                - nonBubblingInputKicksBefore,
            reentrantInputCallDelta:
                report.eventPump.reentrantInputCalls
                - reentrantInputCallsBefore,
        };
    });
}

async function verifyClipboardDispatchTiming(page) {
    return page.evaluate(() => {
        const report = globalThis.__rhythmGameGate1b;
        const inFlightBefore = report.eventPump.inFlight;
        const inputKicksBefore = report.eventPump.inputKicks;
        const reentrantInputCallsBefore =
            report.eventPump.reentrantInputCalls;
        const clipboardData = new DataTransfer();
        clipboardData.setData("text/plain", "gate1b-clipboard-timing");
        const clipboardEvent = new ClipboardEvent("paste", {
            bubbles: true,
            cancelable: true,
            clipboardData,
            composed: true,
        });
        const dispatchReturned = document.dispatchEvent(clipboardEvent);
        return {
            defaultPrevented: clipboardEvent.defaultPrevented,
            dispatchReturned,
            inFlightBefore,
            inputKickDelta:
                report.eventPump.inputKicks - inputKicksBefore,
            isTrusted: clipboardEvent.isTrusted,
            reentrantInputCallDelta:
                report.eventPump.reentrantInputCalls
                    - reentrantInputCallsBefore,
        };
    });
}

async function exerciseForegroundPumpLatency(
    page,
    diagnosticFailure,
) {
    const acknowledgement = await page.evaluate(
        async (sampleCount) => globalThis.__rhythmGameGate1b.command(
            "begin-foreground-latency-sampling",
            { sampleCount },
        ),
        FOREGROUND_INPUT_SAMPLE_COUNT,
    );
    validateExactKeys(
        acknowledgement,
        [
            "command",
            "inputSampleCount",
            "timerIntervalMilliseconds",
            "timerSampleCount",
        ],
        "foreground latency acknowledgement",
    );
    expect(acknowledgement).toEqual({
        command: "begin-foreground-latency-sampling",
        inputSampleCount: FOREGROUND_INPUT_SAMPLE_COUNT,
        timerIntervalMilliseconds: FOREGROUND_TIMER_INTERVAL_MS,
        timerSampleCount: FOREGROUND_TIMER_SAMPLE_COUNT,
    });

    for (let index = 0; index < FOREGROUND_INPUT_SAMPLE_COUNT; ++index) {
        await page.mouse.move(
            180 + (index % 16),
            170 + Math.floor(index / 16),
        );
    }
    await waitForReportOutcome(
        page,
        "trusted input or precise Qt timer sampling did not complete",
        {
            checks: [
                "qt-foreground-input-delivery",
                "qt-foreground-timer-delivery",
            ],
            kind: "core-checks",
        },
        diagnosticFailure,
    );

    const latencyReport = await readReport(page);
    const inputDelivery = checkDetail(
        latencyReport.snapshot,
        "qt-foreground-input-delivery",
    );
    expect(inputDelivery).toEqual({
        sameDispatchRequired: true,
        sampleCount: FOREGROUND_INPUT_SAMPLE_COUNT,
    });
    const timerDelivery = checkDetail(
        latencyReport.snapshot,
        "qt-foreground-timer-delivery",
    );
    validateExactKeys(
        timerDelivery,
        [
            "intervalMilliseconds",
            "latenessMicroseconds",
            "sampleCount",
        ],
        "foreground Qt timer delivery",
    );
    expect(timerDelivery.intervalMilliseconds).toBe(
        FOREGROUND_TIMER_INTERVAL_MS,
    );
    expect(timerDelivery.sampleCount).toBe(
        FOREGROUND_TIMER_SAMPLE_COUNT,
    );
    expect(timerDelivery.latenessMicroseconds).toHaveLength(
        FOREGROUND_TIMER_SAMPLE_COUNT,
    );
    const timerLatenessMilliseconds =
        timerDelivery.latenessMicroseconds.map((microseconds) => {
            expect(Number.isSafeInteger(microseconds)).toBe(true);
            expect(microseconds).toBeGreaterThanOrEqual(0);
            return microseconds / 1000;
        });
    expect(nearestRankP95(timerLatenessMilliseconds)).toBeLessThanOrEqual(
        FOREGROUND_TIMER_LATENESS_BUDGET_MS,
    );
}

async function exercisePersistedPageLifecycle(
    page,
    diagnosticFailure,
) {
    const acknowledgement = await page.evaluate(
        async () => globalThis.__rhythmGameGate1b.command(
            "arm-bfcache-resume-probe",
            {},
        ),
    );
    validateExactKeys(
        acknowledgement,
        ["command", "handlerIndex", "runNonce"],
        "BFCache resume acknowledgement",
    );
    expect(acknowledgement.command).toBe("arm-bfcache-resume-probe");
    expect(acknowledgement.handlerIndex).toBeGreaterThan(0);
    expect(acknowledgement.runNonce).toBeGreaterThan(0);

    const awayNavigation = await page.goto(
        `${probeServer.origin}/probe/bfcache-away`,
        { waitUntil: "domcontentloaded" },
    );
    expect(awayNavigation?.status()).toBe(200);
    await page.goBack({ waitUntil: "commit" });
    await waitForReportOutcome(
        page,
        "real BFCache restoration did not deliver its retained C++ sentinel",
        {
            checks: ["qt-bfcache-resume"],
            kind: "core-checks",
        },
        diagnosticFailure,
    );

    const restored = await page.evaluate(() => ({
        eventPump: structuredClone(
            globalThis.__rhythmGameGate1b.eventPump,
        ),
        report: {
            events: structuredClone(
                [...globalThis.__rhythmGameGate1b.events],
            ),
            snapshot: structuredClone(
                globalThis.__rhythmGameGate1b.snapshot,
            ),
        },
    }));
    // Navigation Timing's "back_forward" type is not a BFCache hit signal.
    // These counters advance only inside the real pageshow.persisted branch.
    expect(restored.eventPump.bfcacheRestores).toBeGreaterThanOrEqual(1);
    expect(restored.eventPump.bfcacheResumePumpSerial).toBeGreaterThan(0);
    expect(restored.eventPump.bfcacheSentinelsQueued).toBe(1);
    expect(restored.eventPump.lifecyclePaused).toBe(false);
    expect(restored.eventPump.lifecyclePauses).toBeGreaterThanOrEqual(1);
    expect(restored.eventPump.stopped).toBe(false);
    const resume = checkDetail(
        restored.report.snapshot,
        "qt-bfcache-resume",
    );
    expect(resume).toEqual({ runNonce: acknowledgement.runNonce });
    const sentinels = restored.report.events.filter(
        (event) => event.type === "qt-bfcache-resume-sentinel",
    );
    expect(sentinels).toHaveLength(1);
    expect(sentinels[0].payload).toEqual({
        runNonce: acknowledgement.runNonce,
    });
}

async function exerciseHiddenPageFallback(page) {
    const baseline = await page.evaluate(() => ({
        hiddenIdleTimers:
            globalThis.__rhythmGameGate1b.eventPump.hiddenIdleTimers,
        idleFrames: globalThis.__rhythmGameGate1b.eventPump.idleFrames,
        visibilityReschedules:
            globalThis.__rhythmGameGate1b.eventPump.visibilityReschedules,
    }));
    const coverPage = await page.context().newPage();
    try {
        await coverPage.goto("about:blank");
        await coverPage.bringToFront();
        await page.waitForFunction(
            () => document.visibilityState === "hidden",
            undefined,
            { timeout: EXPECT_TIMEOUT_MS },
        );
        const acknowledgement = await page.evaluate(
            async () => globalThis.__rhythmGameGate1b.command(
                "arm-hidden-timer-probe",
                {},
            ),
        );
        expect(acknowledgement).toEqual({
            command: "arm-hidden-timer-probe",
            intervalMilliseconds: HIDDEN_TIMER_INTERVAL_MS,
        });
        await page.waitForFunction(
            ({ hiddenTimerBaseline }) => (
                document.visibilityState === "hidden"
                && globalThis.__rhythmGameGate1b.eventPump.hiddenIdleTimers
                    > hiddenTimerBaseline
                && (
                    globalThis.__rhythmGameGate1b.eventPump
                        .hiddenQtTimerSentinels
                    === 1
                )
                && (
                    globalThis.__rhythmGameGate1b.snapshot?.checks?.[
                        "qt-hidden-timer-delivery"
                    ]?.passed === true
                )
            ),
            {
                hiddenTimerBaseline: baseline.hiddenIdleTimers,
            },
            {
                polling: 100,
                timeout: EXPECT_TIMEOUT_MS,
            },
        );
        const postHiddenBaseline = await page.evaluate(() => ({
            idleFrames:
                globalThis.__rhythmGameGate1b.eventPump.idleFrames,
            visibilityReschedules:
                globalThis.__rhythmGameGate1b.eventPump
                    .visibilityReschedules,
        }));
        await page.bringToFront();
        await page.waitForFunction(
            ({ idleFrameBaseline, visibilityBaseline }) => (
                document.visibilityState === "visible"
                && globalThis.__rhythmGameGate1b.eventPump.idleFrames
                    > idleFrameBaseline
                && (
                    globalThis.__rhythmGameGate1b.eventPump
                        .visibilityReschedules
                    >= visibilityBaseline + 1
                )
            ),
            {
                idleFrameBaseline: postHiddenBaseline.idleFrames,
                visibilityBaseline:
                    postHiddenBaseline.visibilityReschedules,
            },
            {
                polling: "raf",
                timeout: EXPECT_TIMEOUT_MS,
            },
        );
        const resumed = await page.evaluate(
            async () => globalThis.__rhythmGameGate1b.command(
                "arm-visible-resume-timer-probe",
                {},
            ),
        );
        expect(resumed).toEqual({
            command: "arm-visible-resume-timer-probe",
            intervalMilliseconds: VISIBLE_RESUME_TIMER_INTERVAL_MS,
        });
        await page.waitForFunction(
            () => (
                document.visibilityState === "visible"
                && (
                    globalThis.__rhythmGameGate1b.eventPump
                        .resumedQtTimerSentinels
                    === 1
                )
                && (
                    globalThis.__rhythmGameGate1b.snapshot?.checks?.[
                        "qt-visible-resume-timer-delivery"
                    ]?.passed === true
                )
            ),
            undefined,
            {
                polling: "raf",
                timeout: EXPECT_TIMEOUT_MS,
            },
        );
        const lifecycleReport = await readReport(page);
        const hiddenSentinels = lifecycleReport.events.filter(
            (event) => event.type === "qt-hidden-timer-sentinel",
        );
        expect(hiddenSentinels).toHaveLength(1);
        expect(hiddenSentinels[0].payload).toEqual({
            intervalMilliseconds: HIDDEN_TIMER_INTERVAL_MS,
        });
        expect(checkDetail(
            lifecycleReport.snapshot,
            "qt-hidden-timer-delivery",
        )).toEqual({
            intervalMilliseconds: HIDDEN_TIMER_INTERVAL_MS,
        });
        const visibleSentinels = lifecycleReport.events.filter(
            (event) => (
                event.type === "qt-visible-resume-timer-sentinel"
            ),
        );
        expect(visibleSentinels).toHaveLength(1);
        expect(visibleSentinels[0].payload).toEqual({
            intervalMilliseconds: VISIBLE_RESUME_TIMER_INTERVAL_MS,
        });
        expect(checkDetail(
            lifecycleReport.snapshot,
            "qt-visible-resume-timer-delivery",
        )).toEqual({
            intervalMilliseconds: VISIBLE_RESUME_TIMER_INTERVAL_MS,
        });
        expect(visibleSentinels[0].sequence).toBeGreaterThan(
            hiddenSentinels[0].sequence,
        );
        expect(lifecycleReport.snapshot.failures).toEqual([]);
        expect(lifecycleReport.eventPump.stopped).toBe(false);
        expect(lifecycleReport.eventPump.lifecyclePaused).toBe(false);
    } finally {
        await coverPage.close();
        await page.bringToFront();
    }
}

async function setShaderPhase(page, phase, diagnosticFailure) {
    const acknowledgement = await page.evaluate(
        async (requestedPhase) => globalThis.__rhythmGameGate1b.command(
            "set-shader-phase",
            { phase: requestedPhase },
        ),
        phase,
    );
    validateExactKeys(
        acknowledgement,
        ["command", "frameBaseline", "generation", "phase"],
        "shader phase acknowledgement",
    );
    expect(acknowledgement.command).toBe("set-shader-phase");
    expect(acknowledgement.phase).toBe(phase);
    expect(Number.isSafeInteger(acknowledgement.generation)).toBe(true);
    expect(Number.isSafeInteger(acknowledgement.frameBaseline)).toBe(true);

    await waitForReportOutcome(
        page,
        `two Qt frames were not rendered for phase ${phase}`,
        {
            frameBaseline: acknowledgement.frameBaseline,
            generation: acknowledgement.generation,
            kind: "render-generation",
        },
        diagnosticFailure,
    );
    return acknowledgement;
}

async function exerciseRuntimeCommandConcurrencyGuard(
    page,
    diagnosticFailure,
) {
    const settled = await page.evaluate(async () => {
        const command = globalThis.__rhythmGameGate1b.command;
        const results = await Promise.allSettled([
            command("set-shader-phase", { phase: 0.20 }),
            command("set-shader-phase", { phase: 0.80 }),
        ]);
        return results.map((result) => (
            result.status === "fulfilled"
                ? {
                    status: result.status,
                    value: result.value,
                }
                : {
                    reason: String(
                        result.reason?.message ?? result.reason,
                    ),
                    status: result.status,
                }
        ));
    });
    expect(settled).toHaveLength(2);
    expect(settled[0].status).toBe("fulfilled");
    validateExactKeys(
        settled[0].value,
        ["command", "frameBaseline", "generation", "phase"],
        "guarded shader command acknowledgement",
    );
    expect(settled[0].value.command).toBe("set-shader-phase");
    expect(settled[0].value.phase).toBe(0.20);
    expect(settled[1]).toEqual({
        reason: "gate1b-command-in-flight",
        status: "rejected",
    });
    await waitForReportOutcome(
        page,
        "the acknowledged concurrent-guard capture was overwritten",
        {
            frameBaseline: settled[0].value.frameBaseline,
            generation: settled[0].value.generation,
            kind: "render-generation",
        },
        diagnosticFailure,
    );
    return settled[0].value;
}

function assertCapturePixels(pngBytes, phase) {
    const png = PNG.sync.read(pngBytes);
    expect(png.width).toBe(CAPTURE_RECT.width);
    expect(png.height).toBe(CAPTURE_RECT.height);
    const expected = expectedRgba(phase);
    for (const [x, y] of SAMPLE_COORDINATES) {
        const offset = (y * png.width + x) * 4;
        const actual = [...png.data.subarray(offset, offset + 4)];
        for (const [channel, expectedValue] of expected.entries()) {
            expect(
                Math.abs(actual[channel] - expectedValue),
                `phase ${phase} pixel (${x}, ${y}) channel ${channel}`,
            ).toBeLessThanOrEqual(PIXEL_TOLERANCE);
        }
    }
}

async function inspectQtCanvasCoverage(page) {
    const layoutCoverage = await page.evaluate(
        ({ captureRect, sampleCoordinates }) => {
            const screen = document.querySelector("#screen");
            const host = screen?.querySelector("#qt-shadow-container") ?? null;
            const qtRoot = host?.shadowRoot ?? null;
            const canvases = qtRoot === null
                ? []
                : [...qtRoot.querySelectorAll("canvas")];
            const canvas = canvases.length === 1 ? canvases[0] : null;
            const windowElement = canvas?.parentElement ?? null;
            const canvasRect = canvas?.getBoundingClientRect() ?? null;
            const canvasStyle = canvas === null
                ? null
                : getComputedStyle(canvas);
            return sampleCoordinates.map(([sampleX, sampleY]) => {
                const x = captureRect.x + sampleX;
                const y = captureRect.y + sampleY;
                const canvasCoversPoint = (
                    canvasRect !== null
                    && canvasRect.left <= x
                    && canvasRect.right > x
                    && canvasRect.top <= y
                    && canvasRect.bottom > y
                );
                return {
                    canvasClassName: canvas?.className ?? null,
                    canvasCount: canvases.length,
                    canvasParentClassName: windowElement?.className ?? null,
                    canvasPointerEvents: canvasStyle?.pointerEvents ?? null,
                    canvasTagName: canvas?.tagName ?? null,
                    hostId: host?.id ?? null,
                    layoutIsQtOwned: (
                        canvas !== null
                        && canvas.isConnected
                        && canvasCoversPoint
                        && canvas.classList.length === 1
                        && canvas.classList.contains("qt-window-canvas")
                        && canvasStyle?.display !== "none"
                        && canvasStyle?.visibility === "visible"
                        && Number(canvasStyle?.opacity ?? 0) > 0
                        && canvasStyle?.pointerEvents === "none"
                        && host?.id === "qt-shadow-container"
                        && screen?.contains(host) === true
                        && windowElement?.classList.length === 1
                        && windowElement?.classList.contains("qt-window")
                        && qtRoot?.querySelectorAll(".qt-window").length === 1
                        && qtRoot?.contains(windowElement) === true
                    ),
                    x,
                    y,
                };
            });
        },
        {
            captureRect: CAPTURE_RECT,
            sampleCoordinates: SAMPLE_COORDINATES,
        },
    );
    const cdpSession = await page.context().newCDPSession(page);
    const objectGroup = "rhythm-game-gate1b-canvas-proof";
    try {
        await cdpSession.send("DOM.enable");
        const canvasEvaluation = await cdpSession.send("Runtime.evaluate", {
            expression: [
                'document.querySelector("#screen")',
                '?.querySelector("#qt-shadow-container")',
                "?.shadowRoot",
                '?.querySelector("canvas.qt-window-canvas") ?? null',
            ].join(""),
            objectGroup,
            returnByValue: false,
        });
        const canvasObjectId = canvasEvaluation.result.objectId;
        if (
            canvasEvaluation.result.subtype === "null"
            || typeof canvasObjectId !== "string"
        ) {
            throw new Error("Qt canvas object could not be resolved by CDP");
        }
        const canvasDescription = await cdpSession.send("DOM.describeNode", {
            depth: 0,
            objectId: canvasObjectId,
            pierce: true,
        });
        const canvasBackendNodeId =
            canvasDescription.node.backendNodeId;
        const compositorCoverage = [];
        for (const coverage of layoutCoverage) {
            const hit = await cdpSession.send("DOM.getNodeForLocation", {
                ignorePointerEventsNone: true,
                includeUserAgentShadowDOM: true,
                x: coverage.x,
                y: coverage.y,
            });
            const hitDescription = await cdpSession.send("DOM.describeNode", {
                backendNodeId: hit.backendNodeId,
                depth: 0,
                pierce: true,
            });
            compositorCoverage.push({
                compositorHitIsExactCanvas:
                    hit.backendNodeId === canvasBackendNodeId,
                compositorNodeName: hitDescription.node.nodeName,
                compositorPseudoType:
                    hitDescription.node.pseudoType ?? null,
            });
        }
        return layoutCoverage.map((coverage, index) => ({
            ...coverage,
            ...compositorCoverage[index],
            owned: (
                coverage.layoutIsQtOwned
                && compositorCoverage[index].compositorHitIsExactCanvas
            ),
        }));
    } finally {
        await cdpSession.send("Runtime.releaseObjectGroup", { objectGroup })
            .catch(() => {});
        await cdpSession.detach();
    }
}

async function inspectOwnedJspiImport(page) {
    return page.evaluate(async (ownedImport) => {
        const manifestResponse = await fetch("/runtime-artifacts.json", {
            cache: "no-store",
        });
        if (!manifestResponse.ok) {
            throw new Error(
                `runtime manifest returned ${manifestResponse.status}`,
            );
        }
        const manifest = await manifestResponse.json();
        const [mainJsResponse, wasmResponse] = await Promise.all([
            fetch(`/${manifest.artifacts.mainJs.url}`, {
                cache: "no-store",
            }),
            fetch(`/${manifest.artifacts.wasm.url}`, {
                cache: "no-store",
            }),
        ]);
        if (!mainJsResponse.ok || !wasmResponse.ok) {
            throw new Error("content-addressed JSPI artifacts were not served");
        }
        const [mainJs, wasmBytes] = await Promise.all([
            mainJsResponse.text(),
            wasmResponse.arrayBuffer(),
        ]);
        const module = await WebAssembly.compile(wasmBytes);
        const imports = WebAssembly.Module.imports(module);
        const ownedOffset = mainJs.indexOf(`function ${ownedImport}(`);
        const signatureOffset = ownedOffset < 0
            ? -1
            : mainJs.indexOf(`${ownedImport}.sig=`, ownedOffset);
        const ownedEnd = signatureOffset < 0
            ? -1
            : mainJs.indexOf(";", signatureOffset);
        const ownedFunction = ownedEnd < 0
            ? ""
            : mainJs.slice(ownedOffset, ownedEnd + 1);
        const wrapperStart = mainJs.indexOf(
            "instrumentWasmImports(imports){",
        );
        const wrapperEndMarker = "},instrumentWasmExports";
        const wrapperEnd = wrapperStart < 0
            ? -1
            : mainJs.indexOf(wrapperEndMarker, wrapperStart);
        const importWrapper = wrapperEnd < 0
            ? ""
            : mainJs.slice(
                wrapperStart,
                wrapperEnd + wrapperEndMarker.length,
            );
        return {
            buildId: manifest.buildId,
            importPatternOwnsAsyncJsInWrapper:
                /importPattern=\/\^\(invoke_\.\*\|__asyncjs__\.\*\)\$\//
                    .test(importWrapper),
            importPatternTestedInWrapper:
                importWrapper.includes("importPattern.test(x)"),
            imports,
            ownedFunctionCancelsOnTerminal: (
                ownedFunction.includes("report.ready")
                && ownedFunction.includes(".catch")
                && ownedFunction.includes("cancelOwnedImport")
                && ownedFunction.includes("resumeWatchdogTimer")
            ),
            ownedFunctionUsesHandleAsync:
                ownedFunction.includes("Asyncify.handleAsync"),
            ownedFunctionUsesPromiseRace: (
                ownedFunction.includes("new Promise")
                && ownedFunction.includes("Promise.race")
                && (ownedFunction.match(/setTimeout\s*\(/g) ?? []).length
                    >= 3
                && (ownedFunction.match(/clearTimeout\s*\(/g) ?? []).length
                    >= 2
            ),
            ownedFunctionOwnsResumeWatchdog: (
                ownedFunction.includes("jspi-resume-watchdog-timeout")
                && ownedFunction.includes("jspi-after-exec")
                && ownedFunction.includes("postLoopSentinel")
                && ownedFunction.includes(".rejectReady")
                && ownedFunction.includes("7500")
            ),
            ownedFunctionRequiresTrustedPointer: (
                ownedFunction.includes("#screen")
                && ownedFunction.includes("pointerup")
                && ownedFunction.includes("isTrusted")
                && ownedFunction.includes("event.button!==0")
                && ownedFunction.includes("capture:true")
                && ownedFunction.includes("removeEventListener")
            ),
            ownedImportHasGeneratedFunction: (
                ownedOffset >= 0
                && signatureOffset > ownedOffset
                && ownedEnd > signatureOffset
            ),
            wrapperUsesWebAssemblySuspending:
                importWrapper.includes(
                    "new WebAssembly.Suspending(original)",
                ),
        };
    }, OWNED_ASYNC_IMPORT);
}

async function installMediaDeviceScenario(page, scenario) {
    await page.addInitScript((requestedScenario) => {
        const state = {
            callCount: 0,
            callCountImmediatelyAfterDispatch: -1,
            currentListIndex: -1,
            deviceChangeDispatches: 0,
            enumerateResolvedCount: 0,
            firstDelayElapsedMilliseconds: 0,
            firstDelayTurns: 0,
            mode: requestedScenario.mode,
            secondEnumerateStartedAfterDispatchMilliseconds: -1,
        };
        let deviceChangeDispatchMonotonicMilliseconds = -1;
        let releaseFirstEnumeration = null;
        const snapshot = () => structuredClone(state);
        const publishApi = (api) => {
            Object.defineProperty(
                globalThis,
                "__gate1bMediaDeviceScenario",
                {
                    configurable: false,
                    enumerable: false,
                    value: Object.freeze(api),
                    writable: false,
                },
            );
        };

        if (requestedScenario.mode === "unavailable") {
            Object.defineProperty(navigator, "mediaDevices", {
                configurable: false,
                enumerable: true,
                value: undefined,
                writable: false,
            });
            publishApi({ snapshot });
            return;
        }
        if (requestedScenario.mode !== "controlled") {
            throw new Error("unknown media-device scenario");
        }
        const mediaDevices = new EventTarget();
        Object.defineProperty(navigator, "mediaDevices", {
            configurable: false,
            enumerable: true,
            value: mediaDevices,
            writable: false,
        });
        const lists = requestedScenario.deviceLists.map(
            (list) => Object.freeze(list.map((device) => Object.freeze({
                deviceId: device.deviceId,
                groupId: device.groupId,
                kind: device.kind,
                label: device.label,
            }))),
        );
        if (lists.length !== 2) {
            throw new Error("controlled scenario requires two device lists");
        }
        state.currentListIndex = 0;
        const firstEnumerationGate =
            requestedScenario.holdFirstEnumeration === true
                ? new Promise((resolve) => {
                    releaseFirstEnumeration = resolve;
                })
                : null;
        const enumerateDevices = async () => {
            ++state.callCount;
            const call = state.callCount;
            const listIndexAtCall = state.currentListIndex;
            if (call === 2) {
                if (deviceChangeDispatchMonotonicMilliseconds < 0) {
                    throw new Error(
                        "second enumeration preceded devicechange",
                    );
                }
                state.secondEnumerateStartedAfterDispatchMilliseconds =
                    performance.now()
                        - deviceChangeDispatchMonotonicMilliseconds;
            }
            if (call === 1) {
                const started = performance.now();
                for (
                    let turn = 0;
                    turn < requestedScenario.initialDelayTurns;
                    ++turn
                ) {
                    await new Promise((resolve) => {
                        setTimeout(resolve, 0);
                    });
                    ++state.firstDelayTurns;
                }
                if (requestedScenario.initialDelayMilliseconds > 0) {
                    await new Promise((resolve) => {
                        setTimeout(
                            resolve,
                            requestedScenario.initialDelayMilliseconds,
                        );
                    });
                }
                state.firstDelayElapsedMilliseconds =
                    performance.now() - started;
                if (firstEnumerationGate !== null) {
                    await firstEnumerationGate;
                }
            }
            ++state.enumerateResolvedCount;
            return lists[listIndexAtCall];
        };
        Object.defineProperty(mediaDevices, "enumerateDevices", {
            configurable: false,
            enumerable: false,
            value: enumerateDevices,
            writable: false,
        });
        publishApi({
            dispatchSyntheticOutputlessChange() {
                state.currentListIndex = 1;
                ++state.deviceChangeDispatches;
                deviceChangeDispatchMonotonicMilliseconds =
                    performance.now();
                const event = new Event("devicechange");
                const dispatchReturned =
                    mediaDevices.dispatchEvent(event);
                state.callCountImmediatelyAfterDispatch =
                    state.callCount;
                return Object.freeze({
                    callCountImmediatelyAfterDispatch:
                        state.callCountImmediatelyAfterDispatch,
                    dispatchReturned,
                    isTrusted: event.isTrusted,
                    secondEnumerateStartedAfterDispatchMilliseconds:
                        state
                            .secondEnumerateStartedAfterDispatchMilliseconds,
                });
            },
            releaseFirstEnumeration() {
                if (releaseFirstEnumeration === null) {
                    throw new Error(
                        "first enumeration is not awaiting release",
                    );
                }
                const release = releaseFirstEnumeration;
                releaseFirstEnumeration = null;
                release();
            },
            snapshot,
        });
    }, scenario);
}

function attachMediaDeviceDiagnostics(
    page,
    allowedConsoleRecords = [],
) {
    const diagnostics = {
        blocking: [],
        console: [],
        crashes: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    };
    const diagnosticFailure = createDiagnosticFailureLatch();
    const allowedConsoleCounts = new Map();
    for (const [index, allowed] of allowedConsoleRecords.entries()) {
        validateExactKeys(
            allowed,
            ["maxOccurrences", "text", "type"],
            `allowed console record ${index}`,
        );
        expect(Number.isSafeInteger(allowed.maxOccurrences)).toBe(true);
        expect(allowed.maxOccurrences).toBeGreaterThan(0);
        const duplicate = allowedConsoleRecords.findIndex(
            (candidate, candidateIndex) => (
                candidateIndex < index
                && candidate.text === allowed.text
                && candidate.type === allowed.type
            ),
        );
        expect(duplicate).toBe(-1);
    }
    const findAllowedConsoleIndex = (record) => (
        findAllowedConsoleRecordIndex(allowedConsoleRecords, record)
    );
    const isAllowedConsole = (record) => (
        findAllowedConsoleIndex(record) >= 0
    );
    page.on("console", (message) => {
        const record = {
            text: message.text(),
            type: message.type(),
        };
        if (
            /allow_blocking_on_main_thread|blocking on the main thread|futex/i
                .test(record.text)
        ) {
            diagnostics.blocking.push(record);
            diagnosticFailure.fail("blocking", record);
        }
        if (record.type === "warning" || record.type === "error") {
            diagnostics.console.push(record);
            const allowedIndex = findAllowedConsoleIndex(record);
            if (allowedIndex < 0) {
                diagnosticFailure.fail("console", record);
                return;
            }
            const count = (allowedConsoleCounts.get(allowedIndex) ?? 0) + 1;
            allowedConsoleCounts.set(allowedIndex, count);
            if (
                count
                > allowedConsoleRecords[allowedIndex].maxOccurrences
            ) {
                diagnosticFailure.fail("console-allowlist-exhausted", {
                    count,
                    maxOccurrences:
                        allowedConsoleRecords[allowedIndex].maxOccurrences,
                    ...record,
                });
            }
        }
    });
    page.on("pageerror", (error) => {
        const record = {
            message: error.message,
            name: error.name,
        };
        diagnostics.pageErrors.push(record);
        diagnosticFailure.fail("pageerror", record);
    });
    page.on("requestfailed", (request) => {
        const record = {
            error: request.failure()?.errorText ?? "unknown",
            method: request.method(),
            url: new URL(request.url()).pathname,
        };
        diagnostics.requestFailures.push(record);
        diagnosticFailure.fail("requestfailed", record);
    });
    page.on("response", (response) => {
        if (response.status() >= 400) {
            const record = {
                status: response.status(),
                url: new URL(response.url()).pathname,
            };
            diagnostics.http.push(record);
            diagnosticFailure.fail("http", record);
        }
    });
    page.on("crash", () => {
        const record = { type: "page-crash" };
        diagnostics.crashes.push(record);
        diagnosticFailure.fail("crash", record);
    });
    return {
        diagnosticFailure,
        diagnostics,
        isAllowedConsole,
    };
}

function validateEncodedAudioDeviceLists(payload, label) {
    for (
        const device
        of [
            ...payload.audioInputs,
            ...payload.audioOutputs,
        ]
    ) {
        validateExactKeys(
            device,
            ["idSha256", "isDefault"],
            `${label} identity`,
        );
        expect(device.idSha256).toMatch(/^[0-9a-f]{64}$/);
        expect(typeof device.isDefault).toBe("boolean");
    }
    for (const devices of [
        payload.audioInputs,
        payload.audioOutputs,
    ]) {
        if (devices.length !== 0) {
            expect(
                devices.filter((device) => device.isDefault),
            ).toHaveLength(1);
        }
    }
}

function validateMediaDeviceSnapshots(events, runNonce) {
    const snapshots = events.filter(
        (event) => event.type === "qt-media-device-snapshot",
    );
    expect(snapshots.length).toBeGreaterThan(0);
    expect(snapshots.length).toBeLessThanOrEqual(16);
    for (const [index, event] of snapshots.entries()) {
        validateExactKeys(
            event.payload,
            [
                "audioInputSignalCount",
                "audioInputs",
                "audioOutputSignalCount",
                "audioOutputs",
                "ordinal",
                "reason",
                "runNonce",
            ],
            `media-device snapshot ${index}`,
        );
        expect(Number.isSafeInteger(
            event.payload.audioInputSignalCount,
        )).toBe(true);
        expect(event.payload.audioInputSignalCount)
            .toBeGreaterThanOrEqual(0);
        expect(Number.isSafeInteger(
            event.payload.audioOutputSignalCount,
        )).toBe(true);
        expect(event.payload.audioOutputSignalCount)
            .toBeGreaterThanOrEqual(0);
        expect(event.payload.ordinal).toBe(index + 1);
        expect(event.payload.runNonce).toBe(runNonce);
        expect([
            "audio-inputs-changed",
            "audio-outputs-changed",
            "initial",
        ]).toContain(event.payload.reason);
        validateEncodedAudioDeviceLists(
            event.payload,
            `media-device snapshot ${index}`,
        );
    }
    return snapshots;
}

function validateMediaDeviceSettlements(events, runNonce) {
    const settlements = events.filter(
        (event) => event.type === "qt-media-device-batch-settled",
    );
    expect(settlements.length).toBeGreaterThan(0);
    expect(settlements.length).toBeLessThanOrEqual(16);
    let previousSnapshotOrdinal = 0;
    for (const [index, event] of settlements.entries()) {
        validateExactKeys(
            event.payload,
            [
                "audioInputSignalCount",
                "audioInputs",
                "audioOutputSignalCount",
                "audioOutputs",
                "runNonce",
                "settledOrdinal",
                "snapshotOrdinal",
            ],
            `media-device settlement ${index}`,
        );
        expect(Number.isSafeInteger(
            event.payload.audioInputSignalCount,
        )).toBe(true);
        expect(event.payload.audioInputSignalCount)
            .toBeGreaterThanOrEqual(0);
        expect(Number.isSafeInteger(
            event.payload.audioOutputSignalCount,
        )).toBe(true);
        expect(event.payload.audioOutputSignalCount)
            .toBeGreaterThanOrEqual(0);
        expect(event.payload.runNonce).toBe(runNonce);
        expect(event.payload.settledOrdinal).toBe(index + 1);
        expect(event.payload.snapshotOrdinal)
            .toBeGreaterThanOrEqual(previousSnapshotOrdinal);
        expect(event.payload.snapshotOrdinal).toBeGreaterThan(0);
        previousSnapshotOrdinal = event.payload.snapshotOrdinal;
        validateEncodedAudioDeviceLists(
            event.payload,
            `media-device settlement ${index}`,
        );
    }
    return settlements;
}

async function readMediaDeviceCheckpoint(page) {
    return page.evaluate(() => {
        const events = globalThis.__rhythmGameGate1b.events;
        const snapshots = events.filter(
            (event) => event.type === "qt-media-device-snapshot",
        );
        const settlements = events.filter(
            (event) => (
                event.type === "qt-media-device-batch-settled"
            ),
        );
        return {
            enumerateResolvedCount:
                globalThis.__gate1bMediaDeviceScenario
                    .snapshot().enumerateResolvedCount,
            latestSettlement: structuredClone(
                settlements.at(-1)?.payload ?? null,
            ),
            settlementCount: settlements.length,
            snapshotCount: snapshots.length,
        };
    });
}

async function waitForStableMediaDeviceSettlement(
    page,
    diagnosticFailure,
    criteria,
) {
    await waitForReportOutcome(
        page,
        "Qt media-device batch did not settle",
        {
            forbiddenInputHashes: criteria.forbiddenInputHashes ?? [],
            forbiddenOutputHashes: criteria.forbiddenOutputHashes ?? [],
            kind: "media-device-settled",
            minimumAudioInputSignalCount:
                criteria.minimumAudioInputSignalCount ?? 0,
            minimumAudioOutputSignalCount:
                criteria.minimumAudioOutputSignalCount ?? 0,
            minimumAudioOutputs: criteria.minimumAudioOutputs ?? 0,
            requiredInputHashes: criteria.requiredInputHashes ?? [],
            requiredOutputHashes: criteria.requiredOutputHashes ?? [],
            sequenceBaseline: criteria.sequenceBaseline,
        },
        diagnosticFailure,
    );
    const first = await readMediaDeviceCheckpoint(page);
    await page.evaluate(async () => {
        await new Promise((resolve) => requestAnimationFrame(resolve));
        await new Promise((resolve) => requestAnimationFrame(resolve));
        await new Promise((resolve) => setTimeout(resolve, 0));
    });
    const second = await readMediaDeviceCheckpoint(page);
    expect(second).toEqual(first);
    return second;
}

async function runMediaDeviceScenario({
    afterActivation = null,
    allowedConsoleRecords = [],
    beforeActivation = null,
    browser,
    page,
    scenario,
    testInfo,
}) {
    const lifecycleArgumentAudit = await auditBrowserLifecycleLaunch(
        browser,
        testInfo.project.name,
    );
    const {
        diagnosticFailure,
        diagnostics,
        isAllowedConsole,
    } = attachMediaDeviceDiagnostics(
        page,
        allowedConsoleRecords,
    );
    await installMediaDeviceScenario(page, scenario);
    const navigation = await page.goto(`${probeServer.origin}/`, {
        waitUntil: "domcontentloaded",
    });
    expect(navigation?.status()).toBe(200);
    await waitForJspiSuspension(page, diagnosticFailure);
    if (beforeActivation !== null) {
        await beforeActivation({
            diagnosticFailure,
            page,
        });
    }
    await page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y);
    if (afterActivation !== null) {
        await afterActivation({
            diagnosticFailure,
            page,
        });
    }
    await waitForReportOutcome(
        page,
        "media-device scenario did not reach post-main readiness",
        { kind: "ready" },
        diagnosticFailure,
    );
    const capture = await captureAndAcknowledgeMedia(
        page,
        testInfo,
        diagnosticFailure,
    );
    await waitForTask4Checks(page, diagnosticFailure);
    const report = await readReport(page);
    validateTask4Report(report, capture);
    expect(report.snapshot.failures).toEqual([]);
    expect(
        report.events.filter(
            (event) => event.type === "terminal-failure",
        ),
    ).toEqual([]);
    expect(diagnostics.blocking).toEqual([]);
    expect(diagnostics.crashes).toEqual([]);
    expect(diagnostics.http).toEqual([]);
    expect(diagnostics.pageErrors).toEqual([]);
    expect(diagnostics.requestFailures).toEqual([]);
    expect(
        diagnostics.console.every(
            (record) => isAllowedConsole(record),
        ),
    ).toBe(true);
    return {
        capture,
        diagnosticFailure,
        diagnostics,
        lifecycleArgumentAudit,
        report,
    };
}

test("@core @network @media executes the post-main Qt/Wasm runtime contract", async ({
    browser,
    page,
}, testInfo) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const lifecycleArgumentAudit = await auditBrowserLifecycleLaunch(
        browser,
        testInfo.project.name,
    );
    const diagnostics = {
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    };
    const diagnosticFailure = createDiagnosticFailureLatch();

    page.on("console", (message) => {
        const record = {
            text: message.text(),
            type: message.type(),
        };
        if (message.type() === "warning" || message.type() === "error") {
            diagnostics.console.push(record);
            diagnosticFailure.fail("console", record);
        }
        if (
            /allow_blocking_on_main_thread|blocking on the main thread|futex/i
                .test(message.text())
        ) {
            diagnostics.blocking.push(record);
            diagnosticFailure.fail("blocking", record);
        }
    });
    page.on("pageerror", (error) => {
        const record = {
            message: error.message,
            name: error.name,
        };
        diagnostics.pageErrors.push(record);
        diagnosticFailure.fail("pageerror", record);
    });
    page.on("requestfailed", (request) => {
        const record = {
            error: request.failure()?.errorText ?? "unknown",
            method: request.method(),
            url: new URL(request.url()).pathname,
        };
        diagnostics.requestFailures.push(record);
        diagnosticFailure.fail("requestfailed", record);
    });
    page.on("response", (response) => {
        if (response.status() >= 400) {
            const record = {
                status: response.status(),
                url: new URL(response.url()).pathname,
            };
            diagnostics.http.push(record);
            diagnosticFailure.fail("http", record);
        }
    });
    page.on("crash", () => {
        const record = { type: "page-crash" };
        diagnostics.crashes.push(record);
        diagnosticFailure.fail("crash", record);
    });
    await page.exposeFunction("__recordGate1bCspViolation", (record) => {
        diagnostics.csp.push(record);
        diagnosticFailure.fail("csp", record);
    });
    await page.addInitScript(() => {
        Object.defineProperty(
            globalThis,
            "__gate1bContextMenuObservations",
            {
                configurable: false,
                enumerable: false,
                value: [],
                writable: false,
            },
        );
        globalThis.addEventListener("contextmenu", (event) => {
            globalThis.__gate1bContextMenuObservations.push(Object.freeze({
                defaultPrevented: event.defaultPrevented,
                isTrusted: event.isTrusted,
            }));
        });
        globalThis.addEventListener("securitypolicyviolation", (event) => {
            globalThis.__recordGate1bCspViolation({
                blockedURI: event.blockedURI,
                effectiveDirective: event.effectiveDirective,
                violatedDirective: event.violatedDirective,
            });
        });
    });

    const navigation = await page.goto(`${probeServer.origin}/`, {
        waitUntil: "domcontentloaded",
    });
    expect(navigation?.status()).toBe(200);
    expect(probeServer.requestLogs[0]?.status).toBe(307);
    expect(probeServer.requestLogs[0]?.route).toBe("/");

    await waitForJspiSuspension(page, diagnosticFailure);
    const activationBaseline = await page.evaluate(() => ({
        inputKicks:
            globalThis.__rhythmGameGate1b.eventPump.inputKicks,
        jspiBeforeImportCount:
            globalThis.__rhythmGameGate1b.events.filter(
                (event) => event.type === "jspi-before-import",
            ).length,
        jspiPromiseResolvedCount:
            globalThis.__rhythmGameGate1b.events.filter(
                (event) => event.type === "jspi-promise-resolved",
            ).length,
        pumpInFlight:
            globalThis.__rhythmGameGate1b.eventPump.inFlight,
        sequence:
            globalThis.__rhythmGameGate1b.events.at(-1).sequence,
    }));
    expect(activationBaseline.jspiBeforeImportCount).toBe(1);
    expect(activationBaseline.jspiPromiseResolvedCount).toBe(0);
    expect(activationBaseline.pumpInFlight).toBe(true);
    expect(await verifyNonBubblingWindowWakeups(page)).toEqual({
        inFlightAfter: true,
        inputKickDelta: 2,
        nonBubblingInputKickDelta: 2,
        reentrantInputCallDelta: 2,
    });
    const clipboardTiming = await verifyClipboardDispatchTiming(page);
    validateExactKeys(
        clipboardTiming,
        [
            "defaultPrevented",
            "dispatchReturned",
            "inFlightBefore",
            "inputKickDelta",
            "isTrusted",
            "reentrantInputCallDelta",
        ],
        "suspended same-dispatch clipboard timing",
    );
    expect(clipboardTiming).toEqual({
        defaultPrevented: true,
        dispatchReturned: false,
        inFlightBefore: true,
        inputKickDelta: 1,
        isTrusted: false,
        reentrantInputCallDelta: 1,
    });
    expect(await verifyContextMenuSuppression(page)).toEqual({
        defaultPrevented: true,
        isTrusted: true,
    });
    expect(await page.evaluate(() => ({
        inFlight: globalThis.__rhythmGameGate1b.eventPump.inFlight,
        jspiPromiseResolvedCount:
            globalThis.__rhythmGameGate1b.events.filter(
                (event) => event.type === "jspi-promise-resolved",
            ).length,
    }))).toEqual({
        inFlight: true,
        jspiPromiseResolvedCount: 0,
    });
    expect(await consumeTransientUserActivation(page)).toEqual({
        activeAfter: false,
        activeBefore: true,
        popupOpened: true,
    });
    await page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y);

    const readyOutcome = await waitForReportOutcome(
        page,
        "Gate 1B readiness did not complete",
        {
            kind: "ready",
        },
        diagnosticFailure,
    );
    const readyReport = await page.evaluate(() => (
        structuredClone(
            globalThis.__rhythmGameGate1b.readyResolution,
        )
    ));
    validateExactKeys(
        readyReport,
        ["events", "snapshot"],
        "bootstrap-owned ready resolution record",
    );
    const readyResolvedValue =
        readyOutcome.readyObservation.resolvedValue;
    expect(readyOutcome.readyObservation.resolvedValueIsDeepFrozen)
        .toBe(true);
    validateEventStream(readyReport.events);
    validateSnapshot(readyReport.snapshot);
    validateSnapshot(readyResolvedValue);
    expect(readyResolvedValue).toEqual(readyReport.snapshot);
    validateNoPrivateMaterial(readyResolvedValue);
    const readyQmlRootIndex = eventIndex(
        readyReport.events,
        "qml-root-attached",
    );
    const readyMainReturningIndex = eventIndex(
        readyReport.events,
        "main-returning",
    );
    const readyPostMainIndex = eventIndex(
        readyReport.events,
        "post-main-tick",
    );
    expect(readyQmlRootIndex).toBeGreaterThanOrEqual(0);
    expect(readyMainReturningIndex).toBeGreaterThan(readyQmlRootIndex);
    expect(readyPostMainIndex).toBeGreaterThan(readyMainReturningIndex);
    validatePostMainApplicationState(
        readyReport.events,
        readyPostMainIndex,
    );
    validateRenderFrames(readyReport.events, {
        capture: "post-main",
        eventSequenceBaseline: readyPostMainIndex,
        frameSequenceBaseline: -1,
        generation: 0,
    });
    const readyPingIndex = eventIndex(
        readyReport.events,
        "command-acknowledged",
        (payload) => payload.command === "probe-ping",
    );
    expect(readyPingIndex).toBeGreaterThan(readyMainReturningIndex);
    expect(readyPingIndex).toBeLessThan(readyPostMainIndex);
    validateExactKeys(
        readyReport.events[readyPingIndex].payload,
        ["command", "inputBuildId"],
        "ready probe-ping acknowledgement",
    );
    expect(readyReport.events[readyPingIndex].payload.inputBuildId)
        .toBe(expectedRuntimeBuildId);

    const mediaCapture = await captureAndAcknowledgeMedia(
        page,
        testInfo,
        diagnosticFailure,
        {
            holdMilliseconds: ADVERSARIAL_MEDIA_CAPTURE_HOLD_MS,
        },
    );
    await waitForCoreChecks(page, diagnosticFailure);
    await waitForTask4Checks(page, diagnosticFailure);

    let report = await readReport(page);
    expect(report.instanceAbsent).toBe(true);
    expect(report.schemaVersion).toBe(1);
    validateStyleAdoption(report.styleAdoption);
    validateEventStream(report.events);
    validateSnapshot(report.snapshot);
    for (const check of [...CORE_CHECKS, ...TASK4_CHECKS]) {
        expect(report.snapshot.checks[check]?.passed, check).toBe(true);
    }
    validateNoPrivateMaterial(report.events);

    expect(await inspectReadOnlyReportAuthority(page)).toEqual({
        eventEntriesAreDeepFrozen: true,
        eventLogUnchanged: true,
        eventPreventExtensionsRejected: true,
        eventPushRejected: true,
        eventViewIsArray: true,
        eventPumpDescriptorIsGetter: true,
        eventPumpIsDeepFrozen: true,
        fieldsAreReadOnly: true,
        globalIsFixed: true,
        instanceAbsent: true,
        methodDescriptorsAreFixed: true,
        mutationResults: {
            addInstance: false,
            deleteEvent: false,
            deleteGlobal: false,
            deleteSnapshot: false,
            replaceEvent: false,
            replaceEvents: false,
            replaceEventPump: false,
            replaceGlobal: false,
            replaceReadyResolution: false,
            replaceSnapshot: false,
            replaceStyleAdoption: false,
        },
        nestedStateUnchanged: true,
        readyIsPromise: true,
        reportIsFrozen: true,
        reportEnumerableKeysAreExact: true,
        reportOwnKeysAreExact: true,
        reportReferenceUnchanged: true,
        readyResolutionIsDeepFrozen: true,
        schemaVersion: 1,
        snapshotIsDeepFrozen: true,
        styleAdoptionIsDeepFrozen: true,
    });
    const boundCommand = await verifyRetainedRuntimeCommand(page);
    validateExactKeys(
        boundCommand,
        [
            "command",
            "inputBuildId",
            "instanceAbsent",
        ],
        "retained runtime command result",
    );
    expect(boundCommand.command).toBe("probe-ping");
    expect(boundCommand.inputBuildId).toBe(expectedRuntimeBuildId);
    expect(boundCommand.inputBuildId).toBe(actualRuntimeBuildId);
    expect(boundCommand.instanceAbsent).toBe(true);

    const qmlRootIndex = eventIndex(report.events, "qml-root-attached");
    const mainReturningIndex = eventIndex(report.events, "main-returning");
    const postMainIndex = eventIndex(report.events, "post-main-tick");
    expect(qmlRootIndex).toBeGreaterThanOrEqual(0);
    expect(mainReturningIndex).toBeGreaterThan(qmlRootIndex);
    expect(postMainIndex).toBeGreaterThan(mainReturningIndex);
    validatePostMainApplicationState(report.events, postMainIndex);

    const postMainFrames = validateRenderFrames(report.events, {
        capture: "post-main",
        eventSequenceBaseline: postMainIndex,
        frameSequenceBaseline: -1,
        generation: 0,
    });
    const pingIndex = eventIndex(
        report.events,
        "command-acknowledged",
        (payload) => payload.command === "probe-ping",
    );
    expect(pingIndex).toBeGreaterThan(mainReturningIndex);
    expect(pingIndex).toBeLessThan(postMainIndex);

    const explicit = checkDetail(
        report.snapshot,
        "explicit-pthread",
    );
    const concurrent = checkDetail(
        report.snapshot,
        "qt-concurrent",
    );
    const threadIdentities = [
        explicit.mainThreadId,
        explicit.threadId,
        concurrent.threadId,
    ];
    for (const identity of threadIdentities) {
        expect(identity).toMatch(/^[1-9][0-9]*$/);
    }
    expect(new Set(threadIdentities).size).toBe(3);
    expect(explicit.isMainRuntimeThread).toBe(false);
    expect(concurrent.isMainRuntimeThread).toBe(false);
    expect(explicit.overlapObserved).toBe(true);
    expect(concurrent.overlapObserved).toBe(true);
    expect(explicit.inputNonce).toBeGreaterThan(0);
    expect(explicit.transformedNonce).toBe(
        (explicit.inputNonce ^ PTHREAD_NONCE_XOR) >>> 0,
    );
    expect(concurrent.result).toBe(42);
    const lifecycleMicros = [
        explicit.explicitReadyMicroseconds,
        concurrent.qtConcurrentStartedMicroseconds,
        concurrent.qtConcurrentObservedReadyMicroseconds,
        concurrent.qtConcurrentReleaseMicroseconds,
        explicit.explicitCompletedMicroseconds,
    ];
    for (const micros of lifecycleMicros) {
        expect(Number.isSafeInteger(micros)).toBe(true);
        expect(micros).toBeGreaterThan(0);
    }
    for (let index = 1; index < lifecycleMicros.length; ++index) {
        expect(lifecycleMicros[index]).toBeGreaterThanOrEqual(
            lifecycleMicros[index - 1],
        );
    }

    const applicationCycle = checkDetail(
        report.snapshot,
        "qt-application-cycle-order",
    );
    validateExactKeys(
        applicationCycle,
        ["expectedOrder", "observedOrder"],
        "Qt retained application-cycle order detail",
    );
    expect(applicationCycle.expectedOrder).toEqual([
        "posted",
        "native",
        "timer",
        "deferred-delete",
    ]);
    expect(applicationCycle.observedOrder).toEqual(
        applicationCycle.expectedOrder,
    );

    const jspi = checkDetail(report.snapshot, "jspi-nested-loop");
    validateExactKeys(
        jspi,
        [
            "elapsedMicroseconds",
            "fullPumpDeferredWhilePrimary",
            "nativeComposedPathsIntact",
            "nativeEnterOrder",
            "nativeEventIdentitiesIntact",
            "nativeExitOrder",
            "nativeStackCanariesIntact",
            "postLoopSentinel",
            "primaryStackCanaryIntact",
            "primaryStackCanaryObservedIntact",
            "promiseResolvedWhileExec",
            "quitDelivered",
            "requestedNonce",
            "resolvedNonce",
            "watchdogTimedOut",
        ],
        "JSPI nested-loop detail",
    );
    expect(jspi.requestedNonce).toBeGreaterThan(0);
    expect(jspi.resolvedNonce).toBe(jspi.requestedNonce);
    expect(jspi.promiseResolvedWhileExec).toBe(true);
    expect(jspi.quitDelivered).toBe(true);
    expect(jspi.postLoopSentinel).toBe(true);
    expect(jspi.watchdogTimedOut).toBe(false);
    expect(jspi.fullPumpDeferredWhilePrimary).toBe(true);
    expect(jspi.nativeComposedPathsIntact).toBe(true);
    expect(jspi.nativeEnterOrder).toEqual([1, 2, 3, 4]);
    expect(jspi.nativeEventIdentitiesIntact).toBe(true);
    expect(jspi.nativeExitOrder).toEqual([4, 3, 2, 1]);
    expect(jspi.nativeStackCanariesIntact).toBe(true);
    expect(jspi.primaryStackCanaryIntact).toBe(true);
    expect(jspi.primaryStackCanaryObservedIntact).toBe(true);
    expect(jspi.elapsedMicroseconds).toBeGreaterThan(0);
    expect(jspi.elapsedMicroseconds).toBeGreaterThanOrEqual(1_000);
    validateJspiEvents(report.events, postMainIndex, jspi);

    const exclusiveGuard = checkDetail(
        report.snapshot,
        "qt-exclusive-suspend-guard",
    );
    validateExactKeys(
        exclusiveGuard,
        [
            "completionDrainCount",
            "completionFinalizedAfterOwnerReturn",
            "deliveryOrder",
            "exclusiveClearedBeforeNormalDrain",
            "exclusiveDrainCount",
            "exclusiveDomDispatch",
            "foreignDrainResults",
            "guardObservations",
            "handlerIndices",
            "normalDrainArmed",
            "normalDrainResult",
            "normalObservation",
            "ownerResumedByExclusive",
            "payloadsValid",
            "queuePreserved",
        ],
        "Qt exclusive suspension guard detail",
    );
    validateExactKeys(
        exclusiveGuard.handlerIndices,
        ["completion", "exclusive", "first", "second"],
        "Qt exclusive suspension handler indices",
    );
    const exclusiveHandlerIndices = Object.values(
        exclusiveGuard.handlerIndices,
    );
    for (const handlerIndex of exclusiveHandlerIndices) {
        expect(Number.isSafeInteger(handlerIndex)).toBe(true);
        expect(handlerIndex).toBeGreaterThan(0);
    }
    expect(new Set(exclusiveHandlerIndices).size).toBe(4);
    const {
        completion: completionHandler,
        exclusive: exclusiveHandler,
        first: firstHandler,
        second: secondHandler,
    } = exclusiveGuard.handlerIndices;
    expect(exclusiveGuard.guardObservations).toHaveLength(2);
    const [firstGuard, secondGuard] =
        exclusiveGuard.guardObservations;
    for (const guard of exclusiveGuard.guardObservations) {
        validateExactKeys(
            guard,
            [
                "exclusiveAfter",
                "exclusiveBefore",
                "ordinal",
                "pendingAfter",
                "pendingBefore",
                "result",
            ],
            "Qt exclusive suspension foreign guard",
        );
    }
    expect(firstGuard).toEqual({
        exclusiveAfter: exclusiveHandler,
        exclusiveBefore: exclusiveHandler,
        ordinal: 1,
        pendingAfter: [firstHandler],
        pendingBefore: [firstHandler],
        result: false,
    });
    expect(secondGuard).toEqual({
        exclusiveAfter: exclusiveHandler,
        exclusiveBefore: exclusiveHandler,
        ordinal: 2,
        pendingAfter: [firstHandler, secondHandler],
        pendingBefore: [firstHandler, secondHandler],
        result: false,
    });
    validateExactKeys(
        exclusiveGuard.normalObservation,
        [
            "exclusiveAfter",
            "exclusiveBefore",
            "pendingAfter",
            "pendingBefore",
            "result",
        ],
        "Qt exclusive suspension normal drain",
    );
    expect([
        [],
        [firstHandler, secondHandler],
    ]).toContainEqual(
        exclusiveGuard.normalObservation.pendingBefore,
    );
    expect(exclusiveGuard.normalObservation).toMatchObject({
        exclusiveAfter: 0,
        exclusiveBefore: 0,
        pendingAfter: [],
        result: true,
    });
    expect(exclusiveGuard).toMatchObject({
        completionDrainCount: 1,
        completionFinalizedAfterOwnerReturn: true,
        deliveryOrder: [
            "exclusive",
            "first",
            "second",
            "completion",
        ],
        exclusiveClearedBeforeNormalDrain: true,
        exclusiveDrainCount: 1,
        exclusiveDomDispatch: true,
        foreignDrainResults: [false, false],
        normalDrainArmed: true,
        normalDrainResult: true,
        ownerResumedByExclusive: true,
        payloadsValid: true,
        queuePreserved: true,
    });
    expect(completionHandler).not.toBe(exclusiveHandler);

    const render = checkDetail(report.snapshot, "qt-render-webgl2");
    validateExactKeys(
        render,
        [
            "contextAttributesResult",
            "contextHandle",
            "graphicsApi",
            "majorVersion",
            "postMainFrameCount",
        ],
        "Qt render check detail",
    );
    const finalPostMainPayload = postMainFrames[1].payload;
    expect(render).toEqual({
        contextAttributesResult:
            finalPostMainPayload.contextAttributesResult,
        contextHandle: finalPostMainPayload.contextHandle,
        graphicsApi: finalPostMainPayload.graphicsApi,
        majorVersion: finalPostMainPayload.majorVersion,
        postMainFrameCount: finalPostMainPayload.captureFrameCount,
    });
    expect(render.postMainFrameCount).toBe(2);

    await waitForUserActivation(
        page,
        activationBaseline.sequence,
        diagnosticFailure,
    );
    const activationPump = await page.evaluate(
        () => structuredClone(
            globalThis.__rhythmGameGate1b.eventPump,
        ),
    );
    expect(activationPump.inputKicks).toBeGreaterThanOrEqual(
        activationBaseline.inputKicks + 2,
    );

    const captures = [];
    const phaseFrameContracts = [];
    const guardedAcknowledgement =
        await exerciseRuntimeCommandConcurrencyGuard(
            page,
            diagnosticFailure,
        );
    const guardedReport = await readReport(page);
    const guardedCommandIndex = eventIndex(
        guardedReport.events,
        "command-acknowledged",
        (payload) => (
            payload.command === "set-shader-phase"
            && payload.generation === guardedAcknowledgement.generation
        ),
    );
    expect(guardedCommandIndex).toBeGreaterThan(postMainIndex);
    const guardedFrameContract = {
        capture: "phase",
        eventSequenceBaseline: guardedCommandIndex,
        frameSequenceBaseline: guardedAcknowledgement.frameBaseline,
        generation: guardedAcknowledgement.generation,
    };
    validateRenderFrames(guardedReport.events, guardedFrameContract);
    phaseFrameContracts.push(guardedFrameContract);
    for (const phase of PHASES) {
        const acknowledgement = await setShaderPhase(
            page,
            phase,
            diagnosticFailure,
        );
        const phaseReport = await readReport(page);
        const commandIndex = eventIndex(
            phaseReport.events,
            "command-acknowledged",
            (payload) => (
                payload.command === "set-shader-phase"
                && payload.generation === acknowledgement.generation
            ),
        );
        expect(commandIndex).toBeGreaterThan(postMainIndex);
        const phaseFrameContract = {
            capture: "phase",
            eventSequenceBaseline: commandIndex,
            frameSequenceBaseline: acknowledgement.frameBaseline,
            generation: acknowledgement.generation,
        };
        validateRenderFrames(phaseReport.events, phaseFrameContract);
        phaseFrameContracts.push(phaseFrameContract);
        const canvasCoverage = await inspectQtCanvasCoverage(page);
        expect(canvasCoverage).toEqual(
            SAMPLE_COORDINATES.map(([sampleX, sampleY]) => ({
                canvasClassName: "qt-window-canvas",
                canvasCount: 1,
                canvasParentClassName: "qt-window",
                canvasPointerEvents: "none",
                canvasTagName: "CANVAS",
                compositorHitIsExactCanvas: true,
                compositorNodeName: "CANVAS",
                compositorPseudoType: null,
                hostId: "qt-shadow-container",
                layoutIsQtOwned: true,
                owned: true,
                x: CAPTURE_RECT.x + sampleX,
                y: CAPTURE_RECT.y + sampleY,
            })),
        );
        const pngBytes = await page.screenshot({
            animations: "disabled",
            clip: CAPTURE_RECT,
            type: "png",
        });
        assertCapturePixels(pngBytes, phase);
        captures.push({
            hash: createHash("sha256").update(pngBytes).digest("hex"),
            phase,
        });
    }
    expect(captures[0].hash).not.toBe(captures[1].hash);

    const jspiShape = await inspectOwnedJspiImport(page);
    const ownedImports = jspiShape.imports.filter(
        (entry) => entry.name === OWNED_ASYNC_IMPORT,
    );
    expect(ownedImports).toEqual([
        {
            kind: "function",
            module: "env",
            name: OWNED_ASYNC_IMPORT,
        },
    ]);
    expect(jspiShape.ownedImportHasGeneratedFunction).toBe(true);
    expect(jspiShape.ownedFunctionCancelsOnTerminal).toBe(true);
    expect(jspiShape.ownedFunctionUsesHandleAsync).toBe(true);
    expect(jspiShape.ownedFunctionUsesPromiseRace).toBe(true);
    expect(jspiShape.ownedFunctionOwnsResumeWatchdog).toBe(true);
    expect(jspiShape.ownedFunctionRequiresTrustedPointer).toBe(true);
    expect(jspiShape.importPatternOwnsAsyncJsInWrapper).toBe(true);
    expect(jspiShape.importPatternTestedInWrapper).toBe(true);
    expect(jspiShape.wrapperUsesWebAssemblySuspending).toBe(true);
    expect(jspiShape.buildId).toBe(expectedRuntimeBuildId);
    expect(jspiShape.buildId).toBe(actualRuntimeBuildId);

    await exerciseForegroundPumpLatency(page, diagnosticFailure);
    await exercisePersistedPageLifecycle(page, diagnosticFailure);

    report = await readReport(page);
    validateTask4Report(report, mediaCapture);
    validateEventPump(report.eventPump);
    validateStyleAdoption(report.styleAdoption);
    validateEventStream(report.events);
    validateSnapshot(report.snapshot);
    validateNoPrivateMaterial(report.events);
    expect(
        report.events.filter((event) => event.type === "qt-render-frame"),
    ).toHaveLength(2 + (2 * phaseFrameContracts.length));
    validateRenderFrames(report.events, {
        capture: "post-main",
        eventSequenceBaseline: postMainIndex,
        frameSequenceBaseline: -1,
        generation: 0,
    });
    for (const contract of phaseFrameContracts) {
        validateRenderFrames(report.events, contract);
    }
    const finalJspiEvents = validateJspiEvents(
        report.events,
        postMainIndex,
        jspi,
    );
    const activationEvents = report.events.filter(
        (event) => event.type === "user-activation-sampled",
    );
    expect(activationEvents).toHaveLength(1);
    expect(activationEvents[0].sequence).toBeGreaterThan(
        finalJspiEvents[1].sequence,
    );
    expect(activationEvents[0].sequence).toBeLessThan(
        finalJspiEvents[2].sequence,
    );
    expect(activationEvents[0].payload).toEqual({ active: true });
    expect(
        report.events.filter((event) => event.type === "terminal-failure"),
    ).toEqual([]);
    const finalLiveAuthority = await inspectReadOnlyReportAuthority(page);
    expect(finalLiveAuthority.eventEntriesAreDeepFrozen).toBe(true);
    expect(finalLiveAuthority.snapshotIsDeepFrozen).toBe(true);
    expect(finalLiveAuthority.readyResolutionIsDeepFrozen).toBe(true);

    const provenance = {
        actualRuntimeBuildId,
        browserVersion: browser.version(),
        certificateTrustValidated: false,
        expectedRuntimeBuildId,
        fetchedRuntimeBuildId: jspiShape.buildId,
        lifecycleArgumentAudit,
        project: testInfo.project.name,
        runtimeLeaf,
    };
    validateExactKeys(
        provenance,
        [
            "actualRuntimeBuildId",
            "browserVersion",
            "certificateTrustValidated",
            "expectedRuntimeBuildId",
            "fetchedRuntimeBuildId",
            "lifecycleArgumentAudit",
            "project",
            "runtimeLeaf",
        ],
        "core provenance",
    );
    await testInfo.attach("gate1b-core-provenance", {
        body: Buffer.from(JSON.stringify(provenance)),
        contentType: "application/json",
    });
    await testInfo.attach("gate1b-core-server-requests", {
        body: Buffer.from(JSON.stringify(probeServer.requestLogs)),
        contentType: "application/json",
    });

    const faviconRequests = probeServer.requestLogs.filter(
        (record) => record.route === "/favicon.ico",
    );
    expect(faviconRequests).toEqual([]);
    expect(diagnostics).toEqual({
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    });
});

test("@media-devices preserves an OpenAL fallback without mediaDevices", async ({
    browser,
    page,
}, testInfo) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const outcome = await runMediaDeviceScenario({
        allowedConsoleRecords: [{
            maxOccurrences: 1,
            text: "No media devices found",
            type: "warning",
        }],
        browser,
        page,
        scenario: {
            mode: "unavailable",
        },
        testInfo,
    });
    const scenarioState = await page.evaluate(
        () => globalThis.__gate1bMediaDeviceScenario.snapshot(),
    );
    expect(scenarioState).toEqual({
        callCount: 0,
        callCountImmediatelyAfterDispatch: -1,
        currentListIndex: -1,
        deviceChangeDispatches: 0,
        enumerateResolvedCount: 0,
        firstDelayElapsedMilliseconds: 0,
        firstDelayTurns: 0,
        mode: "unavailable",
        secondEnumerateStartedAfterDispatchMilliseconds: -1,
    });
    const snapshots = validateMediaDeviceSnapshots(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    validateMediaDeviceSettlements(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    expect(
        snapshots.every(
            (event) => (
                event.payload.audioInputs.length >= 1
                && event.payload.audioOutputs.length >= 1
            ),
        ),
    ).toBe(true);
    expect(outcome.diagnostics.console).toEqual([{
        text: "No media devices found\n",
        type: "warning",
    }]);
});

test("@media-devices preserves immediate audio-input-first enumeration", async ({
    browser,
    page,
}, testInfo) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const inputId = "input-first";
    const outputId = "output-second";
    const inputHash = sha256(Buffer.from(inputId, "utf8"));
    const outputHash = sha256(Buffer.from(outputId, "utf8"));
    let initialCheckpoint;
    const firstList = [
        {
            deviceId: inputId,
            groupId: "ordered-group",
            kind: "audioinput",
            label: "Input First",
        },
        {
            deviceId: outputId,
            groupId: "ordered-group",
            kind: "audiooutput",
            label: "Output Second",
        },
    ];
    const outcome = await runMediaDeviceScenario({
        afterActivation: async ({
            diagnosticFailure,
            page: scenarioPage,
        }) => {
            initialCheckpoint = await waitForStableMediaDeviceSettlement(
                scenarioPage,
                diagnosticFailure,
                {
                    requiredInputHashes: [inputHash],
                    requiredOutputHashes: [outputHash],
                    sequenceBaseline: -1,
                },
            );
        },
        browser,
        page,
        scenario: {
            deviceLists: [firstList, firstList],
            initialDelayMilliseconds: 0,
            initialDelayTurns: 0,
            mode: "controlled",
        },
        testInfo,
    });
    const state = await page.evaluate(
        () => globalThis.__gate1bMediaDeviceScenario.snapshot(),
    );
    expect(state).toEqual({
        callCount: 1,
        callCountImmediatelyAfterDispatch: -1,
        currentListIndex: 0,
        deviceChangeDispatches: 0,
        enumerateResolvedCount: 1,
        firstDelayElapsedMilliseconds:
            state.firstDelayElapsedMilliseconds,
        firstDelayTurns: 0,
        mode: "controlled",
        secondEnumerateStartedAfterDispatchMilliseconds: -1,
    });
    expect(state.firstDelayElapsedMilliseconds).toBeGreaterThanOrEqual(0);
    expect(state.firstDelayElapsedMilliseconds).toBeLessThanOrEqual(50);
    expect(initialCheckpoint).toBeDefined();
    const settlement = initialCheckpoint.latestSettlement;
    expect(
        settlement.audioInputs.find(
            (device) => device.idSha256 === inputHash,
        )?.isDefault,
    ).toBe(false);
    expect(
        settlement.audioOutputs.find(
            (device) => device.idSha256 === outputHash,
        )?.isDefault,
    ).toBe(false);
    validateMediaDeviceSnapshots(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    validateMediaDeviceSettlements(
        outcome.report.events,
        outcome.capture.runNonce,
    );
});

test("@media-devices coalesces rapid changes while enumeration is pending", async ({
    browser,
    page,
}, testInfo) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const initialInputId = "coalesced-input-a";
    const initialOutputId = "coalesced-output-a";
    const changedInputId = "coalesced-input-b";
    const changedOutputId = "coalesced-output-b";
    const initialInputHash = sha256(Buffer.from(initialInputId, "utf8"));
    const initialOutputHash = sha256(Buffer.from(initialOutputId, "utf8"));
    const changedInputHash = sha256(Buffer.from(changedInputId, "utf8"));
    const changedOutputHash = sha256(Buffer.from(changedOutputId, "utf8"));
    let finalCheckpoint;
    let releasedState;
    const outcome = await runMediaDeviceScenario({
        afterActivation: async ({
            diagnosticFailure,
            page: scenarioPage,
        }) => {
            await scenarioPage.evaluate(() => {
                globalThis.__gate1bMediaDeviceScenario
                    .releaseFirstEnumeration();
            });
            await waitForReportOutcome(
                scenarioPage,
                "coalesced browser enumeration did not resolve",
                {
                    kind: "media-device-browser-enumeration",
                    resolvedCount: 2,
                },
                diagnosticFailure,
            );
            releasedState = await scenarioPage.evaluate(
                () => globalThis.__gate1bMediaDeviceScenario.snapshot(),
            );
            expect(releasedState).toMatchObject({
                callCount: 2,
                callCountImmediatelyAfterDispatch: 1,
                currentListIndex: 1,
                deviceChangeDispatches: 2,
                enumerateResolvedCount: 2,
                mode: "controlled",
            });
            expect(
                releasedState
                    .secondEnumerateStartedAfterDispatchMilliseconds,
            ).toBeGreaterThanOrEqual(0);
            finalCheckpoint = await waitForStableMediaDeviceSettlement(
                scenarioPage,
                diagnosticFailure,
                {
                    forbiddenInputHashes: [initialInputHash],
                    forbiddenOutputHashes: [initialOutputHash],
                    requiredInputHashes: [changedInputHash],
                    requiredOutputHashes: [changedOutputHash],
                    sequenceBaseline: -1,
                },
            );
        },
        beforeActivation: async ({
            diagnosticFailure,
            page: scenarioPage,
        }) => {
            await scenarioPage.waitForFunction(() => {
                const scenario =
                    globalThis.__gate1bMediaDeviceScenario;
                if (typeof scenario?.snapshot !== "function") {
                    return false;
                }
                const state = scenario.snapshot();
                return (
                    state.callCount === 1
                    && state.enumerateResolvedCount === 0
                );
            });
            const observations = await scenarioPage.evaluate(() => {
                const scenario =
                    globalThis.__gate1bMediaDeviceScenario;
                return [
                    scenario.dispatchSyntheticOutputlessChange(),
                    scenario.dispatchSyntheticOutputlessChange(),
                ];
            });
            expect(observations).toHaveLength(2);
            for (const observation of observations) {
                validateExactKeys(
                    observation,
                    [
                        "callCountImmediatelyAfterDispatch",
                        "dispatchReturned",
                        "isTrusted",
                        "secondEnumerateStartedAfterDispatchMilliseconds",
                    ],
                    "coalesced devicechange dispatch",
                );
                expect(observation).toMatchObject({
                    callCountImmediatelyAfterDispatch: 1,
                    dispatchReturned: true,
                    isTrusted: false,
                    secondEnumerateStartedAfterDispatchMilliseconds: -1,
                });
            }
            const heldState = await scenarioPage.evaluate(
                () => globalThis.__gate1bMediaDeviceScenario.snapshot(),
            );
            expect(heldState).toMatchObject({
                callCount: 1,
                currentListIndex: 1,
                deviceChangeDispatches: 2,
                enumerateResolvedCount: 0,
            });
        },
        browser,
        page,
        scenario: {
            deviceLists: [
                [
                    {
                        deviceId: initialInputId,
                        groupId: "coalesced-group-a",
                        kind: "audioinput",
                        label: "Coalesced Input A",
                    },
                    {
                        deviceId: initialOutputId,
                        groupId: "coalesced-group-a",
                        kind: "audiooutput",
                        label: "Coalesced Output A",
                    },
                ],
                [
                    {
                        deviceId: changedInputId,
                        groupId: "coalesced-group-b",
                        kind: "audioinput",
                        label: "Coalesced Input B",
                    },
                    {
                        deviceId: changedOutputId,
                        groupId: "coalesced-group-b",
                        kind: "audiooutput",
                        label: "Coalesced Output B",
                    },
                ],
            ],
            holdFirstEnumeration: true,
            initialDelayMilliseconds: 0,
            initialDelayTurns: 0,
            mode: "controlled",
        },
        testInfo,
    });
    expect(releasedState).toBeDefined();
    expect(finalCheckpoint).toBeDefined();
    expect(finalCheckpoint.latestSettlement.audioInputs).toContainEqual({
        idSha256: changedInputHash,
        isDefault: false,
    });
    expect(finalCheckpoint.latestSettlement.audioOutputs).toContainEqual({
        idSha256: changedOutputHash,
        isDefault: false,
    });
    validateMediaDeviceSnapshots(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    validateMediaDeviceSettlements(
        outcome.report.events,
        outcome.capture.runNonce,
    );
});

test("@media-devices survives delayed input-first and outputless change", async ({
    browser,
    page,
}, testInfo) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const initialInputId = "input-a";
    const initialOutputId = "output-a";
    const changedInputId = "input-b";
    const initialInputHash = sha256(Buffer.from(initialInputId, "utf8"));
    const initialOutputHash = sha256(Buffer.from(initialOutputId, "utf8"));
    const changedInputHash = sha256(Buffer.from(changedInputId, "utf8"));
    let deviceChangeSequenceBaseline;
    let fallbackInputHash;
    let fallbackOutputHash;
    let initialCheckpoint;
    let initialState;
    let changedState;
    let changedCheckpoint;
    const outcome = await runMediaDeviceScenario({
        afterActivation: async ({
            diagnosticFailure,
            page: scenarioPage,
        }) => {
            initialCheckpoint = await waitForStableMediaDeviceSettlement(
                scenarioPage,
                diagnosticFailure,
                {
                    requiredInputHashes: [initialInputHash],
                    requiredOutputHashes: [initialOutputHash],
                    sequenceBaseline: -1,
                },
            );
            const initialInputs =
                initialCheckpoint.latestSettlement.audioInputs;
            const initialOutputs =
                initialCheckpoint.latestSettlement.audioOutputs;
            expect(initialInputs).toHaveLength(2);
            expect(initialOutputs).toHaveLength(2);
            const controlledInput = initialInputs.find(
                (device) => device.idSha256 === initialInputHash,
            );
            const controlledOutput = initialOutputs.find(
                (device) => device.idSha256 === initialOutputHash,
            );
            expect(controlledInput?.isDefault).toBe(false);
            expect(controlledOutput?.isDefault).toBe(false);
            const fallbackInput = initialInputs.find(
                (device) => device.idSha256 !== initialInputHash,
            );
            const fallbackOutput = initialOutputs.find(
                (device) => device.idSha256 !== initialOutputHash,
            );
            expect(fallbackInput?.isDefault).toBe(true);
            expect(fallbackOutput?.isDefault).toBe(true);
            fallbackInputHash = fallbackInput.idSha256;
            fallbackOutputHash = fallbackOutput.idSha256;

            deviceChangeSequenceBaseline = await scenarioPage.evaluate(
                () => (
                    globalThis.__rhythmGameGate1b.events.at(-1).sequence
                ),
            );
            const dispatchObservation = await scenarioPage.evaluate(
                () => (
                    globalThis.__gate1bMediaDeviceScenario
                        .dispatchSyntheticOutputlessChange()
                ),
            );
            validateExactKeys(
                dispatchObservation,
                [
                    "callCountImmediatelyAfterDispatch",
                    "dispatchReturned",
                    "isTrusted",
                    "secondEnumerateStartedAfterDispatchMilliseconds",
                ],
                "synthetic devicechange dispatch",
            );
            expect(dispatchObservation).toMatchObject({
                callCountImmediatelyAfterDispatch: 1,
                dispatchReturned: true,
                isTrusted: false,
                secondEnumerateStartedAfterDispatchMilliseconds: -1,
            });
            changedCheckpoint = await waitForStableMediaDeviceSettlement(
                scenarioPage,
                diagnosticFailure,
                {
                    forbiddenInputHashes: [initialInputHash],
                    forbiddenOutputHashes: [initialOutputHash],
                    minimumAudioInputSignalCount:
                        initialCheckpoint.latestSettlement
                            .audioInputSignalCount + 1,
                    minimumAudioOutputSignalCount:
                        initialCheckpoint.latestSettlement
                            .audioOutputSignalCount + 1,
                    minimumAudioOutputs: 1,
                    requiredInputHashes: [changedInputHash],
                    sequenceBaseline: deviceChangeSequenceBaseline,
                },
            );
            changedState = await scenarioPage.evaluate(
                () => (
                    globalThis.__gate1bMediaDeviceScenario.snapshot()
                ),
            );
            expect(changedState).toMatchObject({
                callCount: 2,
                callCountImmediatelyAfterDispatch: 1,
                currentListIndex: 1,
                deviceChangeDispatches: 1,
                enumerateResolvedCount: 2,
                firstDelayTurns: 4,
                mode: "controlled",
            });
            expect(
                changedState
                    .secondEnumerateStartedAfterDispatchMilliseconds,
            ).toBeGreaterThanOrEqual(0);
            expect(
                changedState
                    .secondEnumerateStartedAfterDispatchMilliseconds,
            ).toBeLessThanOrEqual(50);
        },
        beforeActivation: async ({
            diagnosticFailure,
            page: scenarioPage,
        }) => {
            await waitForReportOutcome(
                scenarioPage,
                "delayed browser enumeration did not resolve",
                {
                    kind: "media-device-browser-enumeration",
                    resolvedCount: 1,
                },
                diagnosticFailure,
            );
            initialState = await scenarioPage.evaluate(
                () => (
                    globalThis.__gate1bMediaDeviceScenario.snapshot()
                ),
            );
            validateExactKeys(
                initialState,
                [
                    "callCount",
                    "callCountImmediatelyAfterDispatch",
                    "currentListIndex",
                    "deviceChangeDispatches",
                    "enumerateResolvedCount",
                    "firstDelayElapsedMilliseconds",
                    "firstDelayTurns",
                    "mode",
                    "secondEnumerateStartedAfterDispatchMilliseconds",
                ],
                "delayed media-device pre-activation state",
            );
            expect(initialState).toMatchObject({
                callCount: 1,
                callCountImmediatelyAfterDispatch: -1,
                currentListIndex: 0,
                deviceChangeDispatches: 0,
                enumerateResolvedCount: 1,
                firstDelayTurns: 4,
                mode: "controlled",
                secondEnumerateStartedAfterDispatchMilliseconds: -1,
            });
            expect(initialState.firstDelayElapsedMilliseconds)
                .toBeGreaterThanOrEqual(125);
        },
        browser,
        page,
        scenario: {
            deviceLists: [
                [
                    {
                        deviceId: initialInputId,
                        groupId: "group-a",
                        kind: "audioinput",
                        label: "Input A",
                    },
                    {
                        deviceId: initialOutputId,
                        groupId: "group-a",
                        kind: "audiooutput",
                        label: "Output A",
                    },
                ],
                [
                    {
                        deviceId: changedInputId,
                        groupId: "group-b",
                        kind: "audioinput",
                        label: "Input B",
                    },
                ],
            ],
            initialDelayMilliseconds: 125,
            initialDelayTurns: 4,
            mode: "controlled",
        },
        testInfo,
    });
    const initialSnapshots = validateMediaDeviceSnapshots(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    const settlements = validateMediaDeviceSettlements(
        outcome.report.events,
        outcome.capture.runNonce,
    );
    expect(initialSnapshots.some((event) => (
        event.payload.audioInputs.some(
            (device) => device.idSha256 === initialInputHash,
        )
        && event.payload.audioOutputs.some(
            (device) => device.idSha256 === initialOutputHash,
        )
    ))).toBe(true);
    expect(initialState).toBeDefined();
    expect(initialCheckpoint).toBeDefined();
    expect(changedState).toBeDefined();
    expect(changedCheckpoint).toBeDefined();
    expect(fallbackInputHash).toMatch(/^[0-9a-f]{64}$/);
    expect(fallbackOutputHash).toMatch(/^[0-9a-f]{64}$/);
    expect(await readMediaDeviceCheckpoint(page)).toEqual(
        changedCheckpoint,
    );
    const changedSettlement = changedCheckpoint.latestSettlement;
    expect(changedSettlement.audioInputSignalCount).toBe(
        initialCheckpoint.latestSettlement.audioInputSignalCount + 1,
    );
    expect(changedSettlement.audioOutputSignalCount).toBe(
        initialCheckpoint.latestSettlement.audioOutputSignalCount + 1,
    );
    expect(changedSettlement.audioInputs).toHaveLength(2);
    expect(changedSettlement.audioOutputs).toHaveLength(1);
    expect(changedSettlement.audioInputs).toContainEqual({
        idSha256: changedInputHash,
        isDefault: false,
    });
    expect(changedSettlement.audioInputs).toContainEqual({
        idSha256: fallbackInputHash,
        isDefault: true,
    });
    expect(changedSettlement.audioOutputs).toEqual([{
        idSha256: fallbackOutputHash,
        isDefault: true,
    }]);
    const postChangeSnapshotReasons = outcome.report.events
        .filter((event) => (
            event.type === "qt-media-device-snapshot"
            && event.sequence > deviceChangeSequenceBaseline
        ))
        .map((event) => event.payload.reason);
    expect(postChangeSnapshotReasons).toContain(
        "audio-inputs-changed",
    );
    expect(postChangeSnapshotReasons).toContain(
        "audio-outputs-changed",
    );
    expect(settlements.some((event) => (
        event.payload.audioInputs.some(
            (device) => device.idSha256 === changedInputHash,
        )
        && !event.payload.audioInputs.some(
            (device) => device.idSha256 === initialInputHash,
        )
        && event.payload.audioOutputs.length >= 1
        && !event.payload.audioOutputs.some(
            (device) => device.idSha256 === initialOutputHash,
        )
    ))).toBe(true);
    const changedSettlementIndex = outcome.report.events.findIndex(
        (event) => (
            event.type === "qt-media-device-batch-settled"
            && event.sequence > deviceChangeSequenceBaseline
            && event.payload.audioInputs.some(
                (device) => device.idSha256 === changedInputHash,
            )
            && !event.payload.audioOutputs.some(
                (device) => device.idSha256 === initialOutputHash,
            )
        ),
    );
    const initialSettlementIndex = outcome.report.events.findIndex(
        (event) => (
            event.type === "qt-media-device-batch-settled"
            && event.sequence <= deviceChangeSequenceBaseline
            && event.payload.audioInputs.some(
                (device) => device.idSha256 === initialInputHash,
            )
            && event.payload.audioOutputs.some(
                (device) => device.idSha256 === initialOutputHash,
            )
        ),
    );
    const activationIndex = eventIndex(
        outcome.report.events,
        "user-activation-sampled",
    );
    const sourceSetIndex = eventIndex(
        outcome.report.events,
        "qt-media-source-set",
    );
    const playerOutputCreatedIndex = eventIndex(
        outcome.report.events,
        "qt-media-player-output-created",
    );
    const playerOutputCreated = outcome.report.events[
        playerOutputCreatedIndex
    ].payload;
    expect(changedSettlementIndex).toBeGreaterThanOrEqual(0);
    expect(initialSettlementIndex).toBeGreaterThanOrEqual(0);
    expect(initialSettlementIndex).toBeLessThan(changedSettlementIndex);
    expect(activationIndex).toBeLessThan(playerOutputCreatedIndex);
    expect(playerOutputCreatedIndex).toBeLessThan(sourceSetIndex);
    expect(sourceSetIndex).toBeLessThan(changedSettlementIndex);
    expect(playerOutputCreated.outputDeviceIdSha256).toBe(
        fallbackOutputHash,
    );
    expect(playerOutputCreated.outputDeviceIsDefault).toBe(true);
    expect(
        outcome.report.events.filter(
            (event) => event.type === "terminal-failure",
        ),
    ).toEqual([]);
    expect(outcome.diagnostics.blocking).toEqual([]);
    expect(outcome.diagnostics.console).toEqual([]);
    expect(outcome.diagnostics.crashes).toEqual([]);
    expect(outcome.diagnostics.http).toEqual([]);
    expect(outcome.diagnostics.pageErrors).toEqual([]);
    expect(outcome.diagnostics.requestFailures).toEqual([]);
});

test("@core @headed proves real hidden and visible Chrome delivery", async (
    {},
    testInfo,
) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    expect(["chrome-stable", "chrome-beta"]).toContain(
        testInfo.project.name,
    );
    const external = await launchExternalLifecycleBrowser(
        testInfo.project.name,
    );
    try {
        const { browser, page } = external;
        validateExactKeys(
            external.lifecycleArgumentAudit,
            ["requiredAbsent", "requiredPresent", "verifiedVia"],
            "external lifecycle argument audit",
        );
        expect(external.lifecycleArgumentAudit.requiredPresent).toEqual([
            "--enable-automation",
        ]);
        expect(external.lifecycleArgumentAudit.requiredAbsent).toEqual([
            "--disable-background-timer-throttling",
            "--disable-backgrounding-occluded-windows",
            "--disable-back-forward-cache",
            "--disable-renderer-backgrounding",
        ]);
        expect(external.profileMode).toBe("temporary-external-cdp");

        const diagnostics = {
            console: [],
            crashes: [],
            http: [],
            pageErrors: [],
            requestFailures: [],
        };
        const diagnosticFailure = createDiagnosticFailureLatch();
        page.on("console", (message) => {
            if (
                message.type() !== "warning"
                && message.type() !== "error"
            ) {
                return;
            }
            const record = {
                text: message.text(),
                type: message.type(),
            };
            diagnostics.console.push(record);
            diagnosticFailure.fail("console", record);
        });
        page.on("pageerror", (error) => {
            const record = {
                message: error.message,
                name: error.name,
            };
            diagnostics.pageErrors.push(record);
            diagnosticFailure.fail("pageerror", record);
        });
        page.on("requestfailed", (request) => {
            const record = {
                error: request.failure()?.errorText ?? "unknown",
                method: request.method(),
                url: new URL(request.url()).pathname,
            };
            diagnostics.requestFailures.push(record);
            diagnosticFailure.fail("requestfailed", record);
        });
        page.on("response", (response) => {
            if (response.status() < 400) {
                return;
            }
            const record = {
                status: response.status(),
                url: new URL(response.url()).pathname,
            };
            diagnostics.http.push(record);
            diagnosticFailure.fail("http", record);
        });
        page.on("crash", () => {
            const record = { type: "page-crash" };
            diagnostics.crashes.push(record);
            diagnosticFailure.fail("crash", record);
        });

        const navigation = await page.goto(`${probeServer.origin}/`, {
            waitUntil: "domcontentloaded",
        });
        expect(navigation?.status()).toBe(200);
        await waitForJspiSuspension(page, diagnosticFailure);
        await page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y);
        await waitForReportOutcome(
            page,
            "headed lifecycle runtime readiness did not complete",
            { kind: "ready" },
            diagnosticFailure,
        );
        const mediaCapture = await captureAndAcknowledgeMedia(
            page,
            testInfo,
            diagnosticFailure,
        );
        await waitForCoreChecks(page, diagnosticFailure);
        await waitForTask4Checks(page, diagnosticFailure);
        await exerciseHiddenPageFallback(page);

        const report = await readReport(page);
        validateEventStream(report.events);
        validateSnapshot(report.snapshot);
        validateNoPrivateMaterial(report.events);
        validateTask4Report(report, mediaCapture);
        expect(report.eventPump.hiddenIdleTimers).toBeGreaterThanOrEqual(1);
        expect(report.eventPump.hiddenQtTimerPumpSerial).toBeGreaterThan(0);
        expect(report.eventPump.hiddenQtTimerSentinels).toBe(1);
        expect(report.eventPump.resumedQtTimerPumpSerial).toBeGreaterThan(0);
        expect(report.eventPump.resumedQtTimerSentinels).toBe(1);
        expect(report.eventPump.visibilityReschedules).toBeGreaterThanOrEqual(
            2,
        );
        expect(report.eventPump.lifecyclePaused).toBe(false);
        expect(report.eventPump.stopped).toBe(false);
        expect(
            report.events.filter(
                (event) => event.type === "terminal-failure",
            ),
        ).toEqual([]);

        const provenance = {
            actualHeaded: true,
            browserVersion: browser.version(),
            certificateTrustValidated: false,
            channel: external.identity.channel,
            executableSha256: external.identity.executableSha256,
            lifecycleArgumentAudit:
                external.lifecycleArgumentAudit,
            profileMode: external.profileMode,
            project: testInfo.project.name,
        };
        validateExactKeys(
            provenance,
            [
                "actualHeaded",
                "browserVersion",
                "certificateTrustValidated",
                "channel",
                "executableSha256",
                "lifecycleArgumentAudit",
                "profileMode",
                "project",
            ],
            "headed lifecycle provenance",
        );
        expect(provenance.browserVersion).toBe(
            external.identity.version,
        );
        await testInfo.attach("gate1b-headed-lifecycle-provenance", {
            body: Buffer.from(JSON.stringify(provenance)),
            contentType: "application/json",
        });
        expect(diagnostics).toEqual({
            console: [],
            crashes: [],
            http: [],
            pageErrors: [],
            requestFailures: [],
        });
    } finally {
        await external.close();
    }
});

async function reachReadyAdversarialRuntime(page, mode) {
    const diagnostics = {
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    };
    const diagnosticFailure = createDiagnosticFailureLatch();
    page.on("console", (message) => {
        const record = {
            text: message.text(),
            type: message.type(),
        };
        if (message.type() === "warning" || message.type() === "error") {
            diagnostics.console.push(record);
            diagnosticFailure.fail("console", record);
        }
        if (
            /allow_blocking_on_main_thread|blocking on the main thread|futex/i
                .test(message.text())
        ) {
            diagnostics.blocking.push(record);
            diagnosticFailure.fail("blocking", record);
        }
    });
    page.on("pageerror", (error) => {
        const record = {
            message: error.message,
            name: error.name,
        };
        diagnostics.pageErrors.push(record);
        diagnosticFailure.fail("pageerror", record);
    });
    page.on("requestfailed", (request) => {
        const record = {
            error: request.failure()?.errorText ?? "unknown",
            method: request.method(),
            url: new URL(request.url()).pathname,
        };
        diagnostics.requestFailures.push(record);
        diagnosticFailure.fail("requestfailed", record);
    });
    page.on("response", (response) => {
        if (response.status() >= 400) {
            const record = {
                status: response.status(),
                url: new URL(response.url()).pathname,
            };
            diagnostics.http.push(record);
            diagnosticFailure.fail("http", record);
        }
    });
    page.on("crash", () => {
        const record = { type: "page-crash" };
        diagnostics.crashes.push(record);
        diagnosticFailure.fail("crash", record);
    });
    await page.exposeFunction("__recordGate1bCspViolation", (record) => {
        diagnostics.csp.push(record);
        diagnosticFailure.fail("csp", record);
    });
    await page.addInitScript(() => {
        globalThis.addEventListener("securitypolicyviolation", (event) => {
            globalThis.__recordGate1bCspViolation({
                blockedURI: event.blockedURI,
                effectiveDirective: event.effectiveDirective,
                violatedDirective: event.violatedDirective,
            });
        });
    });

    const navigation = await page.goto(
        `${probeServer.origin}/negative/${mode}/`,
        { waitUntil: "domcontentloaded" },
    );
    expect(navigation?.status()).toBe(200);
    await waitForJspiSuspension(page, diagnosticFailure);
    await page.mouse.click(BUTTON_POINT.x, BUTTON_POINT.y);
    await waitForReportOutcome(
        page,
        "Adversarial runtime readiness did not complete",
        {
            kind: "ready",
        },
        diagnosticFailure,
    );
    await captureAndAcknowledgeMedia(
        page,
        null,
        diagnosticFailure,
    );
    await waitForCoreChecks(page, diagnosticFailure);
    await waitForTask4Checks(page, diagnosticFailure);
    await waitForEventPumpIdle(page, diagnosticFailure);
    expect(await page.evaluate(() => (
        {
            inFlight: globalThis.__rhythmGameGate1b.eventPump.inFlight,
            maxNativeDispatchDepth:
                globalThis.__rhythmGameGate1b.eventPump
                    .maxNativeDispatchDepth,
            phase: globalThis.__rhythmGameGate1b.snapshot.phase,
        }
    ))).toEqual({
        inFlight: false,
        maxNativeDispatchDepth: 4,
        phase: "core-complete",
    });
    expect(diagnostics).toEqual({
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    });
    return {
        diagnosticFailure,
        diagnostics,
    };
}

async function triggerExpectedNativeBoundaryTerminal(
    page,
    command,
    attemptType,
    expectedCode,
    forbiddenType = null,
) {
    const acknowledgement = await page.evaluate(
        async (requestedCommand) => (
            globalThis.__rhythmGameGate1b.command(requestedCommand, {})
        ),
        command,
    );
    expect(acknowledgement).toEqual({
        armed: true,
        command,
    });
    const terminalHandle = await page.waitForFunction(
        (requested) => {
            const report = globalThis.__rhythmGameGate1b;
            const forbidden = requested.forbiddenType === null
                ? undefined
                : report?.events?.find(
                    (candidate) => (
                        candidate.type === requested.forbiddenType
                    ),
                );
            if (forbidden !== undefined) {
                return {
                    forbidden,
                    state: "forbidden",
                };
            }
            const terminalEvents = report?.events?.filter(
                (candidate) => candidate.type === "terminal-failure",
            ) ?? [];
            const event = terminalEvents.find(
                (candidate) => (
                    candidate.payload?.code === requested.code
                ),
            );
            if (event === undefined) {
                return undefined;
            }
            const attempt = report.events.find(
                (candidate) => candidate.type === requested.attemptType,
            );
            if (attempt === undefined) {
                return undefined;
            }
            if (
                report.eventPump.inFlight !== false
                || report.eventPump.stopped !== true
            ) {
                return undefined;
            }
            return {
                attempt,
                attemptCount: report.events.filter(
                    (candidate) => candidate.type === requested.attemptType,
                ).length,
                calls: report.eventPump.calls,
                commandKicks: report.eventPump.commandKicks,
                event,
                eventCount: report.events.length,
                inFlight: report.eventPump.inFlight,
                inputKicks: report.eventPump.inputKicks,
                stopped: report.eventPump.stopped,
                state: "terminal",
                terminalCount: terminalEvents.length,
            };
        },
        {
            attemptType,
            code: expectedCode,
            forbiddenType,
        },
        {
            polling: "raf",
            timeout: EXPECT_TIMEOUT_MS,
        },
    );
    try {
        const outcome = await terminalHandle.jsonValue();
        expect(outcome.state).toBe("terminal");
        return outcome;
    } finally {
        await terminalHandle.dispose();
    }
}

async function assertNativeBoundaryTerminalQuiescent(page, terminal) {
    expect(terminal).toMatchObject({
        attemptCount: 1,
        inFlight: false,
        stopped: true,
        terminalCount: 1,
    });
    const postTerminalCommand = await page.evaluate(async () => {
        try {
            await globalThis.__rhythmGameGate1b.command("probe-ping", {});
            return { resolved: true };
        } catch (error) {
            return {
                message: String(error),
                resolved: false,
            };
        }
    });
    expect(postTerminalCommand).toEqual({
        message: "Error: gate1b-terminal-latched",
        resolved: false,
    });
    await page.evaluate(() => {
        const screen = document.getElementById("screen");
        screen.dispatchEvent(new Event("beforeinput", {
            bubbles: true,
            composed: true,
        }));
        return new Promise((resolve) => {
            setTimeout(() => {
                requestAnimationFrame(() => requestAnimationFrame(resolve));
            }, 100);
        });
    });
    expect(await page.evaluate(() => {
        const report = globalThis.__rhythmGameGate1b;
        return {
            calls: report.eventPump.calls,
            commandKicks: report.eventPump.commandKicks,
            eventCount: report.events.length,
            inFlight: report.eventPump.inFlight,
            inputKicks: report.eventPump.inputKicks,
            stopped: report.eventPump.stopped,
            terminalCount: report.events.filter(
                (event) => event.type === "terminal-failure",
            ).length,
        };
    })).toEqual({
        calls: terminal.calls,
        commandKicks: terminal.commandKicks,
        eventCount: terminal.eventCount,
        inFlight: false,
        inputKicks: terminal.inputKicks,
        stopped: true,
        terminalCount: 1,
    });
}

test("@core rejects a fifth synchronous native pump level", async ({
    page,
}) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const { diagnostics } = await reachReadyAdversarialRuntime(
        page,
        "native-depth-limit",
    );
    const terminal = await triggerExpectedNativeBoundaryTerminal(
        page,
        "trigger-native-depth-limit",
        "qt-native-depth-limit-attempt",
        "runtime-native-event-pump-depth-limit",
    );
    expect(terminal.stopped).toBe(true);
    expect(terminal.attempt.payload).toEqual({
        activeDepth: 4,
        requestedDepth: 5,
    });
    expect(terminal.attempt.sequence).toBeLessThan(
        terminal.event.sequence,
    );
    expect(terminal.event.payload.detail).toEqual({
        activeDepth: 4,
        limit: 4,
    });
    await assertNativeBoundaryTerminalQuiescent(page, terminal);
    expect(diagnostics).toEqual({
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    });
});

test("@core traps a suspending import from raw native ingress", async ({
    page,
}) => {
    test.setTimeout(CORE_TEST_TIMEOUT_MS);
    const { diagnostics } = await reachReadyAdversarialRuntime(
        page,
        "native-suspension-trap",
    );
    const terminal = await triggerExpectedNativeBoundaryTerminal(
        page,
        "trigger-native-suspension-trap",
        "qt-native-suspension-attempt",
        "runtime-native-event-pump-failed",
        "qt-native-suspension-returned",
    );
    expect(terminal.stopped).toBe(true);
    expect(terminal.attempt.payload).toEqual({ depth: 1 });
    expect(terminal.attempt.sequence).toBeLessThan(
        terminal.event.sequence,
    );
    expect(terminal.event.payload.detail.message).not.toHaveLength(0);
    await assertNativeBoundaryTerminalQuiescent(page, terminal);
    expect(diagnostics).toEqual({
        blocking: [],
        console: [],
        crashes: [],
        csp: [],
        http: [],
        pageErrors: [],
        requestFailures: [],
    });
});
