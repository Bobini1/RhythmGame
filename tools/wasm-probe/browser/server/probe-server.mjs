import { createHash } from "node:crypto";
import { lstat, readFile, realpath, stat } from "node:fs/promises";
import https from "node:https";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

import selfsigned from "selfsigned";
import { WebSocketServer } from "ws";

import {
    artifactForRole,
    loadArtifactManifest,
    sha256,
    sha256Sri,
} from "./artifact-manifest.mjs";
import {
    isNegativeMode,
    policyHeaders,
} from "./policy.mjs";

const host = "127.0.0.1";
const immutableCache = "public, max-age=31536000, immutable";
const noStoreCache = "no-store";
const revalidateCache = "no-cache";
const corruption = Buffer.from("\n/* gate-1b-negative-corruption */\n");
const maximumRunNonce = 0xFFFFFFFE;
const maximumProbeLogEntries = 4096;
const maximumRequestLogEntries = 4096;
const maximumWebSocketMessageBytes = 4096;
const expectedWebSocketBinary = Buffer.from([1, 2, 3]);

function canonicalJson(value) {
    if (Array.isArray(value)) {
        return `[${value.map(canonicalJson).join(",")}]`;
    }
    if (value !== null && typeof value === "object") {
        return `{${Object.keys(value).sort().map(
            (key) => `${JSON.stringify(key)}:${canonicalJson(value[key])}`,
        ).join(",")}}`;
    }
    return JSON.stringify(value);
}

function jsonBytes(value) {
    return Buffer.from(`${canonicalJson(value)}\n`, "utf8");
}

function deepFreezeJson(value) {
    if (value !== null && typeof value === "object") {
        for (const child of Object.values(value)) {
            deepFreezeJson(child);
        }
        Object.freeze(value);
    }
    return value;
}

