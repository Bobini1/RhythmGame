import { createHash } from "node:crypto";
import { lstat, readFile, realpath, stat } from "node:fs/promises";
import https from "node:https";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

import selfsigned from "selfsigned";
import { WebSocketServer } from "ws";

import {
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
const corruption = Buffer.from("\n/* gate-1b-negative-corruption */\n");

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

function addLog(requestLogs, request, route, statusCode, body, headers) {
    requestLogs.push(Object.freeze({
        bytes: body.length,
        contentType: headers["content-type"],
        method: request.method ?? "",
        policy: Object.freeze({
            contentSecurityPolicy: headers["content-security-policy"],
            crossOriginEmbedderPolicy:
                headers["cross-origin-embedder-policy"],
            crossOriginOpenerPolicy:
                headers["cross-origin-opener-policy"],
            crossOriginResourcePolicy:
                headers["cross-origin-resource-policy"],
        }),
        route,
        sha256: createHash("sha256").update(body).digest("hex"),
        status: statusCode,
    }));
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

function send(requestLogs, request, response, {
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
    response.writeHead(status, headers);
    response.end(request.method === "HEAD" ? undefined : payload);
    addLog(requestLogs, request, route, status, payload, headers);
}

function rejectUpgrade(
    requestLogs,
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
    const header = Buffer.from(
        [
            `HTTP/1.1 ${status} ${statusText}`,
            ...Object.entries(headers).map(
                ([name, value]) => `${name}: ${value}`,
            ),
            "",
            "",
        ].join("\r\n"),
        "utf8",
    );
    addLog(requestLogs, request, route, status, body, headers);
    socket.end(Buffer.concat([header, body]));
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
    const requestLogs = [];
    let port = 0;
    let origin = "";

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
                const nonce = parsed.searchParams.get("nonce") ?? "";
                send(requestLogs, request, response, {
                    body: jsonBytes({
                        nonce,
                        ok: true,
                        transport: "https",
                    }),
                    contentType: "application/json; charset=utf-8",
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
            let bytes = await readContainedFile(
                resolvedRuntimeDirectory,
                artifact.url,
            );
            const currentDigest = sha256(bytes);
            if (
                currentDigest !== artifact.sha256
                || sha256Sri(bytes) !== artifact.sri
                || bytes.length !== artifact.bytes
                || (
                    artifact.url !== "RhythmGameWasmProbe.html"
                    && !artifact.url.includes(`.${currentDigest}.`)
                )
            ) {
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
                    || artifact.url === "RhythmGameWasmProbe.html"
                    ? noStoreCache
                    : immutableCache,
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

    const websocketServer = new WebSocketServer({noServer: true});
    websocketServer.on("headers", (headers) => {
        headers.push(...websocketPolicyLines(port));
    });
    websocketServer.on("connection", (socket) => {
        socket.send(canonicalJson({
            type: "server-message",
            value: "connected",
        }));
        socket.on("message", (data, isBinary) => {
            if (isBinary) {
                socket.send(data, { binary: true });
                return;
            }
            const text = data.toString("utf8");
            if (text.startsWith("heartbeat:")) {
                socket.send(canonicalJson({
                    nonce: text.slice("heartbeat:".length),
                    type: "heartbeat",
                }));
            } else if (text === "close") {
                socket.close(1000, "probe-complete");
            } else {
                socket.send(text);
            }
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
        requestLogs,
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
