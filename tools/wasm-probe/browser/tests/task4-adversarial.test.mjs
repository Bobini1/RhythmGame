import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import test from "node:test";
import vm from "node:vm";

import {
    findAllowedConsoleRecordIndex,
    normalizeConsoleTextForAllowlist,
} from "../lib/console-policy.mjs";

const probeDirectory = fileURLToPath(new URL("../../", import.meta.url));

async function readProbeSource(relativePath) {
    return readFile(
        new URL(relativePath, new URL("../../", import.meta.url)),
        "utf8",
    );
}

function section(source, startMarker, endMarker) {
    const start = source.indexOf(startMarker);
    const end = source.indexOf(endMarker, start + startMarker.length);
    assert.notEqual(start, -1, `missing start marker: ${startMarker}`);
    assert.notEqual(end, -1, `missing end marker: ${endMarker}`);
    return source.slice(start, end);
}

function emJsBody(source, symbol, nextMarker) {
    const block = section(source, symbol, nextMarker);
    const bodyStart = block.indexOf("\n    {\n");
    const bodyEnd = block.lastIndexOf("\n    });");
    assert.notEqual(bodyStart, -1, `missing EM_JS body: ${symbol}`);
    assert.notEqual(bodyEnd, -1, `missing EM_JS terminator: ${symbol}`);
    return block.slice(bodyStart + "\n    {".length, bodyEnd);
}

test("console allowlist removes exactly one terminal LF or CRLF", () => {
    const allowed = [{
        maxOccurrences: 1,
        text: "No media devices found",
        type: "warning",
    }];
    assert.equal(
        findAllowedConsoleRecordIndex(
            allowed,
            { text: "No media devices found\n", type: "warning" },
        ),
        0,
    );
    assert.equal(
        findAllowedConsoleRecordIndex(
            allowed,
            { text: "No media devices found\r\n", type: "warning" },
        ),
        0,
    );
    assert.equal(
        normalizeConsoleTextForAllowlist("No media devices found\n\n"),
        "No media devices found\n",
    );
    for (const text of [
        "No media devices found\n\n",
        "No media devices found ",
        "No media devices found\r",
        "no media devices found\n",
    ]) {
        assert.equal(
            findAllowedConsoleRecordIndex(
                allowed,
                { text, type: "warning" },
            ),
            -1,
        );
    }
    assert.equal(
        findAllowedConsoleRecordIndex(
            allowed,
            { text: "No media devices found\n", type: "error" },
        ),
        -1,
    );
});

test("unprintable rejection reasons still latch a terminal failure", async () => {
    const bootstrap = await readProbeSource("browser/web/bootstrap.mjs");
    const handlerSource = section(
        bootstrap,
        "function describeUnhandledRejection(event)",
        'globalThis.addEventListener("securitypolicyviolation"',
    );
    const listeners = new Map();
    const failures = [];
    vm.runInNewContext(handlerSource, {
        fail(code, detail) {
            failures.push({ code, detail });
            throw new Error(code);
        },
        globalThis: {
            addEventListener(type, listener) {
                listeners.set(type, listener);
            },
        },
    });
    const reason = {
        [Symbol.toPrimitive]() {
            throw new Error("adversarial-coercion");
        },
    };

    listeners.get("unhandledrejection")({ reason });

    assert.equal(failures.length, 1);
    assert.equal(failures[0].code, "unhandled-rejection");
    assert.equal(
        failures[0].detail.reason,
        "<unprintable-rejection-reason>",
    );
});

