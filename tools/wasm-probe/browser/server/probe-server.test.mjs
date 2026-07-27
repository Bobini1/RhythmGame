import assert from "node:assert/strict";
import { createHash } from "node:crypto";
import {
    lstat,
    mkdtemp,
    readFile,
    rm,
    symlink,
    unlink,
    writeFile,
} from "node:fs/promises";
import https from "node:https";
import os from "node:os";
import path from "node:path";
import tls from "node:tls";
import { test } from "node:test";
import { fileURLToPath } from "node:url";

import WebSocket from "ws";

import {
    contentSecurityPolicy,
    negativeModes,
    responsePolicy,
} from "./policy.mjs";
import { validateArtifactManifest } from "./artifact-manifest.mjs";
import { startProbeServer } from "./probe-server.mjs";

const directory = path.dirname(fileURLToPath(import.meta.url));
const browserDirectory = path.resolve(directory, "..");
const probeDirectory = path.resolve(browserDirectory, "..");
const repositoryRoot = path.resolve(probeDirectory, "../..");
const buildId = "4".repeat(64);
const roles = {
    audioWorklet: ["RhythmGameWasmProbe.aw", "js", "text/javascript; charset=utf-8"],
    bootstrap: ["bootstrap", "mjs", "text/javascript; charset=utf-8"],
    css: ["probe", "css", "text/css; charset=utf-8"],
    mainJs: ["RhythmGameWasmProbe", "js", "text/javascript; charset=utf-8"],
    media: ["probe", "webm", "video/webm"],
    preflightWorker: ["preflight-worker", "mjs", "text/javascript; charset=utf-8"],
    qtloader: ["qtloader", "js", "text/javascript; charset=utf-8"],
    wasm: ["RhythmGameWasmProbe", "wasm", "application/wasm"],
    wasmWorker: ["RhythmGameWasmProbe.ww", "js", "text/javascript; charset=utf-8"],
};

