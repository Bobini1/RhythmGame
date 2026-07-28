import process from "node:process";
import path from "node:path";
import { fileURLToPath } from "node:url";

import {
    launchExternalLifecycleBrowser,
} from "../../wasm-probe/browser/lib/external-lifecycle-browser.mjs";
import {
    startProbeServer,
} from "../../wasm-probe/browser/server/probe-server.mjs";

function parseArguments(arguments_) {
    const options = { browser: "chrome-stable" };
    for (let index = 0; index < arguments_.length; index += 2) {
        const name = arguments_[index];
        const value = arguments_[index + 1];
        if (
            !["--browser", "--runtime-directory"].includes(name)
            || !value
            || value.startsWith("--")
        ) {
            throw new Error(`Invalid manual-launch argument: ${name}`);
        }
        if (name === "--browser") {
            if (!["chrome-stable", "chrome-beta"].includes(value)) {
                throw new Error(`Unsupported browser lane: ${value}`);
            }
            options.browser = value;
        } else {
            options.runtimeDirectory = path.resolve(value);
        }
    }
    if (!options.runtimeDirectory) {
        throw new Error("--runtime-directory is required");
    }
    return Object.freeze(options);
}

async function waitForShutdownSignal() {
    await new Promise((resolve) => {
        process.once("SIGINT", resolve);
        process.once("SIGTERM", resolve);
    });
}

function attachDiagnostics(page) {
    page.on("console", (message) => {
        const type = message.type();
        if (type === "warning" || type === "error") {
            process.stderr.write(
                `[browser console ${type}] ${message.text()}\n`,
            );
        }
    });
    page.on("pageerror", (error) => {
        process.stderr.write(`[browser pageerror] ${String(error)}\n`);
    });
    page.on("crash", () => {
        process.stderr.write("[browser crash] renderer process crashed\n");
    });
    page.on("requestfailed", (request) => {
        process.stderr.write(
            `[browser requestfailed] ${request.url()} `
            + `${request.failure()?.errorText ?? "unknown failure"}\n`,
        );
    });
}

export async function launchManualPlaytest(options) {
    const server = await startProbeServer({
        runtimeDirectory: options.runtimeDirectory,
    });
    let external = null;
    try {
        external = await launchExternalLifecycleBrowser(options.browser);
        attachDiagnostics(external.page);
        await external.page.setViewportSize({
            height: 800,
            width: 1280,
        });
        const url = `${server.origin}/RhythmGameWasmProbe.html`;
        process.stdout.write(`Strict web playtest URL: ${url}\n`);
        await external.page.goto(url, { waitUntil: "domcontentloaded" });
        await waitForShutdownSignal();
    } finally {
        await external?.close();
        await server.close();
    }
}

const invokedAsScript = process.argv[1]
    && fileURLToPath(import.meta.url) === path.resolve(process.argv[1]);
if (invokedAsScript) {
    await launchManualPlaytest(parseArguments(process.argv.slice(2)));
}