async function createTrackerHarness() {
    const bootstrap = await readProbeSource("browser/web/bootstrap.mjs");
    const trackerSource = section(
        bootstrap,
        "function createMediaBackendTracker()",
        "const mediaBackendTracker = createMediaBackendTracker();",
    );
    let monotonicMilliseconds = 10;

    class FakeHTMLMediaElement {
        static HAVE_NOTHING = 0;
        static NETWORK_EMPTY = 0;

        constructor(tagName) {
            this.currentSrc = "";
            this.currentTime = 0;
            this.deferResourceRelease = false;
            this.duration = Number.NaN;
            this.error = null;
            this.isConnected = true;
            this.networkState = FakeHTMLMediaElement.NETWORK_EMPTY;
            this.paused = true;
            this.readyState = FakeHTMLMediaElement.HAVE_NOTHING;
            this.seeking = false;
            this.src = "";
            this.srcObject = null;
            this.sourceChildren = [];
            this.tagName = tagName.toUpperCase();
            this.attributes = new Map();
            this.listeners = new Map();
        }

        addEventListener(type, listener) {
            const listeners = this.listeners.get(type) ?? new Set();
            listeners.add(listener);
            this.listeners.set(type, listeners);
        }

        dispatch(type) {
            for (const listener of this.listeners.get(type) ?? []) {
                listener();
            }
        }

        getAttribute(name) {
            return this.attributes.get(name) ?? null;
        }

        load() {
            if (!this.attributes.has("src")) {
                this.src = "";
                this.error = null;
                this.networkState = this.deferResourceRelease
                    ? 3
                    : FakeHTMLMediaElement.NETWORK_EMPTY;
                this.paused = true;
                this.readyState = FakeHTMLMediaElement.HAVE_NOTHING;
                this.currentTime = 0;
                this.duration = Number.NaN;
                this.seeking = false;
                this.dispatch("emptied");
            }
        }

        completeResourceRelease() {
            this.networkState = FakeHTMLMediaElement.NETWORK_EMPTY;
        }

        destroyAsVideoOutput() {
            this.removeAttribute("src");
            if (this.srcObject !== null) {
                this.srcObject = null;
            }
            this.load();
            this.remove();
        }

        remove() {
            this.isConnected = false;
        }

        removeAttribute(name) {
            this.attributes.delete(name);
            if (name === "src") {
                this.src = "";
            }
        }

        querySelectorAll(selector) {
            return selector === "source" ? this.sourceChildren : [];
        }

        removeEventListener(type, listener) {
            this.listeners.get(type)?.delete(listener);
        }

        setMediaSource(source) {
            this.attributes.set("src", source);
            this.currentSrc = source;
            this.duration = 2.008;
            this.networkState = 1;
            this.paused = false;
            this.readyState = 4;
            this.src = source;
        }
    }

    const documentElements = [];
    const document = {
        createElement(tagName) {
            if (["audio", "video"].includes(tagName)) {
                const element = new FakeHTMLMediaElement(tagName);
                documentElements.push(element);
                return element;
            }
            return { tagName: tagName.toUpperCase() };
        },
        querySelectorAll(selector) {
            return selector === "audio, video"
                ? documentElements.filter((element) => element.isConnected)
                : [];
        },
    };
    const location = { href: "https://rhythmgame.eu/probe/" };
    const performance = {
        now() {
            return monotonicMilliseconds;
        },
    };
    const createTracker = vm.runInNewContext(
        `${trackerSource}\ncreateMediaBackendTracker`,
        {
            document,
            HTMLMediaElement: FakeHTMLMediaElement,
            location,
            performance,
            URL,
        },
    );
    return {
        advanceTo(value) {
            monotonicMilliseconds = value;
        },
        document,
        expectedSource(runNonce) {
            return new URL(
                `/fixtures/probe.webm?nonce=${runNonce}`,
                location.href,
            ).href;
        },
        tracker: createTracker(),
    };
}

