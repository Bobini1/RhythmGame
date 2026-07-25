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
    "permissions-policy": "fullscreen=(self), gamepad=(self), hid=(self)",
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
const routeParts = location.pathname.split("/");
const negativeMode = routeParts[1] === "negative" ? routeParts[2] : null;

let resolveReady;
let rejectReady;
const ready = new Promise((resolve, reject) => {
    resolveReady = resolve;
    rejectReady = reject;
});
const report = {
    schemaVersion: 1,
    instance: null,
    events: [],
    snapshot: null,
    ready,
    command(name, payload) {
        switch (name) {
        case "append-event":
            report.events.push(payload);
            return report.events.length - 1;
        case "publish-snapshot":
            report.snapshot = payload;
            return true;
        case "resolve-ready":
            resolveReady(payload);
            return true;
        case "reject-ready":
            rejectReady(payload);
            return true;
        default:
            throw new Error(`gate1b-command-not-allowed:${String(name)}`);
        }
    },
};
globalThis.__rhythmGameGate1b = report;

let terminalFailure = null;
let activeWorkerCspReject = null;
function fail(code, detail = {}) {
    if (terminalFailure === null) {
        terminalFailure = Object.freeze({ code, detail });
        report.events.push(Object.freeze({
            payload: terminalFailure,
            type: "terminal-failure",
        }));
        rejectReady(terminalFailure);
    }
    const error = new Error(code);
    error.gate1bCode = code;
    throw error;
}

globalThis.addEventListener("unhandledrejection", (event) => {
    try {
        fail("unhandled-rejection", {
            reason: String(event.reason),
        });
    } catch {
        // The terminal record already owns this failure.
    }
});
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
        const shouldReport = active;
        active = false;
        if (ownershipMonitor !== null) {
            clearInterval(ownershipMonitor);
            ownershipMonitor = null;
        }
        if (shouldReport) {
            report.events.push(Object.freeze({
                payload: Object.freeze({ phase }),
                type: "qt-style-adapter-ownership-loss",
            }));
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
                report.events.push(Object.freeze({
                    payload: Object.freeze({
                        bytes: bytes.byteLength,
                        hostId: qtShadowHostId,
                        ruleCount: sheet.cssRules.length,
                        sha256: actual.hex,
                    }),
                    type: "qt-style-adopted",
                }));
                return Object.freeze({ verified: true });
            },
            (error) => Object.freeze({
                error: String(error),
                verified: false,
            }),
        );
        const record = Object.freeze({
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
            if (
                record.root.host.id !== qtShadowHostId
                || record.root.host.shadowRoot !== record.root
                || record.root.host.parentNode !== screen
                || record.root.querySelectorAll("style").length !== 0
                || screen.querySelectorAll("style").length !== 0
                || record.child.parentNode !== null
                || record.child.isConnected
                || record.sheet.cssRules.length !== qtStyleRuleCount
                || record.root.adoptedStyleSheets.filter(
                    (candidate) => candidate === record.sheet,
                ).length !== 1
            ) {
                fail("qt-style-shape");
            }
            if (!ownsAppendChild()) {
                reportOwnershipLoss("initial-adoption-complete");
            }
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
    await new Promise((resolve, reject) => {
        const socket = new WebSocket(url);
        const timer = setTimeout(() => {
            socket.close();
            reject(new Error("wss-timeout"));
        }, 5000);
        socket.addEventListener("open", () => {
            socket.send("close");
        }, { once: true });
        socket.addEventListener("close", (event) => {
            clearTimeout(timer);
            if (event.code === 1000 && event.reason === "probe-complete") {
                resolve();
            } else {
                reject(new Error(`wss-close-${event.code}`));
            }
        }, { once: true });
        socket.addEventListener("error", () => {
            clearTimeout(timer);
            reject(new Error("wss-error"));
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
            onAbort: (text) => qtStyleAdapter.restoreAndFail(
                "runtime-abort",
                { text: String(text) },
            ),
            onExit: (code) => qtStyleAdapter.restoreAndFail(
                "runtime-exit",
                { code },
            ),
            qt: {
                containerElements: [screen],
                entryFunction: window.RhythmGameWasmProbe_entry,
                onExit: (exit) => qtStyleAdapter.restoreAndFail(
                    "runtime-exit",
                    exit,
                ),
            },
        });
    } catch (error) {
        qtStyleAdapter.restore("qtLoad-rejection");
        throw error;
    }
    try {
        await qtStyleAdapter.requireInitialAdoption();
    } catch (error) {
        qtStyleAdapter.restore("qt-style-initial-rejection");
        throw error;
    }
    report.instance = instance;
}

bootstrap().catch((error) => {
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