function readOnlyAppendLog(maximumEntries) {
    if (!Number.isSafeInteger(maximumEntries) || maximumEntries <= 0) {
        throw new TypeError("log entry bound must be a positive integer");
    }
    const entries = [];
    let overflowed = false;
    const view = new Proxy(entries, {
        defineProperty() {
            return false;
        },
        deleteProperty() {
            return false;
        },
        get(target, property, receiver) {
            if (property === "overflowed") {
                return overflowed;
            }
            return Reflect.get(target, property, receiver);
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
    return {
        append(entry) {
            if (entries.length >= maximumEntries) {
                overflowed = true;
                return false;
            }
            entries.push(deepFreezeJson(entry));
            return true;
        },
        view,
    };
}

function parseRunNonce(searchParams) {
    const values = searchParams.getAll("nonce");
    if (
        values.length !== 1
        || !/^[1-9][0-9]{0,9}$/.test(values[0])
    ) {
        return null;
    }
    const runNonce = Number(values[0]);
    return Number.isSafeInteger(runNonce)
        && runNonce <= maximumRunNonce
        ? runNonce
        : null;
}

function parseRawRoute(rawUrl) {
    if (typeof rawUrl !== "string" || !rawUrl.startsWith("/")) {
        throw new Error("request target must be origin-form");
    }
    const queryIndex = rawUrl.indexOf("?");
    const rawPath = queryIndex < 0 ? rawUrl : rawUrl.slice(0, queryIndex);
    if (
        rawPath.includes("\\")
        || rawPath.includes("//")
        || /%(?:2f|5c)/i.test(rawPath)
    ) {
        throw new Error("encoded or duplicate separator");
    }
    let decoded;
    try {
        decoded = decodeURIComponent(rawPath);
    } catch {
        throw new Error("malformed percent encoding");
    }
    if (
        decoded.includes("\\")
        || decoded.includes("//")
        || decoded.split("/").some((part) => part === "." || part === "..")
    ) {
        throw new Error("unsafe path segment");
    }
    const url = new URL(rawUrl, "https://127.0.0.1");
    return { route: decoded, searchParams: url.searchParams };
}

function splitNegativeRoute(route) {
    const match = /^\/negative\/([^/]+)(\/.*)$/.exec(route);
    if (!match) {
        return { mode: null, route };
    }
    if (!isNegativeMode(match[1])) {
        return { invalid: true, mode: null, route };
    }
    return { mode: match[1], route: match[2] };
}

function contained(resolvedRoot, resolvedFile) {
    const relative = path.relative(resolvedRoot, resolvedFile);
    return (
        relative.length > 0
        && !path.isAbsolute(relative)
        && relative !== ".."
        && !relative.startsWith(`..${path.sep}`)
    );
}

async function readContainedFile(root, leaf) {
    const candidate = path.join(root, leaf);
    const resolved = await realpath(candidate);
    if (!contained(root, resolved)) {
        throw new Error("artifact escapes runtime directory");
    }
    const metadata = await stat(resolved);
    if (!metadata.isFile()) {
        throw new Error("artifact is not a regular file");
    }
    return readFile(resolved);
}

async function readVerifiedArtifact(root, artifact) {
    const bytes = await readContainedFile(root, artifact.url);
    const currentDigest = sha256(bytes);
    if (
        bytes.length !== artifact.bytes
        || currentDigest !== artifact.sha256
        || sha256Sri(bytes) !== artifact.sri
        || (
            artifact.url !== "RhythmGameWasmProbe.html"
            && !artifact.url.includes(`.${currentDigest}.`)
        )
    ) {
        return null;
    }
    return bytes;
}

function parseRange(value, length) {
    if (typeof value !== "string" || value.includes(",")) {
        return null;
    }
    const match = /^bytes=(\d*)-(\d*)$/.exec(value);
    if (!match || (!match[1] && !match[2])) {
        return null;
    }
    let start;
    let end;
    if (!match[1]) {
        const suffix = Number(match[2]);
        if (!Number.isSafeInteger(suffix) || suffix <= 0) {
            return null;
        }
        start = Math.max(0, length - suffix);
        end = length - 1;
    } else {
        start = Number(match[1]);
        end = match[2] ? Number(match[2]) : length - 1;
    }
    if (
        !Number.isSafeInteger(start)
        || !Number.isSafeInteger(end)
        || start < 0
        || start >= length
        || end < start
    ) {
        return null;
    }
    return { end: Math.min(end, length - 1), start };
}

function addLog(requestLog, request, route, statusCode, body, headers) {
    return requestLog.append({
        bytes: body.length,
        contentType: headers["content-type"],
        method: request.method ?? "",
        policy: {
            contentSecurityPolicy: headers["content-security-policy"],
            crossOriginEmbedderPolicy:
                headers["cross-origin-embedder-policy"],
            crossOriginOpenerPolicy:
                headers["cross-origin-opener-policy"],
            crossOriginResourcePolicy:
                headers["cross-origin-resource-policy"],
        },
        route,
        sha256: createHash("sha256").update(body).digest("hex"),
        status: statusCode,
    });
}

function requestLogCapacityResponse(port) {
    const body = jsonBytes({ error: "request-log-capacity" });
    return {
        body,
        headers: {
            ...policyHeaders(port),
            "cache-control": noStoreCache,
            connection: "close",
            "content-length": String(body.length),
            "content-type": "application/json; charset=utf-8",
        },
    };
}

function rawHeaderValues(request, expectedName) {
    const values = [];
    const expected = expectedName.toLowerCase();
    for (let index = 0; index < request.rawHeaders.length; index += 2) {
        if (request.rawHeaders[index].toLowerCase() === expected) {
            values.push(request.rawHeaders[index + 1]);
        }
    }
    return values;
}

function hasCanonicalHost(request, port) {
    const values = rawHeaderValues(request, "host");
    return (
        values.length === 1
        && values[0] === `${host}:${port}`
    );
}

function send(requestLog, request, response, {
    body,
    contentType,
    extraHeaders = {},
    mode = null,
    policyRoute,
    route,
    status = 200,
}) {
    const payload = Buffer.isBuffer(body) ? body : Buffer.from(body, "utf8");
    const headers = {
        ...policyHeaders(response.socket.localPort, {
            mode,
            route: policyRoute ?? route,
        }),
        "cache-control": noStoreCache,
        "content-length": String(payload.length),
        "content-type": contentType,
        ...extraHeaders,
    };
    if (!addLog(requestLog, request, route, status, payload, headers)) {
        const capacity = requestLogCapacityResponse(
            response.socket.localPort,
        );
        response.writeHead(503, capacity.headers);
        response.end(
            request.method === "HEAD" ? undefined : capacity.body,
        );
        return false;
    }
    response.writeHead(status, headers);
    response.end(request.method === "HEAD" ? undefined : payload);
    return true;
}

function rejectUpgrade(
    requestLog,
    request,
    socket,
    port,
    {
        error,
        route,
        status,
        statusText,
    },
) {
    const body = jsonBytes({ error });
    const headers = {
        ...policyHeaders(port),
        "cache-control": noStoreCache,
        "content-length": String(body.length),
        "content-type": "application/json; charset=utf-8",
        connection: "close",
    };
    const capacity = addLog(
        requestLog,
        request,
        route,
        status,
        body,
        headers,
    )
        ? null
        : requestLogCapacityResponse(port);
    const responseBody = capacity?.body ?? body;
    const responseHeaders = capacity?.headers ?? headers;
    const responseStatus = capacity === null ? status : 503;
    const responseStatusText = capacity === null
        ? statusText
        : "Service Unavailable";
    const header = Buffer.from(
        [
            `HTTP/1.1 ${responseStatus} ${responseStatusText}`,
            ...Object.entries(responseHeaders).map(
                ([name, value]) => `${name}: ${value}`,
            ),
            "",
            "",
        ].join("\r\n"),
        "utf8",
    );
    socket.end(Buffer.concat([header, responseBody]));
}

function validWebSocketHandshake(request) {
    const connections = rawHeaderValues(request, "connection");
    const upgrades = rawHeaderValues(request, "upgrade");
    const origins = rawHeaderValues(request, "origin");
    const connectionTokens = String(
        request.headers.connection ?? "",
    ).split(",").map((value) => value.trim().toLowerCase());
    const keys = rawHeaderValues(request, "sec-websocket-key");
    const versions = rawHeaderValues(request, "sec-websocket-version");
    if (
        request.method !== "GET"
        || connections.length !== 1
        || upgrades.length !== 1
        || origins.length !== 1
        || String(request.headers.upgrade ?? "").toLowerCase() !== "websocket"
        || !connectionTokens.includes("upgrade")
        || keys.length !== 1
        || versions.length !== 1
        || versions[0] !== "13"
    ) {
        return false;
    }
    const key = keys[0];
    if (
        !/^[A-Za-z0-9+/]{22}==$/.test(key)
        || Buffer.from(key, "base64").length !== 16
    ) {
        return false;
    }
    const extensions = rawHeaderValues(
        request,
        "sec-websocket-extensions",
    );
    return (
        rawHeaderValues(request, "sec-websocket-protocol").length === 0
        && (
            extensions.length === 0
            || (
                extensions.length === 1
                && extensions[0]
                    === "permessage-deflate; client_max_window_bits"
            )
        )
    );
}

async function certificateOptions(certificate, privateKey) {
    if ((certificate && !privateKey) || (!certificate && privateKey)) {
        throw new Error(
            "--certificate and --private-key must be provided together",
        );
    }
    if (certificate) {
        return {
            cert: await readFile(certificate),
            key: await readFile(privateKey),
        };
    }
    const generated = await selfsigned.generate(
        [{ name: "commonName", value: host }],
        {
            algorithm: "sha256",
            days: 1,
            extensions: [{
                altNames: [{ ip: host, type: 7 }],
                name: "subjectAltName",
            }],
            keySize: 2048,
        },
    );
    return { cert: generated.cert, key: generated.private };
}

function websocketPolicyLines(port) {
    return Object.entries(policyHeaders(port)).map(
        ([name, value]) => `${name}: ${value}`,
    );
}

export async function startProbeServer({
    allowMissingRuntimeManifestForSmoke = false,
    certificate,
    privateKey,
    runtimeDirectory,
} = {}) {
    if (!runtimeDirectory) {
        throw new Error("runtimeDirectory is required");
    }
    const resolvedRuntimeDirectory = await realpath(runtimeDirectory);
    let loaded = null;
    let hasRuntimeManifest = true;
    try {
        await lstat(path.join(
            resolvedRuntimeDirectory,
            "runtime-artifacts.json",
        ));
    } catch (error) {
        if (
            !allowMissingRuntimeManifestForSmoke
            || error?.code !== "ENOENT"
        ) {
            throw error;
        }
        hasRuntimeManifest = false;
    }
    if (hasRuntimeManifest) {
        loaded = await loadArtifactManifest(resolvedRuntimeDirectory);
    }
    const tls = await certificateOptions(certificate, privateKey);
    const requestLogs = readOnlyAppendLog(maximumRequestLogEntries);
    const probeLog = readOnlyAppendLog(maximumProbeLogEntries);
    let requestSequence = 0;
    let connectionSequence = 0;
    let port = 0;
    let origin = "";
    const nextRequestId = () => `request-${++requestSequence}`;
    const nextConnectionId = () => `connection-${++connectionSequence}`;

    const server = https.createServer({
        ...tls,
        requireHostHeader: false,
    }, async (request, response) => {
        if (!hasCanonicalHost(request, response.socket.localPort)) {
            send(requestLogs, request, response, {
                body: jsonBytes({ error: "invalid-host" }),
                contentType: "application/json; charset=utf-8",
                route: "/invalid-host",
                status: 421,
            });
            return;
        }
        let parsed;
        try {
            parsed = parseRawRoute(request.url);
        } catch (error) {
            send(requestLogs, request, response, {
                body: jsonBytes({ error: "invalid-route" }),
                contentType: "application/json; charset=utf-8",
                route: "/invalid",
                status: 400,
            });
            return;
        }
        const negative = splitNegativeRoute(parsed.route);
        if (negative.invalid) {
            send(requestLogs, request, response, {
                body: jsonBytes({ error: "not-found" }),
                contentType: "application/json; charset=utf-8",
                route: parsed.route,
                status: 404,
            });
            return;
        }
        const { mode, route } = negative;
        const normalizedLogRoute = mode
            ? `/negative/${mode}${route}`
            : route;

        try {
            if (request.method !== "GET" && request.method !== "HEAD") {
                send(requestLogs, request, response, {
                    body: jsonBytes({ error: "method-not-allowed" }),
                    contentType: "application/json; charset=utf-8",
                    extraHeaders: { allow: "GET, HEAD" },
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: 405,
                });
                return;
            }
            if (route === "/") {
                send(requestLogs, request, response, {
                    body: "Temporary redirect\n",
                    contentType: "text/plain; charset=utf-8",
                    extraHeaders: {
                        location: mode
                            ? `/negative/${mode}/RhythmGameWasmProbe.html`
                            : "/RhythmGameWasmProbe.html",
                    },
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: 307,
                });
                return;
            }
            if (route === "/probe/qnam") {
                const runNonce = parseRunNonce(parsed.searchParams);
                if (runNonce === null) {
                    send(requestLogs, request, response, {
                        body: jsonBytes({ error: "invalid-run-nonce" }),
                        contentType: "application/json; charset=utf-8",
                        mode,
                        policyRoute: route,
                        route: normalizedLogRoute,
                        status: 400,
                    });
                    return;
                }
                const requestId = nextRequestId();
                const body = jsonBytes({
                    nonce: runNonce,
                    ok: true,
                    requestId,
                    transport: "https",
                });
                const sent = send(requestLogs, request, response, {
                    body,
                    contentType: "application/json; charset=utf-8",
                    extraHeaders: {
                        "x-rhythmgame-probe-request-id": requestId,
                    },
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                });
                if (sent) {
                    probeLog.append({
                        bodySha256: sha256(body),
                        event: "response",
                        kind: "qnam",
                        method: request.method,
                        requestId,
                        route,
                        runNonce,
                        status: 200,
                    });
                }
                return;
            }
            if (route === "/probe/bfcache-away") {
                send(requestLogs, request, response, {
                    body: (
                        "<!doctype html><meta charset=\"utf-8\">"
                        + "<link rel=\"icon\" href=\"data:,\">"
                        + "<title>Gate 1B BFCache away</title>\n"
                    ),
                    contentType: "text/html; charset=utf-8",
                    extraHeaders: {
                        "cache-control": revalidateCache,
                    },
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                });
                return;
            }
            if (route === "/probe/error") {
                send(requestLogs, request, response, {
                    body: jsonBytes({ error: "owned-probe-error" }),
                    contentType: "application/json; charset=utf-8",
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: 500,
                });
                return;
            }
            if (route === "/runtime-artifacts.json" && loaded) {
                const bytes = await readContainedFile(
                    resolvedRuntimeDirectory,
                    "runtime-artifacts.json",
                );
                if (!bytes.equals(loaded.bytes)) {
                    send(requestLogs, request, response, {
                        body: jsonBytes({
                            error: "artifact-manifest-drift",
                        }),
                        contentType: "application/json; charset=utf-8",
                        mode,
                        policyRoute: route,
                        route: normalizedLogRoute,
                        status: 409,
                    });
                    return;
                }
                send(requestLogs, request, response, {
                    body: loaded.bytes,
                    contentType: "application/json; charset=utf-8",
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                });
                return;
            }

            if (route === "/fixtures/probe.webm") {
                const runNonce = parseRunNonce(parsed.searchParams);
                if (runNonce === null || loaded === null) {
                    send(requestLogs, request, response, {
                        body: jsonBytes({
                            error: runNonce === null
                                ? "invalid-run-nonce"
                                : "runtime-manifest-required",
                        }),
                        contentType: "application/json; charset=utf-8",
                        mode,
                        policyRoute: route,
                        route: normalizedLogRoute,
                        status: runNonce === null ? 400 : 503,
                    });
                    return;
                }
                const artifact = artifactForRole(
                    loaded.manifest,
                    "media",
                );
                let bytes = await readVerifiedArtifact(
                    resolvedRuntimeDirectory,
                    artifact,
                );
                if (bytes === null) {
                    send(requestLogs, request, response, {
                        body: jsonBytes({
                            error: "artifact-digest-mismatch",
                        }),
                        contentType: "application/json; charset=utf-8",
                        extraHeaders: {
                            "cache-control": noStoreCache,
                        },
                        mode,
                        policyRoute: route,
                        route: normalizedLogRoute,
                        status: 409,
                    });
                    return;
                }

                const requestId = nextRequestId();
                const fullLength = bytes.length;
                const extraHeaders = {
                    "accept-ranges": "bytes",
                    "cache-control": noStoreCache,
                    "x-rhythmgame-probe-request-id": requestId,
                };
                let statusCode = 200;
                if (request.headers.range) {
                    const range = parseRange(
                        request.headers.range,
                        fullLength,
                    );
                    if (range === null) {
                        extraHeaders["content-range"] =
                            `bytes */${fullLength}`;
                        bytes = Buffer.alloc(0);
                        statusCode = 416;
                    } else {
                        extraHeaders["content-range"] =
                            `bytes ${range.start}-${range.end}/${fullLength}`;
                        bytes = bytes.subarray(
                            range.start,
                            range.end + 1,
                        );
                        statusCode = 206;
                    }
                }
                send(requestLogs, request, response, {
                    body: bytes,
                    contentType: artifact.mime,
                    extraHeaders,
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: statusCode,
                });
                probeLog.append({
                    artifactBytes: artifact.bytes,
                    artifactSha256: artifact.sha256,
                    artifactSri: artifact.sri,
                    event: "response",
                    kind: "media",
                    method: request.method,
                    range: request.headers.range ?? null,
                    requestId,
                    responseBytes: bytes.length,
                    responseSha256: sha256(bytes),
                    route,
                    runNonce,
                    status: statusCode,
                });
                return;
            }

            const artifactEntry = loaded
                ? Object.entries(loaded.manifest.artifacts).find(
                    ([, candidate]) => `/${candidate.url}` === route,
                )
                : null;
            if (!artifactEntry) {
                send(requestLogs, request, response, {
                    body: jsonBytes({ error: "not-found" }),
                    contentType: "application/json; charset=utf-8",
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: 404,
                });
                return;
            }
            const [role, artifact] = artifactEntry;
            let bytes = await readVerifiedArtifact(
                resolvedRuntimeDirectory,
                artifact,
            );
            if (bytes === null) {
                send(requestLogs, request, response, {
                    body: jsonBytes({ error: "artifact-digest-mismatch" }),
                    contentType: "application/json; charset=utf-8",
                    mode,
                    policyRoute: route,
                    route: normalizedLogRoute,
                    status: 409,
                });
                return;
            }

            let contentType = artifact.mime;
            if (mode === "wrong-wasm-mime" && artifact.mime === "application/wasm") {
                contentType = "application/octet-stream";
            } else if (
                mode === "corrupt-wasm"
                && artifact.mime === "application/wasm"
            ) {
                bytes = Buffer.concat([bytes, corruption]);
            } else if (
                (
                    mode === "corrupt-bootstrap"
                    && role === "bootstrap"
                )
                || (
                    mode === "corrupt-main-js"
                    && role === "mainJs"
                )
                || (
                    mode === "corrupt-qtloader"
                    && role === "qtloader"
                )
            ) {
                if (request.headers["sec-fetch-dest"] === "script") {
                    bytes = Buffer.concat([bytes, corruption]);
                }
            }

            const extraHeaders = {
                "cache-control": mode !== null
                    ? noStoreCache
                    : (
                        artifact.url === "RhythmGameWasmProbe.html"
                            ? revalidateCache
                            : immutableCache
                    ),
            };
            let statusCode = 200;
            if (artifact.mime === "video/webm" && request.headers.range) {
                const range = parseRange(request.headers.range, bytes.length);
                extraHeaders["accept-ranges"] = "bytes";
                if (!range) {
                    extraHeaders["content-range"] = `bytes */${bytes.length}`;
                    bytes = Buffer.alloc(0);
                    statusCode = 416;
                } else {
                    const fullLength = bytes.length;
                    bytes = bytes.subarray(range.start, range.end + 1);
                    extraHeaders["content-range"] =
                        `bytes ${range.start}-${range.end}/${fullLength}`;
                    statusCode = 206;
                }
            }
            send(requestLogs, request, response, {
                body: bytes,
                contentType,
                extraHeaders,
                mode,
                policyRoute: route,
                route: normalizedLogRoute,
                status: statusCode,
            });
        } catch {
            send(requestLogs, request, response, {
                body: jsonBytes({ error: "request-failed-closed" }),
                contentType: "application/json; charset=utf-8",
                mode,
                policyRoute: route,
                route: normalizedLogRoute,
                status: 500,
            });
        }
    });

    const websocketServer = new WebSocketServer({
        maxPayload: maximumWebSocketMessageBytes,
        noServer: true,
        perMessageDeflate: false,
    });
    websocketServer.on("headers", (headers, request) => {
        headers.push(...websocketPolicyLines(port));
        if (request.gate1bProbeContext !== undefined) {
            headers.push(
                "X-RhythmGame-Probe-Connection-Id: "
                + request.gate1bProbeContext.connectionId,
            );
        }
    });
    websocketServer.on("connection", (socket, request) => {
        const context = request.gate1bProbeContext;
        if (context === undefined) {
            socket.close(1011, "probe-context-missing");
            return;
        }
        const append = (event, detail = {}) => (
            probeLog.append({
                connectionId: context.connectionId,
                event,
                kind: "wss",
                runNonce: context.runNonce,
                ...detail,
            })
        );
        let protocolStep = "text-echo";
        let terminal = false;
        const closeTerminal = (code, reason, nextStep = "failed") => {
            if (terminal) {
                return;
            }
            terminal = true;
            protocolStep = nextStep;
            socket.close(code, reason);
        };
        const closeForPolicyViolation = (reason) => {
            closeTerminal(1008, reason);
        };
        if (!append("opened", {
            origin: context.origin,
            route: "/probe/ws",
        })) {
            closeTerminal(1013, "probe-log-capacity");
            return;
        }
        socket.send(canonicalJson({
            connectionId: context.connectionId,
            nonce: context.runNonce,
            type: "server-message",
            value: "connected",
        }));
        if (!append("server-message")) {
            closeTerminal(1013, "probe-log-capacity");
            return;
        }
        let receivedMessageCount = 0;
        socket.on("message", (data, isBinary) => {
            if (terminal) {
                return;
            }
            ++receivedMessageCount;
            if (receivedMessageCount > 4) {
                closeForPolicyViolation("message-count");
                return;
            }
            if (protocolStep === "text-echo") {
                if (isBinary) {
                    closeForPolicyViolation("message-order");
                    return;
                }
                const text = data.toString("utf8");
                const expectedText = `text-echo:${context.runNonce}`;
                if (text !== expectedText) {
                    closeForPolicyViolation("text-echo-mismatch");
                    return;
                }
                if (!append("text-received", { text })) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                socket.send(text);
                if (!append("text-echoed", { text })) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                protocolStep = "binary-echo";
                return;
            }
            if (protocolStep === "binary-echo") {
                if (!isBinary) {
                    closeForPolicyViolation("message-order");
                    return;
                }
                const bytes = Buffer.from(data);
                if (!bytes.equals(expectedWebSocketBinary)) {
                    closeForPolicyViolation("binary-echo-mismatch");
                    return;
                }
                if (!append("binary-received", {
                    bytes: bytes.length,
                    sha256: sha256(bytes),
                })) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                socket.send(data, { binary: true });
                if (!append("binary-echoed", {
                    bytes: bytes.length,
                    sha256: sha256(bytes),
                })) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                protocolStep = "heartbeat";
                return;
            }
            if (protocolStep === "heartbeat") {
                if (isBinary) {
                    closeForPolicyViolation("message-order");
                    return;
                }
                const text = data.toString("utf8");
                if (text !== `heartbeat:${context.runNonce}`) {
                    closeForPolicyViolation("nonce-mismatch");
                    return;
                }
                if (!append("heartbeat-received", { text })) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                socket.send(canonicalJson({
                    nonce: context.runNonce,
                    type: "heartbeat",
                }));
                if (!append("heartbeat-sent")) {
                    closeTerminal(1013, "probe-log-capacity");
                    return;
                }
                protocolStep = "close";
                return;
            }
            if (protocolStep !== "close" || isBinary) {
                closeForPolicyViolation("message-order");
                return;
            }
            const text = data.toString("utf8");
            if (text !== "close") {
                closeForPolicyViolation("close-mismatch");
                return;
            }
            if (!append("close-requested")) {
                closeTerminal(1013, "probe-log-capacity");
                return;
            }
            closeTerminal(1000, "probe-complete", "complete");
        });
        socket.on("error", () => {
            terminal = true;
            if (protocolStep !== "complete") {
                protocolStep = "failed";
            }
        });
        socket.on("close", (code, reason) => {
            terminal = true;
            if (protocolStep !== "complete") {
                protocolStep = "failed";
            }
            append("closed", {
                code,
                reason: reason.toString("utf8"),
            });
        });
    });

    server.on("clientError", (error, socket) => {
        if (!socket.writable) {
            return;
        }
        const rawPacket = Buffer.isBuffer(error.rawPacket)
            ? error.rawPacket.toString("latin1")
            : "";
        const lines = rawPacket.split("\r\n");
        const hostCount = lines.filter(
            (line) => /^host:/i.test(line),
        ).length;
        const method = lines[0]?.split(" ")[0] ?? "";
        const invalidHost = hostCount !== 1;
        rejectUpgrade(
            requestLogs,
            { method },
            socket,
            socket.localPort ?? port,
            {
                error: invalidHost ? "invalid-host" : "malformed-http",
                route: invalidHost ? "/invalid-host" : "/invalid",
                status: invalidHost ? 421 : 400,
                statusText: invalidHost
                    ? "Misdirected Request"
                    : "Bad Request",
            },
        );
    });

    server.on("upgrade", (request, socket, head) => {
        if (!hasCanonicalHost(request, port)) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "invalid-host",
                route: "/invalid-host",
                status: 421,
                statusText: "Misdirected Request",
            });
            return;
        }
        let parsed;
        try {
            parsed = parseRawRoute(request.url);
        } catch {
            parsed = null;
        }
        const expectedOrigin = origin;
        if (
            !parsed
            || parsed.route !== "/probe/ws"
        ) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "forbidden-upgrade",
                route: parsed?.route ?? "/invalid",
                status: 403,
                statusText: "Forbidden",
            });
            return;
        }
        const runNonce = parseRunNonce(parsed.searchParams);
        if (runNonce === null) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "invalid-run-nonce",
                route: parsed.route,
                status: 400,
                statusText: "Bad Request",
            });
            return;
        }
        if (rawHeaderValues(request, "origin").length !== 1) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "malformed-websocket-upgrade",
                route: parsed.route,
                status: 400,
                statusText: "Bad Request",
            });
            return;
        }
        if (request.headers.origin !== expectedOrigin) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "forbidden-upgrade",
                route: parsed.route,
                status: 403,
                statusText: "Forbidden",
            });
            return;
        }
        if (!validWebSocketHandshake(request)) {
            rejectUpgrade(requestLogs, request, socket, port, {
                error: "malformed-websocket-upgrade",
                route: parsed.route,
                status: 400,
                statusText: "Bad Request",
            });
            return;
        }
        request.gate1bProbeContext = Object.freeze({
            connectionId: nextConnectionId(),
            origin: request.headers.origin,
            runNonce,
        });
        websocketServer.handleUpgrade(request, socket, head, (websocket) => {
            websocketServer.emit("connection", websocket, request);
        });
    });

    await new Promise((resolve, reject) => {
        server.once("error", reject);
        server.listen(0, host, () => {
            server.off("error", reject);
            resolve();
        });
    });
    const address = server.address();
    if (!address || typeof address === "string") {
        throw new Error("HTTPS server did not expose an IPv4 address");
    }
    port = address.port;
    origin = `https://${host}:${port}`;

    return Object.freeze({
        close: () => new Promise((resolve, reject) => {
            websocketServer.clients.forEach((client) => client.terminate());
            websocketServer.close();
            server.close((error) => error ? reject(error) : resolve());
        }),
        origin,
        port,
        probeLogs: probeLog.view,
        requestLogs: requestLogs.view,
    });
}