test("Task 4 success teardown never calls the warning-producing Qt stop path", async () => {
    const media = await readProbeSource("src/MediaProbe.cpp");
    const teardown = section(
        media,
        "void MediaProbe::armBackendRemovalAndDestroy()",
        "void MediaProbe::finishBackendRemoval()",
    );
    assert.match(teardown, /armOwnedMediaBackendRemoval/);
    assert.doesNotMatch(teardown, /setSource\s*\(\s*QUrl\s*\{\s*\}\s*\)/);
    assert.doesNotMatch(teardown, /->stop\s*\(/);
});

test("Task 4 backend removal is certified after Qt player destruction", async () => {
    const media = await readProbeSource("src/MediaProbe.cpp");
    const clear = section(
        media,
        "void MediaProbe::armBackendRemovalAndDestroy()",
        "void MediaProbe::finishBackendRemoval()",
    );
    const finish = section(
        media,
        "void MediaProbe::finishBackendRemoval()",
        "void MediaProbe::finishObjectTeardownIfReady()",
    );
    const complete = section(
        media,
        "void MediaProbe::finishObjectTeardownIfReady()",
        "void MediaProbe::append(",
    );
    assert.match(clear, /&QObject::destroyed/);
    assert.match(clear, /player->deleteLater\(\)/);
    assert.match(finish, /!m_playerDestructionRecorded/);
    assert.match(finish, /mediaElementResourceReleased/);
    assert.match(finish, /backendRemovalStabilityWindow/);
    assert.match(finish, /qt-media-backend-removed/);
    assert.match(complete, /!m_backendRemovalRecorded/);
});

test("Task 4 preflight WSS completes the exact bounded server protocol", async () => {
    const bootstrap = await readProbeSource("browser/web/bootstrap.mjs");
    const preflightSource = section(
        bootstrap,
        "async function preflightWebSocket()",
        "async function injectVerifiedScript(",
    );
    let constructedUrl = null;
    let constructedSocket = null;
    let sentCountAfterOpen = null;
    const sentMessages = [];

    class FakeWebSocket {
        constructor(url) {
            constructedUrl = new URL(url);
            constructedSocket = this;
            this.listeners = new Map();
            queueMicrotask(() => {
                this.emit("open", {});
                sentCountAfterOpen = sentMessages.length;
                this.emit("message", {
                    data: JSON.stringify({
                        connectionId: "00000000-0000-4000-8000-000000000001",
                        nonce: 2,
                        type: "server-message",
                        value: "connected",
                    }),
                });
            });
        }

        addEventListener(type, listener) {
            this.listeners.set(type, listener);
        }

        close() {
            throw new Error("preflight timeout unexpectedly closed the socket");
        }

        emit(type, event) {
            this.listeners.get(type)?.(event);
        }

        send(message) {
            sentMessages.push(message);
            const step = sentMessages.length;
            if (step === 1) {
                assert.equal(message, "text-echo:2");
                queueMicrotask(() => this.emit("message", { data: message }));
            } else if (step === 2) {
                assert.deepEqual(Array.from(message), [1, 2, 3]);
                queueMicrotask(() => this.emit("message", {
                    data: message.slice().buffer,
                }));
            } else if (step === 3) {
                assert.equal(message, "heartbeat:2");
                queueMicrotask(() => this.emit("message", {
                    data: JSON.stringify({
                        nonce: 2,
                        type: "heartbeat",
                    }),
                }));
            } else if (step === 4) {
                assert.equal(message, "close");
                queueMicrotask(() => this.emit("close", {
                    code: 1000,
                    reason: "probe-complete",
                }));
            } else {
                throw new Error(`unexpected preflight send ${step}`);
            }
        }
    }

    const preflight = vm.runInNewContext(
        `${preflightSource}\npreflightWebSocket`,
        {
            URL,
            WebSocket: FakeWebSocket,
            clearTimeout() {},
            crypto: {
                getRandomValues(values) {
                    values[0] = 0xFFFFFFFF;
                    return values;
                },
            },
            fail(code) {
                throw new Error(`unexpected preflight failure: ${code}`);
            },
            location: {
                href: "https://127.0.0.1:49152/RhythmGameWasmProbe.html",
            },
            queueMicrotask,
            setTimeout() {
                return 1;
            },
        },
    );

    await preflight();
    assert.equal(constructedUrl.protocol, "wss:");
    assert.equal(constructedUrl.host, "127.0.0.1:49152");
    assert.equal(constructedUrl.pathname, "/probe/ws");
    assert.equal(constructedUrl.searchParams.getAll("nonce").length, 1);
    assert.equal(constructedUrl.searchParams.get("nonce"), "2");
    assert.equal(constructedSocket.binaryType, "arraybuffer");
    assert.equal(sentCountAfterOpen, 0);
    assert.equal(sentMessages[0], "text-echo:2");
    assert.deepEqual(Array.from(sentMessages[1]), [1, 2, 3]);
    assert.equal(sentMessages[2], "heartbeat:2");
    assert.equal(sentMessages[3], "close");
    constructedSocket.emit("message", { data: "late" });
    assert.equal(sentMessages.length, 4);
});

test("Task 4 tracker binds and observes exactly one nonce-owned video", async () => {
    const bootstrap = await readProbeSource("browser/web/bootstrap.mjs");
    const tracker = section(
        bootstrap,
        "function createMediaBackendTracker()",
        "const mediaBackendTracker = createMediaBackendTracker();",
    );
    for (const marker of [
        "matchingElements.length !== 1",
        "record.element.src === expectedSource.href",
        "record.element.currentSrc === expectedSource.href",
        "state.ownedRecord = matchingElements[0]",
        "record.element.isConnected === false",
        "cleanupArmed",
        "sourceAttributeCleared",
        "currentSourceCleared",
        "currentSourceOwnedOrEmpty",
        "currentTimeReset",
        "documentOwnedElementCount",
        "domRemoved",
        "durationCleared",
        "emptiedObserved",
        "hadResourceBeforeDestruction",
        "mediaElementResourceReleased",
        "mediaErrorCleared",
        "matchingElementCount: 1",
        "networkStateEmpty",
        "observerMutatedElement: false",
        "readyStateEmpty",
        "seekingStopped",
        "sourceElementCount",
        "sourcePropertyCleared",
        "release",
    ]) {
        assert.ok(tracker.includes(marker), `missing tracker marker: ${marker}`);
    }
    assert.doesNotMatch(tracker, /removeAttribute\s*\(/);
    assert.doesNotMatch(tracker, /\.load\s*\(/);
    assert.doesNotMatch(tracker, /\.remove\s*\(/);
    assert.doesNotMatch(tracker, /\.srcObject\s*=(?!=)/);
    assert.doesNotMatch(tracker, /elements\.length\s*>\s*0\s*&&\s*elements\.every/);
});

test("Task 4 tracker ignores unrelated empty media in executable behavior", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(77), true);
    const unrelated = harness.document.createElement("video");
    assert.equal(harness.tracker.armSeek(77, 1000), null);
    assert.equal(harness.tracker.armRemoval(77), null);
    assert.equal(harness.tracker.finish(77), null);
    assert.equal(harness.tracker.release(77), null);
    assert.equal(unrelated.isConnected, true);
});

test("Task 4 tracker rejects ambiguous exact-source ownership", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(78), true);
    const expectedSource = harness.expectedSource(78);
    harness.document.createElement("video").setMediaSource(expectedSource);
    harness.document.createElement("video").setMediaSource(expectedSource);
    assert.equal(harness.tracker.armSeek(78, 1000), null);
    assert.equal(harness.tracker.armRemoval(78), null);
    assert.equal(harness.tracker.finish(78), null);
    assert.equal(harness.tracker.release(78), null);
});

