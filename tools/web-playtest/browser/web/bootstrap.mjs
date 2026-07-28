const requiredRoles = Object.freeze([
    "audioWorklet",
    "bootstrap",
    "css",
    "html",
    "mainJs",
    "media",
    "preflightWorker",
    "qtloader",
    "wasm",
    "wasmWorker",
]);
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
    html: ["RhythmGameWasmProbe", "html"],
    mainJs: ["RhythmGameWasmProbe", "js"],
    media: ["probe", "webm"],
    preflightWorker: ["preflight-worker", "mjs"],
    qtloader: ["qtloader", "js"],
    wasm: ["RhythmGameWasmProbe", "wasm"],
    wasmWorker: ["RhythmGameWasmProbe.ww", "js"],
});
const digestPattern = /^[0-9a-f]{64}$/;
const sriPattern = /^sha256-[A-Za-z0-9+/]{43}=$/;
const qtShadowHostId = "qt-shadow-container";
const qtStyleByteLength = 5238;
const qtStyleSha256 =
    "6b7168686da79590ea116889998716dfa624e1467411daa2bffee066a867d53e";
const qtStyleRuleCount = 37;
const qtStyleKeySelectors = Object.freeze([".qt-screen", ".qt-window"]);
const maximumTraceEntries = 8192;
const phaseNames = Object.freeze([
    "InstallingChart",
    "Decoding",
    "Ready",
    "Countdown",
    "Playing",
    "Finished",
    "Aborted",
    "Error",
]);

function fail(code, detail = {}) {
    const error = new Error(`web-playtest:${code}`);
    error.cause = detail;
    throw error;
}

function cloneAndFreeze(value) {
    if (Array.isArray(value)) {
        return Object.freeze(value.map(cloneAndFreeze));
    }
    if (value !== null && typeof value === "object") {
        return Object.freeze(Object.fromEntries(
            Object.entries(value).map(
                ([key, child]) => [key, cloneAndFreeze(child)],
            ),
        ));
    }
    return value;
}

function installTestBridge() {
    let latest = cloneAndFreeze({
        audioLifecycle: 0,
        audioReadyForTrustedResume: false,
        audioWorkletError: 0,
        combo: 0,
        decodedAssets: 0,
        droppedInputs: 0,
        heapBytes: 0,
        latestJudgement: "",
        phase: 0,
        phaseText: phaseNames[0],
        pressedLaneMask: 0,
        score: 0,
        sessionGeneration: 0,
        terminalError: "",
        totalAssets: 0,
    });
    let latestTrace = new Uint8Array();
    let latestTraceSessionGeneration = 0;
    let resolveReady;
    let rejectReady;
    let readyResolved = false;
    const ready = new Promise((resolve, reject) => {
        resolveReady = resolve;
        rejectReady = reject;
    });

    const publishNativeState = (state) => {
        if (
            state === null
            || typeof state !== "object"
            || !Number.isInteger(state.phase)
            || state.phase < 0
            || state.phase >= phaseNames.length
        ) {
            fail("test-bridge-state");
        }
        latest = cloneAndFreeze({
            ...state,
        });
        if (
            !readyResolved
            && state.phase === 2
            && state.decodedAssets === 630
            && state.totalAssets === 630
        ) {
            readyResolved = true;
            resolveReady();
        }
    };
    const publishTrace = (bytes, sessionGeneration) => {
        if (
            !(bytes instanceof Uint8Array)
            || bytes.byteLength > maximumTraceEntries * 4096
            || !Number.isSafeInteger(sessionGeneration)
            || sessionGeneration < latestTraceSessionGeneration
        ) {
            fail("test-bridge-trace");
        }
        latestTrace = new Uint8Array(bytes);
        latestTraceSessionGeneration = sessionGeneration;
    };

    Object.defineProperty(
        globalThis,
        "__rhythmGameWebPlaytestTestBridge",
        {
            configurable: false,
            enumerable: false,
            value: Object.freeze({
                publishNativeState,
                publishTrace,
            }),
            writable: false,
        },
    );
    Object.defineProperty(
        globalThis,
        "__rhythmGameWebPlaytest",
        {
            configurable: false,
            enumerable: false,
            value: Object.freeze({
                ready,
                snapshot: () => latest,
                takeTraceBytes: () => new Uint8Array(latestTrace),
            }),
            writable: false,
        },
    );
    return Object.freeze({
        rejectReady,
    });
}

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
    return Object.freeze({
        hex: bytesToHex(result),
        sri: `sha256-${bytesToBase64(result)}`,
    });
}

function expectedArtifactUrl(role, sha256) {
    if (role === "html") {
        return "RhythmGameWasmProbe.html";
    }
    const [stem, extension] = artifactNames[role];
    return `${stem}.${sha256}.${extension}`;
}