test("bootstrap self-audit owns exact MIME and byte metadata", async () => {
    const source = await readFile(
        path.join(browserDirectory, "web", "bootstrap.mjs"),
        "utf8",
    );
    assert.match(
        source,
        /bootstrapResponse\.contentType\s*!==\s*bootstrapArtifact\.mime/,
    );
    assert.match(
        source,
        /bootstrapResponse\.bytes\.byteLength\s*!==\s*bootstrapArtifact\.bytes/,
    );
    assert.match(source, /fail\("artifact-bootstrap-mime"/);
    assert.match(source, /fail\("artifact-bootstrap-bytes"/);
});

function digest(bytes) {
    return createHash("sha256").update(bytes).digest("hex");
}

function sri(bytes) {
    return `sha256-${createHash("sha256").update(bytes).digest("base64")}`;
}

function canonical(value) {
    if (Array.isArray(value)) {
        return `[${value.map(canonical).join(",")}]`;
    }
    if (value && typeof value === "object") {
        return `{${Object.keys(value).sort().map(
            (key) => `${JSON.stringify(key)}:${canonical(value[key])}`,
        ).join(",")}}`;
    }
    return JSON.stringify(value);
}

async function makeRuntime() {
    const runtimeDirectory = await mkdtemp(
        path.join(os.tmpdir(), "rhythmgame-probe-server-"),
    );
    const artifacts = {};
    for (const [role, [stem, extension, mime]] of Object.entries(roles)) {
        const bytes = role === "wasm"
            ? Buffer.from([0, 97, 115, 109, 1, 0, 0, 0])
            : Buffer.from(`${role}\n`, "utf8");
        const sha256 = digest(bytes);
        const url = `${stem}.${sha256}.${extension}`;
        await writeFile(path.join(runtimeDirectory, url), bytes);
        artifacts[role] = {
            buildId,
            bytes: bytes.length,
            mime,
            sha256,
            sri: sri(bytes),
            url,
        };
    }

    const htmlBytes = Buffer.from(
        "<!doctype html><title>Gate 1B server fixture</title>\n",
        "utf8",
    );
    artifacts.html = {
        buildId,
        bytes: htmlBytes.length,
        mime: "text/html; charset=utf-8",
        sha256: digest(htmlBytes),
        sri: sri(htmlBytes),
        url: "RhythmGameWasmProbe.html",
    };
    await writeFile(
        path.join(runtimeDirectory, artifacts.html.url),
        htmlBytes,
    );
    const manifest = { artifacts, buildId, schemaVersion: 1 };
    await writeFile(
        path.join(runtimeDirectory, "runtime-artifacts.json"),
        `${canonical(manifest)}\n`,
        "utf8",
    );
    return { artifacts, manifest, runtimeDirectory };
}

function request(origin, route, options = {}) {
    return new Promise((resolve, reject) => {
        const url = new URL(route, origin);
        const request_ = https.request({
            agent: options.agent,
            hostname: url.hostname,
            path: options.rawPath ?? `${url.pathname}${url.search}`,
            port: url.port,
            protocol: url.protocol,
            headers: options.headers,
            method: options.method ?? "GET",
            rejectUnauthorized: false,
            setHost: options.setHost,
        }, (response) => {
            const chunks = [];
            response.on("data", (chunk) => chunks.push(chunk));
            response.on("end", () => resolve({
                body: Buffer.concat(chunks),
                headers: response.headers,
                status: response.statusCode,
            }));
        });
        request_.on("error", reject);
        request_.end(options.body);
    });
}

function rawRequest(origin, lines) {
    return new Promise((resolve, reject) => {
        const { hostname, port } = new URL(origin);
        const socket = tls.connect({
            host: hostname,
            port: Number(port),
            rejectUnauthorized: false,
        });
        const chunks = [];
        const timer = setTimeout(() => {
            socket.destroy();
            reject(new Error("raw HTTPS request timeout"));
        }, 5000);
        socket.once("secureConnect", () => {
            socket.end(`${lines.join("\r\n")}\r\n\r\n`);
        });
        socket.on("data", (chunk) => chunks.push(chunk));
        socket.once("error", (error) => {
            clearTimeout(timer);
            reject(error);
        });
        socket.once("end", () => {
            clearTimeout(timer);
            const bytes = Buffer.concat(chunks);
            const split = bytes.indexOf("\r\n\r\n");
            const head = bytes.subarray(0, split).toString("utf8");
            const [statusLine, ...headerLines] = head.split("\r\n");
            const headers = Object.fromEntries(headerLines.map((line) => {
                const colon = line.indexOf(":");
                return [
                    line.slice(0, colon).toLowerCase(),
                    line.slice(colon + 1).trim(),
                ];
            }));
            resolve({
                body: bytes.subarray(split + 4),
                headers,
                status: Number(statusLine.split(" ")[1]),
            });
        });
    });
}

function assertPolicy(response, origin, expectedCsp = contentSecurityPolicy(
    new URL(origin).port,
)) {
    for (const [name, value] of Object.entries(responsePolicy)) {
        assert.equal(response.headers[name], value, name);
    }
    assert.equal(response.headers["content-security-policy"], expectedCsp);
    assert.equal(response.headers["x-content-type-options"], "nosniff");
}

async function withServer(callback) {
    const fixture = await makeRuntime();
    const server = await startProbeServer({
        runtimeDirectory: fixture.runtimeDirectory,
    });
    try {
        await callback({ ...fixture, ...server });
    } finally {
        await server.close();
        await rm(fixture.runtimeDirectory, { force: true, recursive: true });
    }
}

test("one random loopback HTTPS port serves exact policy and MIME routes", async () => {
    await withServer(async ({ artifacts, origin, port }) => {
        assert.equal(new URL(origin).protocol, "https:");
        assert.equal(new URL(origin).hostname, "127.0.0.1");
        assert.notEqual(port, 0);

        const cases = [
            ["/", 307, "text/plain; charset=utf-8"],
            ["/RhythmGameWasmProbe.html", 200, "text/html; charset=utf-8"],
            ["/runtime-artifacts.json", 200, "application/json; charset=utf-8"],
            [`/${artifacts.bootstrap.url}`, 200, "text/javascript; charset=utf-8"],
            [`/${artifacts.wasm.url}`, 200, "application/wasm"],
            [`/${artifacts.preflightWorker.url}`, 200, "text/javascript; charset=utf-8"],
            [`/${artifacts.media.url}`, 200, "video/webm"],
            ["/probe/bfcache-away", 200, "text/html; charset=utf-8"],
            ["/probe/qnam?nonce=11", 200, "application/json; charset=utf-8"],
            ["/missing", 404, "application/json; charset=utf-8"],
            ["/probe/error", 500, "application/json; charset=utf-8"],
        ];
        for (const [route, status, mime] of cases) {
            const response = await request(origin, route);
            assert.equal(response.status, status, route);
            assert.equal(response.headers["content-type"], mime, route);
            assertPolicy(response, origin);
        }

        const qnam = await request(origin, "/probe/qnam?nonce=12");
        const qnamBody = JSON.parse(qnam.body.toString("utf8"));
        assert.deepEqual(qnamBody, {
            nonce: 12,
            ok: true,
            requestId: qnam.headers["x-rhythmgame-probe-request-id"],
            transport: "https",
        });
        assert.equal(
            qnam.headers["cache-control"],
            "no-store",
        );
        const applicationHtml = await request(
            origin,
            "/RhythmGameWasmProbe.html",
        );
        assert.equal(
            applicationHtml.headers["cache-control"],
            "no-cache",
        );
        const bfcacheAway = await request(origin, "/probe/bfcache-away");
        assert.equal(
            bfcacheAway.headers["cache-control"],
            "no-cache",
        );
        assert.equal(
            bfcacheAway.body.toString("utf8").match(
                /<link rel="icon" href="data:,">/g,
            )?.length,
            1,
        );
        const immutable = await request(origin, `/${artifacts.mainJs.url}`);
        assert.equal(
            immutable.headers["cache-control"],
            "public, max-age=31536000, immutable",
        );
    });
});

test("manifest roles are bound to exact semantic URL shapes", async () => {
    const fixture = await makeRuntime();
    try {
        const swapped = structuredClone(fixture.manifest);
        [
            swapped.artifacts.mainJs,
            swapped.artifacts.qtloader,
        ] = [
            swapped.artifacts.qtloader,
            swapped.artifacts.mainJs,
        ];
        assert.throws(
            () => validateArtifactManifest(swapped),
            /artifact mainJs has invalid URL/,
        );

        const duplicate = structuredClone(fixture.manifest);
        duplicate.artifacts.mainJs.url =
            duplicate.artifacts.qtloader.url.toUpperCase();
        assert.throws(
            () => validateArtifactManifest(duplicate),
            /duplicate artifact URL/i,
        );
    } finally {
        await rm(fixture.runtimeDirectory, { force: true, recursive: true });
    }
});

test("HTTP and WSS reject non-canonical Host aliases with policy", async () => {
    await withServer(async ({ origin, port, requestLogs }) => {
        const wrongHttp = await request(origin, "/probe/qnam", {
            headers: { host: `attacker.invalid:${port}` },
        });
        const missingHttp = await request(origin, "/probe/qnam", {
            setHost: false,
        });
        const duplicateHttp = await rawRequest(origin, [
            "GET /probe/qnam HTTP/1.1",
            `Host: 127.0.0.1:${port}`,
            `Host: attacker.invalid:${port}`,
            "Connection: close",
        ]);
        for (const [label, response] of [
            ["wrong HTTP Host", wrongHttp],
            ["missing HTTP Host", missingHttp],
            ["duplicate HTTP Host", duplicateHttp],
        ]) {
            assert.equal(
                response.status,
                421,
                `${label}: ${response.body.toString("utf8")}`,
            );
            assertPolicy(response, origin);
        }

        const websocketHeaders = [
            "Connection: Upgrade",
            "Upgrade: websocket",
            `Origin: ${origin}`,
            `Sec-WebSocket-Key: ${Buffer.alloc(16, 9).toString("base64")}`,
            "Sec-WebSocket-Version: 13",
        ];
        const wrongUpgrade = await request(origin, "/probe/ws", {
            headers: {
                connection: "Upgrade",
                host: `attacker.invalid:${port}`,
                origin,
                "sec-websocket-key":
                    Buffer.alloc(16, 9).toString("base64"),
                "sec-websocket-version": "13",
                upgrade: "websocket",
            },
        });
        const missingUpgrade = await request(origin, "/probe/ws", {
            headers: {
                connection: "Upgrade",
                origin,
                "sec-websocket-key":
                    Buffer.alloc(16, 9).toString("base64"),
                "sec-websocket-version": "13",
                upgrade: "websocket",
            },
            setHost: false,
        });
        const duplicateUpgrade = await rawRequest(origin, [
            "GET /probe/ws HTTP/1.1",
            `Host: ${new URL(origin).host}`,
            `Host: attacker.invalid:${port}`,
            ...websocketHeaders,
        ]);
        for (const response of [
            wrongUpgrade,
            missingUpgrade,
            duplicateUpgrade,
        ]) {
            assert.equal(response.status, 421);
            assertPolicy(response, origin);
        }
        assert.equal(
            requestLogs.filter(
                (entry) => entry.route === "/invalid-host"
                    && entry.status === 421,
            ).length,
            6,
        );
    });
});

test("media ranges are exact and retain policy on 206 and 416", async () => {
    await withServer(async ({ artifacts, origin }) => {
        const full = await request(origin, `/${artifacts.media.url}`);
        const partial = await request(origin, `/${artifacts.media.url}`, {
            headers: { range: "bytes=0-2" },
        });
        assert.equal(partial.status, 206);
        assert.deepEqual(partial.body, full.body.subarray(0, 3));
        assert.equal(partial.headers["content-range"], `bytes 0-2/${full.body.length}`);
        assertPolicy(partial, origin);

        const invalid = await request(origin, `/${artifacts.media.url}`, {
            headers: { range: `bytes=${full.body.length}-` },
        });
        assert.equal(invalid.status, 416);
        assert.equal(invalid.headers["content-range"], `bytes */${full.body.length}`);
        assertPolicy(invalid, origin);
    });
});

test("fixed media alias revalidates manifest bytes for every range outcome", async () => {
    await withServer(async ({ artifacts, origin, probeLogs }) => {
        const contentAddressed = await request(
            origin,
            `/${artifacts.media.url}`,
        );
        const full = await request(
            origin,
            "/fixtures/probe.webm?nonce=101",
        );
        assert.equal(full.status, 200);
        assert.deepEqual(full.body, contentAddressed.body);
        assert.equal(full.headers["cache-control"], "no-store");
        assert.equal(full.headers["content-type"], "video/webm");
        assert.equal(full.headers["accept-ranges"], "bytes");
        assert.match(
            full.headers["x-rhythmgame-probe-request-id"],
            /^request-[1-9][0-9]*$/,
        );

        const partial = await request(
            origin,
            "/fixtures/probe.webm?nonce=101",
            { headers: { range: "bytes=0-2" } },
        );
        assert.equal(partial.status, 206);
        assert.deepEqual(partial.body, full.body.subarray(0, 3));
        assert.equal(partial.headers["cache-control"], "no-store");
        assert.equal(
            partial.headers["content-range"],
            `bytes 0-2/${full.body.length}`,
        );

        const invalid = await request(
            origin,
            "/fixtures/probe.webm?nonce=101",
            { headers: { range: `bytes=${full.body.length}-` } },
        );
        assert.equal(invalid.status, 416);
        assert.equal(invalid.headers["cache-control"], "no-store");
        assert.equal(
            invalid.headers["content-range"],
            `bytes */${full.body.length}`,
        );

        const mediaLogs = probeLogs.filter(
            (entry) => entry.kind === "media" && entry.runNonce === 101,
        );
        assert.equal(mediaLogs.length, 3);
        assert.deepEqual(
            mediaLogs.map((entry) => entry.status),
            [200, 206, 416],
        );
        for (const entry of mediaLogs) {
            assert.match(entry.requestId, /^request-[1-9][0-9]*$/);
            assert.equal(entry.artifactBytes, artifacts.media.bytes);
            assert.equal(entry.artifactSha256, artifacts.media.sha256);
            assert.equal(entry.artifactSri, artifacts.media.sri);
            assert.equal(entry.route, "/fixtures/probe.webm");
        }
        assert.equal(new Set(
            mediaLogs.map((entry) => entry.requestId),
        ).size, 3);
        assert.throws(() => probeLogs.push({}), TypeError);
    });
});

test("fixed media alias fails closed after manifest artifact drift", async () => {
    await withServer(async ({
        artifacts,
        origin,
        runtimeDirectory,
    }) => {
        const artifactPath = path.join(
            runtimeDirectory,
            artifacts.media.url,
        );
        const original = await readFile(artifactPath);
        await writeFile(
            artifactPath,
            Buffer.concat([original, Buffer.from("drift\n")]),
        );
        for (const options of [
            {},
            { headers: { range: "bytes=0-2" } },
            { headers: { range: `bytes=${original.length}-` } },
        ]) {
            const response = await request(
                origin,
                "/fixtures/probe.webm?nonce=102",
                options,
            );
            assert.equal(response.status, 409);
            assert.equal(response.headers["cache-control"], "no-store");
            assert.deepEqual(
                JSON.parse(response.body.toString("utf8")),
                { error: "artifact-digest-mismatch" },
            );
        }
    });
});

test("QNAM response and append-only probe log share nonce and request ID", async () => {
    await withServer(async ({ origin, probeLogs }) => {
        const response = await request(
            origin,
            "/probe/qnam?nonce=4294967294",
        );
        assert.equal(response.status, 200);
        assert.equal(response.headers["access-control-allow-origin"], undefined);
        assert.equal(
            response.headers["content-type"],
            "application/json; charset=utf-8",
        );
        const body = JSON.parse(response.body.toString("utf8"));
        assert.deepEqual(body, {
            nonce: 4294967294,
            ok: true,
            requestId: response.headers["x-rhythmgame-probe-request-id"],
            transport: "https",
        });
        const matching = probeLogs.filter(
            (entry) => entry.kind === "qnam"
                && entry.runNonce === body.nonce
                && entry.requestId === body.requestId,
        );
        assert.equal(matching.length, 1);
        assert.equal(matching[0].status, 200);
        assert.equal(matching[0].route, "/probe/qnam");

        for (const nonce of ["", "0", "4294967295", "not-a-number"]) {
            const invalid = await request(
                origin,
                `/probe/qnam?nonce=${nonce}`,
            );
            assert.equal(invalid.status, 400);
        }
    });
});

test("normal artifact routes revalidate URL digests against current bytes", async () => {
    await withServer(async ({ artifacts, origin, runtimeDirectory }) => {
        const route = `/${artifacts.mainJs.url}`;
        const initial = await request(origin, route);
        assert.equal(initial.status, 200);
        await writeFile(
            path.join(runtimeDirectory, artifacts.mainJs.url),
            Buffer.concat([initial.body, Buffer.from("changed\n")]),
        );
        const changed = await request(origin, route);
        assert.equal(changed.status, 409);
        assert.deepEqual(
            JSON.parse(changed.body.toString("utf8")),
            { error: "artifact-digest-mismatch" },
        );
        assertPolicy(changed, origin);
    });
});

test("the served manifest cannot drift from the startup route snapshot", async () => {
    await withServer(async ({ origin, runtimeDirectory }) => {
        const initial = await request(origin, "/runtime-artifacts.json");
        assert.equal(initial.status, 200);
        await writeFile(
            path.join(runtimeDirectory, "runtime-artifacts.json"),
            "{}\n",
            "utf8",
        );
        const changed = await request(origin, "/runtime-artifacts.json");
        assert.equal(changed.status, 409);
        assert.deepEqual(
            JSON.parse(changed.body.toString("utf8")),
            { error: "artifact-manifest-drift" },
        );
        assertPolicy(changed, origin);
    });
});

test("unsafe and unknown routes fail closed without a shell fallback", async () => {
    await withServer(async ({ origin }) => {
        for (const route of [
            "/../runtime-artifacts.json",
            "/%2e%2e/runtime-artifacts.json",
            "/%2e%2e%2fruntime-artifacts.json",
            "/a//b",
            "/%5cruntime-artifacts.json",
            "/unknown.js",
        ]) {
            const response = await request(origin, route, { rawPath: route });
            assert.ok(
                response.status === 400 || response.status === 404,
                `${route}: ${response.status}`,
            );
            assert.notEqual(
                response.headers["content-type"],
                "text/html; charset=utf-8",
            );
            assertPolicy(response, origin);
        }
    });
});

test("WSS shares HTTPS origin and exposes one nonce-correlated probe stream", async () => {
    await withServer(async ({ origin, port, probeLogs }) => {
        const wrongOriginStatus = await new Promise((resolve, reject) => {
            const socket = new WebSocket(
                `wss://127.0.0.1:${port}/probe/ws?nonce=7`,
                {
                origin: "https://example.invalid",
                rejectUnauthorized: false,
                },
            );
            const timer = setTimeout(
                () => reject(new Error("wrong-Origin WSS timeout")),
                5000,
            );
            socket.once("unexpected-response", (_request, response) => {
                clearTimeout(timer);
                resolve({
                    headers: response.headers,
                    status: response.statusCode,
                });
            });
            socket.once("error", () => {});
        });
        assert.equal(wrongOriginStatus.status, 403);
        assertPolicy(wrongOriginStatus, origin);

        const socket = new WebSocket(
            `wss://127.0.0.1:${port}/probe/ws?nonce=7`,
            {
                origin,
                rejectUnauthorized: false,
            },
        );
        const acceptedUpgrade = new Promise((resolve) => {
            socket.once("upgrade", resolve);
        });
        const messages = [];
        const close = new Promise((resolve, reject) => {
            socket.on("message", (data, isBinary) => {
                messages.push({ data: Buffer.from(data), isBinary });
                if (messages.length === 1) {
                    socket.send("text-echo:7");
                } else if (messages.length === 2) {
                    socket.send(Buffer.from([1, 2, 3]));
                } else if (messages.length === 3) {
                    socket.send("heartbeat:7");
                } else if (messages.length === 4) {
                    socket.send("close");
                }
            });
            socket.on("close", (code, reason) => resolve({
                code,
                reason: reason.toString("utf8"),
            }));
            socket.on("error", reject);
        });
        const closed = await close;
        assertPolicy(await acceptedUpgrade, origin);
        const connected = JSON.parse(messages[0].data.toString("utf8"));
        assert.deepEqual(connected, {
            connectionId: connected.connectionId,
            nonce: 7,
            type: "server-message",
            value: "connected",
        });
        assert.match(connected.connectionId, /^connection-[1-9][0-9]*$/);
        assert.equal(messages[1].data.toString("utf8"), "text-echo:7");
        assert.equal(messages[1].isBinary, false);
        assert.deepEqual(messages[2].data, Buffer.from([1, 2, 3]));
        assert.equal(messages[2].isBinary, true);
        assert.deepEqual(
            JSON.parse(messages[3].data.toString("utf8")),
            { nonce: 7, type: "heartbeat" },
        );
        assert.deepEqual(closed, { code: 1000, reason: "probe-complete" });

        const stream = probeLogs.filter(
            (entry) => entry.kind === "wss"
                && entry.runNonce === 7
                && entry.connectionId === connected.connectionId,
        );
        assert.deepEqual(
            stream.map((entry) => entry.event),
            [
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
            ],
        );
        assert.equal(stream[0].origin, origin);
    });
});

test("WSS rejects oversized and out-of-order messages without materializing protocol logs", async () => {
    await withServer(async ({ origin, port, probeLogs }) => {
        const connect = () => new WebSocket(
            `wss://127.0.0.1:${port}/probe/ws?nonce=9`,
            {
                origin,
                rejectUnauthorized: false,
            },
        );
        const runViolation = async (sendViolation) => {
            const socket = connect();
            const greeting = new Promise((resolve, reject) => {
                socket.once("message", resolve);
                socket.once("error", reject);
            });
            const closed = new Promise((resolve) => {
                socket.once("close", (code, reason) => resolve({
                    code,
                    reason: reason.toString("utf8"),
                }));
            });
            await greeting;
            sendViolation(socket);
            return closed;
        };

        const orderViolation = await runViolation(
            (socket) => socket.send(Buffer.from([1, 2, 3])),
        );
        assert.deepEqual(orderViolation, {
            code: 1008,
            reason: "message-order",
        });

        const closeFirst = await runViolation(
            (socket) => socket.send("close"),
        );
        assert.deepEqual(closeFirst, {
            code: 1008,
            reason: "text-echo-mismatch",
        });

        const queuedAfterViolation = await runViolation((socket) => {
            socket.send("close");
            socket.send("text-echo:9");
            socket.send(Buffer.from([1, 2, 3]));
            socket.send("heartbeat:9");
            socket.send("close");
        });
        assert.deepEqual(queuedAfterViolation, {
            code: 1008,
            reason: "text-echo-mismatch",
        });

        const oversized = await runViolation(
            (socket) => socket.send(Buffer.alloc(4097, 0x41)),
        );
        assert.equal(oversized.code, 1009);

        const streams = probeLogs.filter(
            (entry) => entry.kind === "wss" && entry.runNonce === 9,
        );
        assert.equal(
            streams.some((entry) => (
                entry.event === "binary-received"
                || entry.event === "text-received"
            )),
            false,
        );
        for (const connectionId of new Set(
            streams.map((entry) => entry.connectionId),
        )) {
            assert.deepEqual(
                streams
                    .filter((entry) => entry.connectionId === connectionId)
                    .slice(0, 2)
                    .map((entry) => entry.event),
                ["opened", "server-message"],
            );
        }
    });
});

test("malformed same-origin WSS upgrades retain policy and request logs", async () => {
    await withServer(async ({ origin, port, requestLogs }) => {
        const baseHeaders = {
            connection: "Upgrade",
            origin,
            "sec-websocket-key": Buffer.alloc(16, 7).toString("base64"),
            "sec-websocket-version": "13",
            upgrade: "websocket",
        };
        const responses = [];
        responses.push(await request(origin, "/probe/ws", {
            headers: {
                ...baseHeaders,
                "sec-websocket-version": "12",
            },
        }));
        const missingKeyHeaders = { ...baseHeaders };
        delete missingKeyHeaders["sec-websocket-key"];
        responses.push(await request(origin, "/probe/ws", {
            headers: missingKeyHeaders,
        }));
        responses.push(await request(origin, "/probe/ws", {
            headers: {
                ...baseHeaders,
                "sec-websocket-protocol": "unexpected",
            },
        }));
        responses.push(await request(origin, "/probe/ws", {
            headers: {
                ...baseHeaders,
                "sec-websocket-extensions": "permessage-deflate",
            },
        }));
        responses.push(await rawRequest(origin, [
            "GET /probe/ws HTTP/1.1",
            `Host: 127.0.0.1:${port}`,
            "Connection: Upgrade",
            "Upgrade: websocket",
            `Origin: ${origin}`,
            `Sec-WebSocket-Key: ${baseHeaders["sec-websocket-key"]}`,
            "Sec-WebSocket-Version: 13",
            "Sec-WebSocket-Version: 13",
        ]));
        for (const response of responses) {
            assert.equal(response.status, 400);
            assert.equal(
                response.headers["content-type"],
                "application/json; charset=utf-8",
            );
            assertPolicy(response, origin);
        }
        assert.equal(requestLogs.filter(
            (entry) => entry.route === "/probe/ws"
                && entry.status === 400,
        ).length, responses.length);
    });
});

test("an allowlisted artifact symlink escape fails closed", async (t) => {
    const fixture = await makeRuntime();
    const outside = await mkdtemp(
        path.join(os.tmpdir(), "rhythmgame-probe-server-outside-"),
    );
    const artifact = fixture.artifacts.mainJs;
    const insidePath = path.join(fixture.runtimeDirectory, artifact.url);
    const outsidePath = path.join(outside, artifact.url);
    await writeFile(outsidePath, await readFile(insidePath));
    await unlink(insidePath);
    try {
        await symlink(outsidePath, insidePath, "file");
    } catch (error) {
        await rm(outside, { force: true, recursive: true });
        await rm(fixture.runtimeDirectory, { force: true, recursive: true });
        if (error?.code === "EPERM") {
            t.skip("creating a Windows test symlink requires Developer Mode");
            return;
        }
        throw error;
    }

    const server = await startProbeServer({
        runtimeDirectory: fixture.runtimeDirectory,
    });
    try {
        const response = await request(server.origin, `/${artifact.url}`);
        assert.equal(response.status, 500);
        assert.equal(
            response.headers["content-type"],
            "application/json; charset=utf-8",
        );
        assertPolicy(response, server.origin);
    } finally {
        await server.close();
        await rm(outside, { force: true, recursive: true });
        await rm(fixture.runtimeDirectory, { force: true, recursive: true });
    }
});

test("negative modes are isolated and alter only their named contract", async () => {
    await withServer(async ({ artifacts, origin }) => {
        assert.deepEqual(
            [...negativeModes].sort(),
            [
                "blocked-worker-src",
                "corrupt-bootstrap",
                "corrupt-main-js",
                "corrupt-qtloader",
                "corrupt-wasm",
                "missing-coep",
                "missing-coop",
                "missing-wasm-unsafe-eval",
                "native-depth-limit",
                "native-suspension-trap",
                "wrong-wasm-mime",
            ],
        );

        const normalManifest = await request(origin, "/runtime-artifacts.json");
        const missingCoop = await request(
            origin,
            "/negative/missing-coop/runtime-artifacts.json",
        );
        assert.equal(missingCoop.headers["cross-origin-opener-policy"], undefined);
        assert.equal(
            missingCoop.headers["cross-origin-embedder-policy"],
            normalManifest.headers["cross-origin-embedder-policy"],
        );

        const missingCoep = await request(
            origin,
            "/negative/missing-coep/runtime-artifacts.json",
        );
        assert.equal(missingCoep.headers["cross-origin-embedder-policy"], undefined);
        assert.equal(
            missingCoep.headers["cross-origin-opener-policy"],
            normalManifest.headers["cross-origin-opener-policy"],
        );

        const normalWasm = await request(origin, `/${artifacts.wasm.url}`);
        const wrongMime = await request(
            origin,
            `/negative/wrong-wasm-mime/${artifacts.wasm.url}`,
        );
        assert.deepEqual(wrongMime.body, normalWasm.body);
        assert.equal(wrongMime.headers["content-type"], "application/octet-stream");
        assert.equal(wrongMime.headers["cache-control"], "no-store");

        const corruptWasm = await request(
            origin,
            `/negative/corrupt-wasm/${artifacts.wasm.url}`,
        );
        assert.notDeepEqual(corruptWasm.body, normalWasm.body);
        assert.equal(corruptWasm.headers["content-type"], "application/wasm");

        const noWasmEval = await request(
            origin,
            "/negative/missing-wasm-unsafe-eval/runtime-artifacts.json",
        );
        assert.ok(!noWasmEval.headers["content-security-policy"].includes(
            "'wasm-unsafe-eval'",
        ));
        assert.ok(!contentSecurityPolicy(new URL(origin).port).includes(
            "'unsafe-eval'",
        ));
        assert.ok(!contentSecurityPolicy(new URL(origin).port).includes(
            "'unsafe-inline'",
        ));
        assert.ok(!contentSecurityPolicy(new URL(origin).port).includes("blob:"));

        const blockedWorkerDocument = await request(
            origin,
            "/negative/blocked-worker-src/RhythmGameWasmProbe.html",
        );
        assert.ok(blockedWorkerDocument.headers["content-security-policy"].includes(
            "worker-src 'none'",
        ));
        const blockedWorkerManifest = await request(
            origin,
            "/negative/blocked-worker-src/runtime-artifacts.json",
        );
        assert.equal(
            blockedWorkerManifest.headers["content-security-policy"],
            contentSecurityPolicy(new URL(origin).port),
        );

        const corruptMainFetch = await request(
            origin,
            `/negative/corrupt-main-js/${artifacts.mainJs.url}`,
            { headers: { "sec-fetch-dest": "empty" } },
        );
        const corruptMainScript = await request(
            origin,
            `/negative/corrupt-main-js/${artifacts.mainJs.url}`,
            { headers: { "sec-fetch-dest": "script" } },
        );
        assert.notDeepEqual(corruptMainFetch.body, corruptMainScript.body);

        const corruptLoaderFetch = await request(
            origin,
            `/negative/corrupt-qtloader/${artifacts.qtloader.url}`,
            { headers: { "sec-fetch-dest": "empty" } },
        );
        const corruptLoaderScript = await request(
            origin,
            `/negative/corrupt-qtloader/${artifacts.qtloader.url}`,
            { headers: { "sec-fetch-dest": "script" } },
        );
        assert.notDeepEqual(corruptLoaderFetch.body, corruptLoaderScript.body);

        const normalBootstrap = await request(
            origin,
            `/${artifacts.bootstrap.url}`,
        );
        const corruptBootstrap = await request(
            origin,
            `/negative/corrupt-bootstrap/${artifacts.bootstrap.url}`,
            { headers: { "sec-fetch-dest": "script" } },
        );
        assert.notDeepEqual(corruptBootstrap.body, normalBootstrap.body);

        for (const mode of [
            "native-depth-limit",
            "native-suspension-trap",
        ]) {
            const adversarialManifest = await request(
                origin,
                `/negative/${mode}/runtime-artifacts.json`,
            );
            assert.deepEqual(adversarialManifest.body, normalManifest.body);
            assert.equal(
                adversarialManifest.headers["content-security-policy"],
                normalManifest.headers["content-security-policy"],
            );
        }

        const normalAfterNegative = await request(origin, `/${artifacts.wasm.url}`);
        assert.deepEqual(normalAfterNegative.body, normalWasm.body);
    });
});

test("request logs are normalized, cryptographic, and contain no absolute paths", async () => {
    await withServer(async ({ origin, requestLogs, runtimeDirectory }) => {
        await request(origin, "/probe/qnam?nonce=13");
        await request(origin, "/missing");
        assert.ok(requestLogs.length >= 2);
        for (const entry of requestLogs) {
            assert.equal(typeof entry.method, "string");
            assert.match(entry.route, /^\//);
            assert.equal(typeof entry.status, "number");
            assert.equal(typeof entry.bytes, "number");
            assert.match(entry.sha256, /^[0-9a-f]{64}$/);
            assert.equal(typeof entry.contentType, "string");
            assert.equal(typeof entry.policy, "object");
            assert.ok(!JSON.stringify(entry).includes(runtimeDirectory));
            assert.ok(!JSON.stringify(entry).includes(repositoryRoot));
        }
    });
});

test("request logs are bounded and externally append-only", async () => {
    await withServer(async ({
        origin,
        port,
        probeLogs,
        requestLogs,
    }) => {
        const maximumEntries = 4096;
        const batchSize = 64;
        const agent = new https.Agent({
            keepAlive: true,
            maxSockets: batchSize,
            rejectUnauthorized: false,
        });
        try {
            for (
                let offset = 0;
                offset < maximumEntries;
                offset += batchSize
            ) {
                const responses = await Promise.all(
                    Array.from(
                        {
                            length: Math.min(
                                batchSize,
                                maximumEntries - offset,
                            ),
                        },
                        () => request(origin, "/probe/error", {
                            agent,
                            method: "HEAD",
                        }),
                    ),
                );
                assert.equal(
                    responses.every((response) => response.status === 500),
                    true,
                );
            }

            assert.equal(requestLogs.length, maximumEntries);
            const firstEntry = requestLogs[0];
            const overflow = await request(
                origin,
                "/probe/error",
                { agent },
            );
            assert.equal(overflow.status, 503);
            assertPolicy(overflow, origin);
            assert.deepEqual(
                JSON.parse(overflow.body.toString("utf8")),
                { error: "request-log-capacity" },
            );
            assert.equal(requestLogs.length, maximumEntries);
            assert.equal(requestLogs.overflowed, true);

            const probeLogLength = probeLogs.length;
            const refusedProbe = await request(
                origin,
                "/probe/qnam?nonce=4100",
                { agent },
            );
            assert.equal(refusedProbe.status, 503);
            assertPolicy(refusedProbe, origin);
            assert.deepEqual(
                JSON.parse(refusedProbe.body.toString("utf8")),
                { error: "request-log-capacity" },
            );
            assert.equal(probeLogs.length, probeLogLength);
            assert.equal(
                probeLogs.some(
                    (entry) => (
                        entry.kind === "qnam"
                        && entry.runNonce === 4100
                    ),
                ),
                false,
            );

            assert.equal(Reflect.set(requestLogs, "0", {}), false);
            assert.equal(Reflect.deleteProperty(requestLogs, "0"), false);
            assert.equal(
                Reflect.defineProperty(requestLogs, "0", { value: {} }),
                false,
            );
            assert.throws(() => requestLogs.push({}), TypeError);
            assert.throws(() => requestLogs.splice(0, 1), TypeError);
            assert.strictEqual(requestLogs[0], firstEntry);
            assert.equal(Object.isFrozen(firstEntry), true);
            assert.equal(Object.isFrozen(firstEntry.policy), true);
            assert.equal(
                Reflect.set(
                    firstEntry.policy,
                    "contentSecurityPolicy",
                    "tampered",
                ),
                false,
            );

            const afterOverflow = await request(
                origin,
                "/probe/error",
                { agent, method: "HEAD" },
            );
            assert.equal(afterOverflow.status, 503);
            assert.equal(afterOverflow.body.length, 0);
            assert.equal(requestLogs.length, maximumEntries);

            const rejectedUpgrade = await rawRequest(origin, [
                "GET /probe/ws HTTP/1.1",
                `Host: 127.0.0.1:${port}`,
                "Connection: Upgrade",
                "Upgrade: websocket",
            ]);
            assert.equal(rejectedUpgrade.status, 503);
            assertPolicy(rejectedUpgrade, origin);
            assert.deepEqual(
                JSON.parse(rejectedUpgrade.body.toString("utf8")),
                { error: "request-log-capacity" },
            );
            assert.equal(requestLogs.length, maximumEntries);
        } finally {
            agent.destroy();
        }
    });
});

test("the current Gate 1A output supports a non-runtime server smoke", async (t) => {
    const currentBuild = path.join(probeDirectory, "build", "wasm-release");
    try {
        await readFile(path.join(currentBuild, "RhythmGameWasmProbe.wasm"));
    } catch {
        t.skip("current Gate 1A build output is absent");
        return;
    }
    const server = await startProbeServer({
        allowMissingRuntimeManifestForSmoke: true,
        runtimeDirectory: currentBuild,
    });
    try {
        const response = await request(
            server.origin,
            "/probe/qnam?nonce=14",
        );
        assert.equal(response.status, 200);
        const responseBody = JSON.parse(response.body.toString("utf8"));
        assert.deepEqual(responseBody, {
            nonce: 14,
            ok: true,
            requestId: response.headers["x-rhythmgame-probe-request-id"],
            transport: "https",
        });
        assertPolicy(response, server.origin);
        assert.ok(server.requestLogs.some(
            (entry) => entry.route === "/probe/qnam" && entry.status === 200,
        ));
    } finally {
        await server.close();
    }
});

test("smoke fallback accepts only an absent runtime manifest", async (t) => {
    const fixture = await makeRuntime();
    try {
        await writeFile(
            path.join(fixture.runtimeDirectory, "runtime-artifacts.json"),
            "{invalid-json\n",
            "utf8",
        );
        await assert.rejects(
            startProbeServer({
                allowMissingRuntimeManifestForSmoke: true,
                runtimeDirectory: fixture.runtimeDirectory,
            }),
            /invalid runtime artifact manifest JSON/,
        );

        await unlink(path.join(
            fixture.runtimeDirectory,
            "runtime-artifacts.json",
        ));
        try {
            await symlink(
                path.join(fixture.runtimeDirectory, "missing-manifest.json"),
                path.join(
                    fixture.runtimeDirectory,
                    "runtime-artifacts.json",
                ),
                "file",
            );
        } catch (error) {
            if (error?.code === "EPERM") {
                t.diagnostic(
                    "dangling-link check skipped: Windows Developer Mode off",
                );
            } else {
                throw error;
            }
        }
        if (await lstat(path.join(
            fixture.runtimeDirectory,
            "runtime-artifacts.json",
        )).then(() => true, () => false)) {
            await assert.rejects(
                startProbeServer({
                    allowMissingRuntimeManifestForSmoke: true,
                    runtimeDirectory: fixture.runtimeDirectory,
                }),
                /ENOENT|no such file/i,
            );
            await unlink(path.join(
                fixture.runtimeDirectory,
                "runtime-artifacts.json",
            ));
        }

        const smoke = await startProbeServer({
            allowMissingRuntimeManifestForSmoke: true,
            runtimeDirectory: fixture.runtimeDirectory,
        });
        await smoke.close();
    } finally {
        await rm(fixture.runtimeDirectory, { force: true, recursive: true });
    }
});