test("Task 4 tracker proves seek and removal on the same exact element", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(79), true);
    const unrelated = harness.document.createElement("video");
    const owned = harness.document.createElement("video");
    owned.setMediaSource(harness.expectedSource(79));
    owned.currentTime = 0.4;

    const arm = harness.tracker.armSeek(79, 1000);
    assert.equal(arm.preSeekPositionMilliseconds, 400);
    harness.advanceTo(20);
    owned.currentTime = 1;
    owned.dispatch("seeking");
    harness.advanceTo(30);
    owned.dispatch("seeked");
    const seekProof = harness.tracker.finishSeek(79);
    assert.equal(seekProof.elementId, arm.elementId);
    assert.equal(seekProof.requestMonotonicMilliseconds, 10);
    assert.equal(seekProof.seekingMonotonicMilliseconds, 20);
    assert.equal(seekProof.seekedMonotonicMilliseconds, 30);
    assert.equal(seekProof.seekedPositionMilliseconds, 1000);

    const removalArm = harness.tracker.armRemoval(79);
    assert.equal(removalArm.elementId, arm.elementId);
    assert.equal(removalArm.matchingElementCount, 1);
    assert.equal(removalArm.cleanupArmed, true);
    assert.equal(removalArm.hadResourceBeforeDestruction, true);
    assert.equal(removalArm.observerMutatedElement, false);
    assert.equal(owned.isConnected, true);
    assert.equal(owned.src, harness.expectedSource(79));
    assert.equal(harness.tracker.finish(79).mediaElementResourceReleased, false);

    owned.destroyAsVideoOutput();
    const removed = harness.tracker.finish(79);
    assert.equal(removed.elementId, arm.elementId);
    assert.equal(removed.matchingElementCount, 1);
    assert.equal(removed.currentSourceCleared, false);
    assert.equal(removed.currentSourceOwnedOrEmpty, true);
    assert.equal(removed.currentTimeReset, true);
    assert.equal(removed.documentOwnedElementCount, 0);
    assert.equal(removed.durationCleared, true);
    assert.equal(removed.emptiedObserved, true);
    assert.equal(removed.hadResourceBeforeDestruction, true);
    assert.equal(removed.mediaElementResourceReleased, true);
    assert.equal(removed.mediaErrorCleared, true);
    assert.equal(removed.networkStateEmpty, true);
    assert.equal(removed.observerMutatedElement, false);
    assert.equal(removed.paused, true);
    assert.equal(removed.readyStateEmpty, true);
    assert.equal(removed.seekingStopped, true);
    assert.equal(removed.sourceAttributeCleared, true);
    assert.equal(removed.sourceElementCount, 0);
    assert.equal(removed.sourceObjectCleared, true);
    assert.equal(removed.sourcePropertyCleared, true);
    assert.equal(removed.domRemoved, true);
    assert.equal(unrelated.isConnected, true);
    assert.deepEqual(
        harness.tracker.finish(79),
        removed,
    );
    assert.deepEqual(harness.tracker.release(79), removed);
    assert.equal(harness.tracker.finish(79), null);
});

test("Task 4 tracker accepts retained currentSrc after resource release", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(81), true);
    const owned = harness.document.createElement("video");
    owned.setMediaSource(harness.expectedSource(81));

    const removalArm = harness.tracker.armRemoval(81);
    assert.equal(removalArm.observerMutatedElement, false);
    assert.equal(owned.isConnected, true);
    owned.destroyAsVideoOutput();
    const removed = harness.tracker.finish(81);
    assert.equal(removed.currentSourceCleared, false);
    assert.equal(removed.currentSourceOwnedOrEmpty, true);
    assert.equal(removed.emptiedObserved, true);
    assert.equal(removed.mediaElementResourceReleased, true);
    assert.equal(removed.networkStateEmpty, true);
    assert.equal(removed.readyStateEmpty, true);
    assert.equal(removed.sourceAttributeCleared, true);
    assert.equal(removed.sourceObjectCleared, true);
    assert.equal(removed.domRemoved, true);
    assert.deepEqual(
        harness.tracker.finish(81),
        removed,
    );
    assert.deepEqual(
        harness.tracker.finish(81),
        removed,
    );
    assert.equal(harness.tracker.begin(82), false);
    assert.deepEqual(
        harness.tracker.release(81),
        removed,
    );
    assert.equal(harness.tracker.finish(81), null);
    assert.equal(harness.tracker.begin(82), true);
    assert.equal(harness.tracker.finish(82, true), null);
});