function validateManifest(manifest) {
    if (
        manifest === null
        || typeof manifest !== "object"
        || Array.isArray(manifest)
        || manifest.schemaVersion !== 1
        || !digestPattern.test(manifest.buildId)
        || manifest.artifacts === null
        || typeof manifest.artifacts !== "object"
        || Array.isArray(manifest.artifacts)
    ) {
        fail("artifact-manifest-shape");
    }
    const roles = Object.keys(manifest.artifacts).sort();
    if (
        roles.length !== requiredRoles.length
        || roles.some((role, index) => role !== requiredRoles[index])
    ) {
        fail("artifact-manifest-roles");
    }
    const urls = new Set();
    for (const role of requiredRoles) {
        const artifact = manifest.artifacts[role];
        if (
            artifact === null
            || typeof artifact !== "object"
            || Array.isArray(artifact)
            || artifact.buildId !== manifest.buildId
            || !Number.isSafeInteger(artifact.bytes)
            || artifact.bytes < 0
            || artifact.mime !== expectedMimes[role]
            || !digestPattern.test(artifact.sha256)
            || !sriPattern.test(artifact.sri)
            || artifact.url !== expectedArtifactUrl(role, artifact.sha256)
            || artifact.url.includes("/")
            || artifact.url.includes("\\")
        ) {
            fail("artifact-manifest-entry", { role });
        }
        const folded = artifact.url.toLowerCase();
        if (urls.has(folded)) {
            fail("artifact-manifest-duplicate-url", { role });
        }
        urls.add(folded);
    }
}

async function fetchArtifact(url) {
    const resolved = new URL(url, document.baseURI);
    if (resolved.protocol !== "https:" || resolved.origin !== location.origin) {
        fail("artifact-url");
    }
    const response = await fetch(resolved, {
        cache: "no-store",
        credentials: "same-origin",
        redirect: "error",
    });
    if (!response.ok || response.url !== resolved.href) {
        fail("artifact-fetch", { url: resolved.href });
    }
    return Object.freeze({
        bytes: new Uint8Array(await response.arrayBuffer()),
        contentType: response.headers.get("content-type"),
        url: resolved.href,
    });
}

async function auditArtifact(role, artifact) {
    const fetched = await fetchArtifact(artifact.url);
    if (
        fetched.contentType !== artifact.mime
        || fetched.bytes.byteLength !== artifact.bytes
    ) {
        fail("artifact-response-shape", { role });
    }
    const actual = await digest(fetched.bytes);
    if (actual.hex !== artifact.sha256 || actual.sri !== artifact.sri) {
        fail("artifact-digest", { role });
    }
    return Object.freeze({
        sri: artifact.sri,
        url: fetched.url,
    });
}

function preflightFeatures() {
    if (!isSecureContext) {
        fail("preflight-secure-context");
    }
    if (!crossOriginIsolated) {
        fail("preflight-cross-origin-isolated");
    }
    if (
        typeof SharedArrayBuffer !== "function"
        || typeof Worker !== "function"
        || typeof AudioContext !== "function"
        || !("audioWorklet" in AudioContext.prototype)
        || typeof CSSStyleSheet !== "function"
        || typeof CSSStyleSheet.prototype.replaceSync !== "function"
        || typeof ShadowRoot !== "function"
        || !("adoptedStyleSheets" in ShadowRoot.prototype)
    ) {
        fail("preflight-browser-capabilities");
    }
}

async function preflightWorker(workerUrl) {
    const nonce = crypto.randomUUID();
    const worker = new Worker(workerUrl, {
        name: "rhythmgame-web-playtest-preflight",
        type: "module",
    });
    try {
        await new Promise((resolve, reject) => {
            const timeout = setTimeout(
                () => reject(new Error("worker-timeout")),
                5000,
            );
            worker.addEventListener("error", (event) => {
                clearTimeout(timeout);
                reject(event.error ?? new Error("worker-error"));
            }, { once: true });
            worker.addEventListener("message", (event) => {
                clearTimeout(timeout);
                if (
                    event.data?.nonce !== nonce
                    || event.data?.transport !== "dedicated-worker"
                ) {
                    reject(new Error("worker-response"));
                    return;
                }
                resolve();
            }, { once: true });
            worker.postMessage({ nonce });
        });
    } finally {
        worker.terminate();
    }
}