function parseArguments(arguments_) {
    const options = {};
    for (let index = 0; index < arguments_.length; index += 1) {
        const name = arguments_[index];
        if (![
            "--certificate",
            "--private-key",
            "--runtime-directory",
        ].includes(name)) {
            throw new Error(`Unknown probe-server argument: ${name}`);
        }
        const value = arguments_[index + 1];
        if (!value || value.startsWith("--")) {
            throw new Error(`Missing value for ${name}`);
        }
        index += 1;
        if (name === "--certificate") {
            options.certificate = value;
        } else if (name === "--private-key") {
            options.privateKey = value;
        } else {
            options.runtimeDirectory = value;
        }
    }
    return options;
}

const invokedAsScript = process.argv[1]
    && fileURLToPath(import.meta.url) === path.resolve(process.argv[1]);
if (invokedAsScript) {
    const instance = await startProbeServer(parseArguments(process.argv.slice(2)));
    const ready = {
        origin: instance.origin,
        port: instance.port,
        type: "rhythmgame-gate1b-origin-ready",
    };
    if (typeof process.send === "function") {
        process.send(ready);
    } else {
        process.stdout.write(`${canonicalJson(ready)}\n`);
    }
    const stop = async () => {
        await instance.close();
        process.exitCode = 0;
    };
    process.once("SIGINT", stop);
    process.once("SIGTERM", stop);
}