test("Task 4 tracker exposes source restoration before finalization", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(87), true);
    const owned = harness.document.createElement("video");
    const expectedSource = harness.expectedSource(87);
    owned.setMediaSource(expectedSource);

    assert.notEqual(harness.tracker.armRemoval(87), null);
    owned.destroyAsVideoOutput();
    assert.equal(
        harness.tracker.finish(87).mediaElementResourceReleased,
        true,
    );
    owned.setMediaSource(expectedSource);
    const restored = harness.tracker.finish(87);
    assert.equal(restored.mediaElementResourceReleased, false);
    assert.equal(restored.networkStateEmpty, false);
    assert.equal(restored.readyStateEmpty, false);
    assert.equal(restored.sourceAttributeCleared, false);
    assert.equal(restored.sourcePropertyCleared, false);
    assert.equal(harness.tracker.begin(88), false);
    assert.equal(harness.tracker.release(87), null);
    assert.notEqual(harness.tracker.finish(87, true), null);
    assert.equal(harness.tracker.begin(88), true);
    assert.equal(harness.tracker.finish(88, true), null);
});

test("Task 4 tracker retains ownership until NETWORK_EMPTY", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(83), true);
    const owned = harness.document.createElement("video");
    owned.setMediaSource(harness.expectedSource(83));
    owned.deferResourceRelease = true;

    assert.notEqual(harness.tracker.armRemoval(83), null);
    owned.destroyAsVideoOutput();
    assert.equal(
        harness.tracker.finish(83).mediaElementResourceReleased,
        false,
    );
    assert.equal(harness.tracker.finish(83).emptiedObserved, true);
    assert.equal(harness.tracker.finish(83).networkStateEmpty, false);
    assert.equal(harness.tracker.release(83), null);
    assert.equal(harness.tracker.begin(84), false);

    owned.completeResourceRelease();
    const released = harness.tracker.finish(83);
    assert.equal(released.currentSourceCleared, false);
    assert.equal(released.currentSourceOwnedOrEmpty, true);
    assert.equal(released.mediaElementResourceReleased, true);
    assert.equal(released.networkStateEmpty, true);
    assert.deepEqual(harness.tracker.release(83), released);
    assert.equal(harness.tracker.finish(83), null);
    assert.equal(harness.tracker.begin(84), true);
    assert.equal(harness.tracker.finish(84, true), null);
});

test("Task 4 tracker abandon releases delayed resource ownership", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(85), true);
    const owned = harness.document.createElement("video");
    owned.setMediaSource(harness.expectedSource(85));
    owned.deferResourceRelease = true;

    assert.notEqual(harness.tracker.armRemoval(85), null);
    owned.destroyAsVideoOutput();
    assert.equal(
        harness.tracker.finish(85).mediaElementResourceReleased,
        false,
    );
    assert.equal(
        harness.tracker.finish(85, true).mediaElementResourceReleased,
        false,
    );
    assert.equal(harness.tracker.finish(85), null);
    assert.equal(harness.tracker.begin(86), true);
    assert.equal(harness.tracker.finish(86, true), null);
});

test("Task 4 natural playback crossing cannot produce seek proof", async () => {
    const harness = await createTrackerHarness();
    assert.equal(harness.tracker.begin(80), true);
    const owned = harness.document.createElement("video");
    owned.setMediaSource(harness.expectedSource(80));
    owned.currentTime = 0.4;

    const arm = harness.tracker.armSeek(80, 1000);
    assert.notEqual(arm, null);
    harness.advanceTo(500);
    owned.currentTime = 1;
    owned.dispatch("timeupdate");
    assert.equal(harness.tracker.finishSeek(80), null);
    harness.advanceTo(700);
    owned.currentTime = 1.1;
    owned.dispatch("timeupdate");
    assert.equal(harness.tracker.finishSeek(80), null);

    assert.notEqual(harness.tracker.armRemoval(80), null);
    owned.destroyAsVideoOutput();
    assert.notEqual(harness.tracker.finish(80), null);
    assert.notEqual(harness.tracker.release(80), null);
});