function sameDescriptor(left, right) {
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
    const originalDescriptor = Object.getOwnPropertyDescriptor(
        prototype,
        "appendChild",
    );
    const originalAppendChild = prototype.appendChild;
    let adoption = null;
    let active = true;

    function adapterAppendChild(child) {
        const root = this;
        const ownedStyle = (
            root instanceof ShadowRoot
            && child instanceof HTMLStyleElement
            && root.host.id === qtShadowHostId
            && root.host.shadowRoot === root
            && root.host.parentNode === screen
            && child.parentNode === null
        );
        if (!ownedStyle) {
            return Reflect.apply(originalAppendChild, root, arguments);
        }
        if (adoption !== null) {
            fail("qt-style-duplicate-root");
        }
        const text = child.textContent;
        const bytes = new TextEncoder().encode(text);
        if (
            !text.startsWith("\n")
            || bytes.byteLength !== qtStyleByteLength
            || text.toLowerCase().includes("@import")
            || text.toLowerCase().includes("url(")
            || qtStyleKeySelectors.some(
                (selector) => !text.includes(selector),
            )
        ) {
            fail("qt-style-shape");
        }
        const sheet = new CSSStyleSheet();
        sheet.replaceSync(text);
        if (sheet.cssRules.length !== qtStyleRuleCount) {
            fail("qt-style-shape");
        }
        const previousSheets = [...root.adoptedStyleSheets];
        root.adoptedStyleSheets = [...previousSheets, sheet];
        adoption = Object.freeze({
            bytes,
            child,
            previousSheets: Object.freeze(previousSheets),
            root,
            sheet,
        });
        return child;
    }

    const installedDescriptor = Object.freeze({
        configurable: true,
        enumerable: originalDescriptor?.enumerable ?? false,
        value: adapterAppendChild,
        writable: true,
    });
    Object.defineProperty(prototype, "appendChild", installedDescriptor);
    if (!sameDescriptor(
        Object.getOwnPropertyDescriptor(prototype, "appendChild"),
        installedDescriptor,
    )) {
        fail("qt-style-adapter-installation");
    }

    const restore = () => {
        if (!active) {
            return;
        }
        if (!sameDescriptor(
            Object.getOwnPropertyDescriptor(prototype, "appendChild"),
            installedDescriptor,
        )) {
            fail("qt-style-adapter-ownership");
        }
        if (originalDescriptor === undefined) {
            Reflect.deleteProperty(prototype, "appendChild");
        } else {
            Object.defineProperty(
                prototype,
                "appendChild",
                originalDescriptor,
            );
        }
        if (prototype.appendChild !== originalAppendChild) {
            fail("qt-style-adapter-restoration");
        }
        active = false;
    };

    return Object.freeze({
        async requireInitialAdoption() {
            if (adoption === null) {
                fail("qt-style-initial-missing");
            }
            const actual = await digest(adoption.bytes);
            if (
                actual.hex !== qtStyleSha256
                || adoption.root.querySelectorAll("style").length !== 0
                || adoption.child.parentNode !== null
                || adoption.sheet.cssRules.length !== qtStyleRuleCount
                || adoption.root.adoptedStyleSheets.filter(
                    (candidate) => candidate === adoption.sheet,
                ).length !== 1
            ) {
                fail("qt-style-fingerprint");
            }
            restore();
        },
        restore,
    });
}

async function injectVerifiedScript(asset) {
    await new Promise((resolve, reject) => {
        const script = document.createElement("script");
        script.src = asset.url;
        script.integrity = asset.sri;
        script.crossOrigin = "anonymous";
        script.addEventListener("load", resolve, { once: true });
        script.addEventListener("error", reject, { once: true });
        document.head.append(script);
    });
}

const bridgeLifecycle = installTestBridge();

async function bootstrap() {
    preflightFeatures();
    const manifestResponse = await fetch("runtime-artifacts.json", {
        cache: "no-store",
        credentials: "same-origin",
        redirect: "error",
    });
    if (
        !manifestResponse.ok
        || new URL(manifestResponse.url).origin !== location.origin
        || manifestResponse.headers.get("content-type")
            !== "application/json; charset=utf-8"
    ) {
        fail("artifact-manifest-response");
    }
    const manifest = await manifestResponse.json();
    validateManifest(manifest);

    const audited = {};
    for (const role of requiredRoles) {
        audited[role] = await auditArtifact(
            role,
            manifest.artifacts[role],
        );
    }
    if (
        new URL(import.meta.url).pathname.split("/").at(-1)
        !== manifest.artifacts.bootstrap.url
    ) {
        fail("artifact-bootstrap-url");
    }
    await preflightWorker(audited.preflightWorker.url);
    await injectVerifiedScript(audited.mainJs);
    await injectVerifiedScript(audited.qtloader);
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
    const styleAdapter = installQtShadowStyleAdapter(screen);
    try {
        await window.qtLoad({
            locateFile,
            onAbort: (text) => fail("runtime-abort", { text: String(text) }),
            onExit: (code) => fail("runtime-exit", { code }),
            qt: {
                containerElements: [screen],
                entryFunction: window.RhythmGameWasmProbe_entry,
                onExit: (exit) => fail("runtime-exit", exit),
            },
        });
        await styleAdapter.requireInitialAdoption();
    } catch (error) {
        styleAdapter.restore();
        throw error;
    }
}

bootstrap().catch((error) => {
    bridgeLifecycle.rejectReady(error);
    console.error("RhythmGame web playtest bootstrap failed", error);
});
