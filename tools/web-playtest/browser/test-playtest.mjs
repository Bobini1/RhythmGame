import assert from "node:assert/strict";
import http from "node:http";
import https from "node:https";
import { mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

import {
    launchExternalLifecycleBrowser,
} from "../../wasm-probe/browser/lib/external-lifecycle-browser.mjs";
import {
    startProbeServer,
} from "../../wasm-probe/browser/server/probe-server.mjs";

const defaultInputSequence = fileURLToPath(new URL(
    "../tests/fixtures/canonical-input.json",
    import.meta.url,
));
const terminalPhases = new Set([5, 6, 7]);

function delay(milliseconds) {
    return new Promise((resolve) => {
        setTimeout(resolve, milliseconds);
    });
}

function parseArguments(arguments_) {
    const options = { inputSequence: defaultInputSequence };
    for (let index = 0; index < arguments_.length; index += 2) {
        const name = arguments_[index];
        const value = arguments_[index + 1];
        if (
            !["--input-sequence", "--runtime-directory"].includes(name)
            || !value
            || value.startsWith("--")
        ) {
            throw new Error(`Invalid test-playtest argument: ${name}`);
        }
        if (name === "--runtime-directory") {
            options.runtimeDirectory = path.resolve(value);
        } else {
            options.inputSequence = path.resolve(value);
        }
    }
    if (!options.runtimeDirectory) {
        throw new Error("--runtime-directory is required");
    }
    return Object.freeze(options);
}

function request(origin, route, {
    plaintext = false,
    rawPath = false,
} = {}) {
    const url = new URL(route, origin);
    const transport = plaintext ? http : https;
    return new Promise((resolve, reject) => {
        const operation = transport.request({
            headers: { host: `127.0.0.1:${url.port}` },
            hostname: "127.0.0.1",
            method: "GET",
            path: rawPath ? route : `${url.pathname}${url.search}`,
            port: url.port,
            rejectUnauthorized: false,
        }, (response) => {
            const chunks = [];
            response.on("data", (chunk) => chunks.push(chunk));
            response.on("end", () => resolve(Object.freeze({
                body: Buffer.concat(chunks),
                headers: response.headers,
                status: response.statusCode,
            })));
        });
        operation.once("error", reject);
        operation.end();
    });
}

async function runNegativeServerTests(server, runtimeDirectory) {
    const unhashed = await request(
        server.origin,
        "/RhythmGameWasmProbe.js",
    );
    assert.equal(unhashed.status, 404, "unhashed asset must be rejected");

    const traversal = await request(
        server.origin,
        "/%2e%2e%2fruntime-artifacts.json",
        { rawPath: true },
    );
    assert.ok(
        [400, 404].includes(traversal.status),
        "path traversal must be rejected",
    );

    const missingCoop = await request(
        server.origin,
        "/negative/missing-coop/RhythmGameWasmProbe.html",
    );
    assert.equal(
        missingCoop.headers["cross-origin-opener-policy"],
        undefined,
        "missing-coop must omit its isolation header",
    );
    const missingCoep = await request(
        server.origin,
        "/negative/missing-coep/RhythmGameWasmProbe.html",
    );
    assert.equal(
        missingCoep.headers["cross-origin-embedder-policy"],
        undefined,
        "missing-coep must omit its isolation header",
    );

    await assert.rejects(
        request(server.origin, "/", { plaintext: true }),
        Error,
        "non-HTTPS access must fail",
    );

    const invalidRoot = await mkdtemp(path.join(
        os.tmpdir(),
        "rhythmgame-playtest-missing-role-",
    ));
    try {
        const manifest = JSON.parse(await readFile(
            path.join(runtimeDirectory, "runtime-artifacts.json"),
            "utf8",
        ));
        delete manifest.artifacts.media;
        await writeFile(
            path.join(invalidRoot, "runtime-artifacts.json"),
            `${JSON.stringify(manifest)}\n`,
            "utf8",
        );
        await assert.rejects(
            startProbeServer({ runtimeDirectory: invalidRoot }),
            /missing or extra roles/,
            "missing manifest role must fail before serving",
        );
    } finally {
        await rm(invalidRoot, { force: true, recursive: true });
    }
}

async function readInputSequence(inputSequencePath) {
    const fixture = JSON.parse(await readFile(inputSequencePath, "utf8"));
    if (
        fixture?.schemaVersion !== 1
        || !["native", "lr2"].includes(fixture.preset)
        || !Array.isArray(fixture.events)
        || fixture.events.length === 0
        || fixture.events.length > 256
    ) {
        throw new Error("Invalid canonical input sequence");
    }
    return fixture;
}

function captureDiagnostics(page) {
    const diagnostics = {
        audioWorkletFailures: [],
        browserDisconnected: [],
        browserCrashes: [],
        consoleWarnings: [],
        contextClosed: [],
        failedRequests: [],
        httpFailures: [],
        pageClosed: [],
        qmlErrors: [],
        uncaughtExceptions: [],
    };
    page.on("pageerror", (error) => {
        diagnostics.uncaughtExceptions.push(String(error));
    });
    page.on("console", (message) => {
        if (message.type() === "warning") {
            diagnostics.consoleWarnings.push(message.text());
            return;
        }
        if (message.type() !== "error") {
            return;
        }
        const text = message.text();
        if (/qml|qrc:/i.test(text)) {
            diagnostics.qmlErrors.push(text);
        } else if (/audio.?worklet/i.test(text)) {
            diagnostics.audioWorkletFailures.push(text);
        } else {
            diagnostics.uncaughtExceptions.push(text);
        }
    });
    page.on("crash", () => {
        diagnostics.browserCrashes.push("renderer process crashed");
    });
    page.on("requestfailed", (request_) => {
        diagnostics.failedRequests.push({
            error: request_.failure()?.errorText ?? "unknown failure",
            url: request_.url(),
        });
    });
    page.on("response", (response) => {
        if (response.status() >= 400) {
            diagnostics.httpFailures.push({
                status: response.status(),
                url: response.url(),
            });
        }
    });
    return diagnostics;
}

function captureLifecycleDiagnostics(external, diagnostics) {
    external.browser.on("disconnected", () => {
        diagnostics.browserDisconnected.push("browser disconnected");
    });
    external.context.on("close", () => {
        diagnostics.contextClosed.push("browser context closed");
    });
    external.page.on("close", () => {
        diagnostics.pageClosed.push("page closed");
    });
}

async function snapshot(page) {
    return page.evaluate(
        () => globalThis.__rhythmGameWebPlaytest?.snapshot() ?? null,
    );
}

async function waitForPhase(page, accepted, timeout = 120_000) {
    await page.waitForFunction(
        (phases) => {
            const state = globalThis.__rhythmGameWebPlaytest?.snapshot();
            return state !== undefined
                && (
                    phases.includes(state.phase)
                    || [5, 6, 7].includes(state.phase)
                );
        },
        [...accepted],
        { timeout },
    );
    const state = await snapshot(page);
    if (!accepted.includes(state.phase)) {
        throw new Error(
            `Unexpected terminal phase ${state.phase} while waiting for `
                + accepted.join(","),
        );
    }
    return state;
}

async function clickStart(page) {
    const screen = await page.locator("#screen").boundingBox();
    assert.notEqual(screen, null, "Qt screen must have a browser box");
    await page.mouse.click(
        screen.x + screen.width - 210,
        screen.y + 58,
    );
}

async function injectInputSequence(page, fixture) {
    const started = performance.now();
    for (const event of fixture.events) {
        const remaining = event.atMilliseconds - (performance.now() - started);
        if (remaining > 0) {
            await delay(remaining);
        }
        if (event.action === "press") {
            await page.keyboard.down(event.code);
        } else {
            await page.keyboard.up(event.code);
        }
    }
}

export async function runPlaytest({
    completeForTrace = false,
    inputSequence = defaultInputSequence,
    runtimeDirectory,
    runNegativeTests = false,
}) {
    const fixture = await readInputSequence(inputSequence);
    const server = await startProbeServer({ runtimeDirectory });
    let external = null;
    let diagnostics = null;
    try {
        if (runNegativeTests) {
            await runNegativeServerTests(server, runtimeDirectory);
        }
        external = await launchExternalLifecycleBrowser("chrome-stable");
        await external.page.setViewportSize({
            height: 800,
            width: 1280,
        });
        diagnostics = captureDiagnostics(external.page);
        captureLifecycleDiagnostics(external, diagnostics);
        await external.page.goto(
            `${server.origin}/RhythmGameWasmProbe.html`,
            { waitUntil: "domcontentloaded" },
        );
        const isolated = await external.page.evaluate(
            () => isSecureContext && crossOriginIsolated,
        );
        assert.equal(isolated, true);
        await external.page.evaluate(
            () => globalThis.__rhythmGameWebPlaytest.ready,
        );
        const ready = await waitForPhase(external.page, [2]);
        assert.equal(ready.audioReadyForTrustedResume, true);

        await clickStart(external.page);
        await waitForPhase(external.page, [3, 4]);
        await waitForPhase(external.page, [4]);
        await injectInputSequence(external.page, fixture);
        await external.page.waitForFunction(() => {
            const state = globalThis.__rhythmGameWebPlaytest.snapshot();
            return state.latestJudgement.length > 0 || state.combo > 0;
        });

        let finalState;
        if (completeForTrace) {
            finalState = await waitForPhase(
                external.page,
                [5],
                10 * 60_000,
            );
            await external.page.waitForFunction(
                () => globalThis.__rhythmGameWebPlaytest
                    .takeTraceBytes().byteLength > 0,
            );
        } else {
            await external.page.keyboard.press("Escape");
            await waitForPhase(external.page, [6]);
            await clickStart(external.page);
            await waitForPhase(external.page, [3, 4]);
            await external.page.keyboard.press("Escape");
            finalState = await waitForPhase(
                external.page,
                terminalPhases,
            );
        }
        assert.equal(finalState.droppedInputs, 0);
        assert.equal(finalState.terminalError, "");
        assert.equal(finalState.audioWorkletError, 0);
        assert.deepEqual(diagnostics.uncaughtExceptions, []);
        assert.deepEqual(diagnostics.qmlErrors, []);
        assert.deepEqual(diagnostics.audioWorkletFailures, []);
        assert.deepEqual(diagnostics.browserDisconnected, []);
        assert.deepEqual(diagnostics.browserCrashes, []);
        assert.deepEqual(diagnostics.consoleWarnings, []);
        assert.deepEqual(diagnostics.contextClosed, []);
        assert.deepEqual(diagnostics.failedRequests, []);
        assert.deepEqual(diagnostics.httpFailures, []);
        assert.deepEqual(diagnostics.pageClosed, []);
        const traceBytes = await external.page.evaluate(() => [
            ...globalThis.__rhythmGameWebPlaytest.takeTraceBytes(),
        ]);
        if (completeForTrace) {
            assert.equal(finalState.phase, 5);
            assert.ok(
                traceBytes.length > 0,
                "Finished run must publish native trace bytes",
            );
        }
        return Object.freeze({
            diagnostics: Object.freeze(diagnostics),
            finalState: Object.freeze(finalState),
            traceBytes: Uint8Array.from(traceBytes),
        });
    } catch (error) {
        let lastState = null;
        let traceSummary = null;
        if (external !== null && !external.page.isClosed()) {
            lastState = await snapshot(external.page).catch(() => null);
            const traceBytes = await external.page.evaluate(() => [
                ...globalThis.__rhythmGameWebPlaytest.takeTraceBytes(),
            ]).catch(() => []);
            if (traceBytes.length > 0) {
                const trace = JSON.parse(
                    new TextDecoder().decode(Uint8Array.from(traceBytes)),
                );
                traceSummary = {
                    chartLengthNs: trace.result?.chartLengthNs ?? null,
                    gaugeSamples: trace.gaugeSamples?.length ?? null,
                    inputs: trace.inputs?.length ?? null,
                    judgements: trace.judgements?.length ?? null,
                };
            }
        }
        throw new Error(
            "Web playtest failed with diagnostics: "
                + JSON.stringify({
                    browserConnected:
                        external?.browser.isConnected() ?? false,
                    diagnostics,
                    lastState,
                    traceSummary,
                }),
            { cause: error },
        );
    } finally {
        await external?.close();
        await server.close();
    }
}

const invokedAsScript = process.argv[1]
    && fileURLToPath(import.meta.url) === path.resolve(process.argv[1]);
if (invokedAsScript) {
    const options = parseArguments(process.argv.slice(2));
    await runPlaytest({
        inputSequence: options.inputSequence,
        runNegativeTests: true,
        runtimeDirectory: options.runtimeDirectory,
    });
}