test("Task 4 seek proof is command-correlated and cannot be natural crossing", async () => {
    const [bootstrap, mediaHeader, mediaSource] = await Promise.all([
        readProbeSource("browser/web/bootstrap.mjs"),
        readProbeSource("src/MediaProbe.h"),
        readProbeSource("src/MediaProbe.cpp"),
    ]);
    const combined = `${bootstrap}\n${mediaHeader}\n${mediaSource}`;
    for (const marker of [
        'addEventListener("seeking"',
        'addEventListener("seeked"',
        "armOwnedMediaSeekTracking",
        "finishOwnedMediaSeekTracking",
        "seekResponseTimeout",
        "minimumSeekJumpMilliseconds",
        "maximumSeekTargetErrorMilliseconds",
        "requestMonotonicMilliseconds",
        "preSeekPositionMilliseconds",
        "seekedMonotonicMilliseconds",
        "seekedPositionMilliseconds",
        "qt-media-seek-timeout",
    ]) {
        assert.ok(combined.includes(marker), `missing seek marker: ${marker}`);
    }
    const seekProofStart = mediaSource.indexOf(
        "void MediaProbe::pollSeekProof()",
    );
    const seekProofEnd = mediaSource.indexOf(
        "void MediaProbe::handleNaturalEnd()",
        seekProofStart,
    );
    assert.ok(seekProofStart >= 0 && seekProofEnd > seekProofStart);
    assert.doesNotMatch(
        mediaSource.slice(seekProofStart, seekProofEnd),
        /position\s*>=\s*requiredSeekPositionMilliseconds/,
    );
    assert.match(
        mediaSource,
        /m_postSeekFramePositionSamples\.size\(\)\s*<\s*2/,
    );
    assert.match(
        mediaSource,
        /m_resumePositionMilliseconds\s*\+\s*requiredPostSeekAdvanceMilliseconds/,
    );
    assert.match(
        mediaHeader,
        /maximumPostSeekFramePositionSamples\s*=\s*32/,
    );
    assert.match(
        mediaSource,
        /m_postSeekFramePositionSamples\.replace\(\s*maximumPostSeekFramePositionSamples\s*-\s*1,\s*position\)/,
    );
    assert.match(
        mediaSource,
        /m_backendRemovalPoll->start\(\)/,
    );
    assert.match(
        mediaHeader,
        /backendRemovalResponseTimeout\s*=\s*std::chrono::seconds\{1\}/,
    );
    assert.match(
        mediaSource,
        /status\s*==\s*QMediaPlayer::EndOfMedia\)\s*\{[\s\S]{0,180}QTimer::singleShot\([\s\S]{0,180}&MediaProbe::handleNaturalEnd/,
    );
});

test("Task 4 buffers every native WSS event until connection identity is known", async () => {
    const [networkHeader, networkSource, spec] = await Promise.all([
        readProbeSource("src/NetworkProbe.h"),
        readProbeSource("src/NetworkProbe.cpp"),
        readProbeSource("browser/tests/gate1b.spec.mjs"),
    ]);
    const combined = `${networkHeader}\n${networkSource}`;
    for (const marker of [
        "m_pendingWebSocketEvents",
        "appendWebSocketEvent",
        "flushPendingWebSocketEvents",
        'payload.insert(QStringLiteral("connectionId"), m_connectionId)',
        "nativeWebSocketEvents",
        "payload.connectionId === wss.connectionId",
    ]) {
        assert.ok(
            combined.includes(marker) || spec.includes(marker),
            `missing WSS identity marker: ${marker}`,
        );
    }
    const connected = section(
        networkSource,
        "void NetworkProbe::webSocketConnected()",
        "void NetworkProbe::webSocketTextMessage",
    );
    assert.doesNotMatch(connected, /\bappend\s*\(\s*u"qt-wss-/);
});

test("Task 4 QNAM applies the hard byte cap before allocation", async () => {
    const network = await readProbeSource("src/NetworkProbe.cpp");
    assert.match(
        network,
        /setReadBufferSize\s*\(\s*maximumQnamResponseBytes\s*\+\s*1\s*\)/,
    );
    assert.match(network, /read\s*\(\s*maximumReadableBytes\s*\)/);
    assert.ok(network.includes("remainingBytes + 1"));
    assert.doesNotMatch(network, /readAll\s*\(/);
});

test("terminal ready rejection settles the suspended JSPI import", async () => {
    const bridge = await readProbeSource("src/BrowserRuntimeBridge.cpp");
    const body = emJsBody(
        bridge,
        "rgGate1bAwaitOwnedNonce",
        "rgDispatchNativeDepthProbeEvent",
    );
    let rejectReady;
    const ready = new Promise((resolve, reject) => {
        void resolve;
        rejectReady = reject;
    });
    class FakeElement {
        addEventListener() {}
        removeEventListener() {}
    }
    const screen = new FakeElement();
    const heapBytes = new Uint8Array(512);
    const heapWords = new Int32Array(heapBytes.buffer);
    const canaryPointer = 128;
    const canaryLength = 32;
    const canarySeed = 0x5A;
    for (let index = 0; index < canaryLength; ++index) {
        heapBytes[canaryPointer + index] =
            (canarySeed + (index * 37)) & 0xFF;
    }
    const report = {
        events: [],
        ready,
        rejectReady() {},
        snapshot: { failures: [] },
    };
    const invoke = vm.runInNewContext(
        `(async function (
            requestedNonce,
            firstNativeDepthHandler,
            primaryStackCanary,
            primaryStackCanaryLength,
            primaryStackCanarySeed,
            fullPumpDeferred,
            primaryStackCanaryObservedIntact
        ) {${body}\n})`,
        {
            clearTimeout,
            document: {
                getElementById() {
                    return screen;
                },
                querySelector() {
                    return screen;
                },
            },
            HEAP32: heapWords,
            HEAPU8: heapBytes,
            HTMLElement: FakeElement,
            __rhythmGameGate1b: report,
            Module: {
                async qtSendPendingApplicationEvents() {
                    return false;
                },
                qtSuspendResumeControl: {
                    eventHandlers: {},
                },
            },
            MouseEvent: class {},
            setTimeout,
        },
    );
    const resultPromise = invoke(
        123,
        1,
        canaryPointer,
        canaryLength,
        canarySeed,
        4,
        8,
    );
    setTimeout(() => rejectReady(new Error("terminal")), 0);
    const result = await Promise.race([
        resultPromise,
        new Promise((resolve) => {
            setTimeout(() => resolve("still-pending"), 100);
        }),
    ]);
    assert.equal(result, 0xFFFFFFFF);
});

test("exclusive handler failure still invokes bounded completion", async () => {
    const bridge = await readProbeSource("src/BrowserRuntimeBridge.cpp");
    const body = emJsBody(
        bridge,
        "rgScheduleExclusiveSuspendGuardProbe",
        "rgArmExclusiveSuspendGuardNormalDrain",
    );
    const timers = [];
    let nextTimerId = 0;
    let completionCount = 0;
    const listeners = new Set();
    class FakeElement {
        addEventListener(type, listener) {
            assert.equal(type, "click");
            listeners.add(listener);
        }

        dispatchEvent(event) {
            for (const listener of [...listeners]) {
                listener(event);
            }
        }

        removeEventListener(type, listener) {
            assert.equal(type, "click");
            listeners.delete(listener);
        }
    }
    class FakeMouseEvent {
        constructor(type) {
            this.type = type;
        }
    }
    const screen = new FakeElement();
    const control = {
        eventHandlers: {
            1() {},
            2() {},
            3() {
                throw new Error("exclusive-handler-failed");
            },
            4() {
                ++completionCount;
            },
        },
        exclusiveEventHandler: 0,
        pendingEvents: [],
        resume: null,
    };
    const Module = {
        qtSendPendingEvents: async () => false,
        qtSuspendResumeControl: control,
    };
    const invoke = vm.runInNewContext(
        `(function (
            firstHandler,
            secondHandler,
            exclusiveHandler,
            completionHandler
        ) {${body}\n})`,
        {
            clearTimeout() {},
            document: {
                getElementById() {
                    return screen;
                },
            },
            HTMLElement: FakeElement,
            Module,
            MouseEvent: FakeMouseEvent,
            setTimeout(callback, delay) {
                const timer = {
                    callback,
                    delay,
                    id: ++nextTimerId,
                };
                timers.push(timer);
                return timer.id;
            },
        },
    );
    assert.equal(invoke(1, 2, 3, 4), 1);
    for (let iteration = 0; timers.length > 0 && iteration < 32; ++iteration) {
        timers.sort(
            (left, right) => left.delay - right.delay || left.id - right.id,
        );
        timers.shift().callback();
        await Promise.resolve();
    }
    assert.equal(completionCount, 1);
    assert.match(
        Module.__rhythmGameExclusiveSuspendGuardProbe.error,
        /exclusive-handler-failed/,
    );
    assert.ok(
        bridge.includes("exclusiveFinalizationWatchdogMilliseconds"),
    );
});

test("failed exclusive completion handlers retain a native fallback", async () => {
    const [bridge, jspi] = await Promise.all([
        readProbeSource("src/BrowserRuntimeBridge.cpp"),
        readProbeSource("src/JspiNestedLoopProbe.cpp"),
    ]);
    const body = emJsBody(
        bridge,
        "rgScheduleExclusiveSuspendGuardProbe",
        "rgArmExclusiveSuspendGuardNormalDrain",
    );
    const runCase = async (completionHandler) => {
        const timers = [];
        let nextTimerId = 0;
        const listeners = new Set();
        class FakeElement {
            addEventListener(type, listener) {
                assert.equal(type, "click");
                listeners.add(listener);
            }

            dispatchEvent(event) {
                for (const listener of [...listeners]) {
                    listener(event);
                }
            }

            removeEventListener(type, listener) {
                assert.equal(type, "click");
                listeners.delete(listener);
            }
        }
        const screen = new FakeElement();
        const eventHandlers = {
            1() {},
            2() {},
            3() {},
        };
        if (completionHandler !== null) {
            eventHandlers[4] = completionHandler;
        }
        const Module = {
            qtSendPendingEvents: async () => false,
            qtSuspendResumeControl: {
                eventHandlers,
                exclusiveEventHandler: 0,
                pendingEvents: [],
                resume: null,
            },
        };
        const invoke = vm.runInNewContext(
            `(function (
                firstHandler,
                secondHandler,
                exclusiveHandler,
                completionHandler
            ) {${body}\n})`,
            {
                clearTimeout() {},
                document: {
                    getElementById() {
                        return screen;
                    },
                },
                HTMLElement: FakeElement,
                Module,
                MouseEvent: class {},
                setTimeout(callback, delay) {
                    const timer = {
                        callback,
                        delay,
                        id: ++nextTimerId,
                    };
                    timers.push(timer);
                    return timer.id;
                },
            },
        );
        assert.equal(invoke(1, 2, 3, 4), 1);
        for (
            let iteration = 0;
            timers.length > 0 && iteration < 32;
            ++iteration
        ) {
            timers.sort(
                (left, right) => (
                    left.delay - right.delay || left.id - right.id
                ),
            );
            timers.shift().callback();
            await Promise.resolve();
        }
        return Module.__rhythmGameExclusiveSuspendGuardProbe;
    };

    const missing = await runCase(null);
    assert.match(missing.error, /exclusive-probe-handler-completion/);
    assert.equal(missing.completionScheduled, true);

    let throwingAttempts = 0;
    const throwing = await runCase(() => {
        ++throwingAttempts;
        throw new Error("completion-handler-threw");
    });
    assert.match(throwing.error, /completion-handler-threw/);
    assert.equal(throwing.completionScheduled, true);
    assert.equal(throwingAttempts, 1);

    const nativeWatchdogStart =
        jspi.indexOf("m_exclusiveFinalizationWatchdog->start(");
    const browserDispatch =
        jspi.indexOf("scheduleExclusiveSuspendGuardProbe(");
    assert.notEqual(nativeWatchdogStart, -1);
    assert.ok(nativeWatchdogStart < browserDispatch);
    assert.ok(jspi.includes("exclusiveNativeFinalizationDeadline"));
    assert.ok(jspi.includes("m_exclusiveFinalizationWatchdog->stop()"));
});

test("Embind bounds JS handles before UTF-8 conversion", async () => {
    const [bridge, bootstrap] = await Promise.all([
        readProbeSource("src/BrowserRuntimeBridge.cpp"),
        readProbeSource("browser/web/bootstrap.mjs"),
    ]);
    const boundary = section(
        bridge,
        "std::string rhythmGameGate1bCommand(",
        "EM_JS(unsigned, rgGate1bBrowserCapabilityBits",
    );
    assert.match(
        boundary,
        /emscripten::val commandValue,\s*emscripten::val payloadValue/,
    );
    assert.doesNotMatch(
        boundary,
        /const std::string &(?:command|payload)Utf8/,
    );
    const typeCheck = boundary.indexOf("commandValue.isString()");
    const commandLength = boundary.indexOf('commandValue["length"]');
    const payloadLength = boundary.indexOf('payloadValue["length"]');
    const firstStringConversion = boundary.indexOf(".as<std::string>()");
    assert.notEqual(typeCheck, -1);
    assert.ok(typeCheck < commandLength);
    assert.ok(commandLength < firstStringConversion);
    assert.ok(payloadLength < firstStringConversion);
    assert.ok(
        boundary.indexOf("maximumRuntimeCommandNameBytes")
            < firstStringConversion,
    );
    assert.ok(
        boundary.indexOf("maximumRuntimeCommandPayloadBytes")
            < firstStringConversion,
    );
    const typedCatch =
        boundary.indexOf("catch (const std::exception &error)");
    const fallbackCatch = boundary.indexOf("catch (...)");
    assert.ok(firstStringConversion < typedCatch);
    assert.ok(typedCatch < fallbackCatch);
    assert.match(boundary, /return encodeRuntimeCommandSuccess\(reply\)/);
    assert.match(
        boundary,
        /normalizedRuntimeCommandErrorCode\(error\.what\(\)\)/,
    );
    assert.match(
        boundary,
        /encodeRuntimeCommandFailure\(\s*QStringLiteral\("runtime-command-native-failure"\)\)/,
    );
    assert.match(
        bridge,
        /QJsonObject\{\s*\{QStringLiteral\("error"\), errorCode\},\s*\{QStringLiteral\("ok"\), false\}/,
    );
    assert.match(
        bridge,
        /QJsonObject\{\s*\{QStringLiteral\("ok"\), true\},\s*\{QStringLiteral\("reply"\), reply\}/,
    );
    assert.match(bootstrap, /function parseRuntimeCommandResult\(/);
    assert.match(bootstrap, /\["error", "ok"\]/);
    assert.match(bootstrap, /\["ok", "reply"\]/);
    assert.match(bootstrap, /runtime-command-boundary-threw/);
    assert.doesNotMatch(
        bridge + bootstrap,
        /getExceptionMessage|EXPORT_EXCEPTION_HANDLING_HELPERS/,
    );
    assert.match(bootstrap, /"x"\.repeat\(65\)/);
    assert.match(bootstrap, /" "\.repeat\(4097\)/);
    assert.match(bootstrap, /runtime-command-boundary-audit-threw/);
    assert.match(bootstrap, /runtime-command-entry-scrub-failed/);
    assert.match(bridge, /browserPathAuthorizesRuntimeCommand/);
    assert.match(bridge, /runtime-command-native-route/);
});

test("Task 4 adversarial source test resolves the intended probe tree", () => {
    assert.match(
        probeDirectory.replaceAll("\\", "/"),
        /tools\/wasm-probe\/$/,
    );
});
