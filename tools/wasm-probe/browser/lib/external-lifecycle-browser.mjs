import { spawn } from "node:child_process";
import { createHash } from "node:crypto";
import {
    mkdir,
    mkdtemp,
    readFile,
    rm,
} from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { chromium } from "@playwright/test";

import {
    auditLifecycleArguments,
    browserInspectionArguments,
} from "./chromium-lifecycle-policy.mjs";
import {
    assertNoAcceptanceBypass,
    describeBrowser,
    resolveBrowserLane,
} from "./browser-matrix.mjs";

const browserDirectory = path.dirname(
    fileURLToPath(new URL("../package.json", import.meta.url)),
);
const profileRoot = path.resolve(browserDirectory, ".profiles");
const playwrightCoreBundlePath = path.join(
    browserDirectory,
    "node_modules",
    "playwright-core",
    "lib",
    "coreBundle.js",
);
const playwrightCoreBundleSha256 =
    "3258d1cf334c6afc95f22aa9c292436cb976b391e0437f1359c83b84f0cb9d66";
const headedProjects = new Set(["chrome-stable", "chrome-beta"]);
const startupTimeoutMilliseconds = 15_000;
const shutdownTimeoutMilliseconds = 5_000;

function delay(milliseconds) {
    return new Promise((resolve) => {
        setTimeout(resolve, milliseconds);
    });
}

async function verifyPinnedNoDefaultsContract() {
    const bytes = await readFile(playwrightCoreBundlePath);
    const digest = createHash("sha256").update(bytes).digest("hex");
    if (digest !== playwrightCoreBundleSha256) {
        throw new Error(
            `Pinned Playwright core bundle digest changed: ${digest}`,
        );
    }
    const source = bytes.toString("utf8");
    for (const marker of [
        "noDefaults: tOptional(tBoolean)",
        "const skipDefaultOverrides = "
            + "this._crPage._browserContext._browser.options.noDefaults",
        "Emulation.setFocusEmulationEnabled",
        "noDefaults: params2.noDefaults",
    ]) {
        if (!source.includes(marker)) {
            throw new Error(
                `Pinned Playwright noDefaults contract changed: ${marker}`,
            );
        }
    }
}

function assertOwnedProfile(profileDirectory) {
    const relative = path.relative(profileRoot, profileDirectory);
    if (
        relative.length === 0
        || relative.startsWith("..")
        || path.isAbsolute(relative)
    ) {
        throw new Error("External lifecycle profile escaped its owned root");
    }
}

async function waitForDevToolsPort(child, profileDirectory) {
    const portFile = path.join(profileDirectory, "DevToolsActivePort");
    const deadline = Date.now() + startupTimeoutMilliseconds;
    while (Date.now() < deadline) {
        if (child.spawnError) {
            throw child.spawnError;
        }
        if (child.exitCode !== null) {
            throw new Error(
                `External Chrome exited during startup: ${child.exitCode}`,
            );
        }
        try {
            const [encodedPort] = (
                await readFile(portFile, "utf8")
            ).trim().split(/\r?\n/);
            const port = Number(encodedPort);
            if (Number.isSafeInteger(port) && port > 0 && port <= 65_535) {
                return port;
            }
        } catch (error) {
            if (error?.code !== "ENOENT") {
                throw error;
            }
        }
        await delay(100);
    }
    throw new Error("External Chrome DevTools endpoint timed out");
}

async function waitForChildExit(child, timeoutMilliseconds) {
    if (child.exitCode !== null) {
        return true;
    }
    return Promise.race([
        new Promise((resolve) => {
            child.once("exit", () => resolve(true));
        }),
        delay(timeoutMilliseconds).then(() => false),
    ]);
}

async function resolveChannelExecutable(projectName) {
    const launchOptions = { headless: true };
    const resolver = await resolveBrowserLane(projectName, launchOptions);
    try {
        const session = await resolver.newBrowserCDPSession();
        let commandLine;
        try {
            commandLine = await session.send(
                "Browser.getBrowserCommandLine",
            );
        } finally {
            await session.detach();
        }
        if (
            !Array.isArray(commandLine.arguments)
            || commandLine.arguments.length === 0
        ) {
            throw new Error(
                "Channel resolver did not expose its executable",
            );
        }
        const identity = await describeBrowser(resolver, launchOptions);
        return Object.freeze({
            executable: commandLine.arguments[0],
            identity: Object.freeze(identity),
        });
    } finally {
        await resolver.close();
    }
}

export async function launchExternalLifecycleBrowser(projectName) {
    if (!headedProjects.has(projectName)) {
        throw new Error(
            `External lifecycle browser is not valid for ${projectName}`,
        );
    }
    await verifyPinnedNoDefaultsContract();
    const resolved = await resolveChannelExecutable(projectName);
    await mkdir(profileRoot, { recursive: true });
    const profileDirectory = await mkdtemp(
        path.join(profileRoot, `${projectName}-`),
    );
    assertOwnedProfile(profileDirectory);

    const launchArguments = [
        `--user-data-dir=${profileDirectory}`,
        "--remote-debugging-address=127.0.0.1",
        "--remote-debugging-port=0",
        ...browserInspectionArguments,
        "--no-first-run",
        "--no-default-browser-check",
        "about:blank",
    ];
    const child = spawn(resolved.executable, launchArguments, {
        shell: false,
        stdio: "ignore",
        windowsHide: false,
    });
    child.spawnError = null;
    child.once("error", (error) => {
        child.spawnError = error;
    });

    let browser = null;
    let certificateSession = null;
    let closed = false;
    const cleanup = async () => {
        if (closed) {
            return;
        }
        closed = true;
        if (certificateSession !== null) {
            await certificateSession.detach().catch(() => {});
        }
        if (browser !== null) {
            await browser.close().catch(() => {});
        }
        if (!await waitForChildExit(child, shutdownTimeoutMilliseconds)) {
            child.kill();
            await waitForChildExit(child, shutdownTimeoutMilliseconds);
        }
        assertOwnedProfile(profileDirectory);
        await rm(profileDirectory, {
            force: true,
            maxRetries: 5,
            recursive: true,
            retryDelay: 100,
        });
    };

    try {
        const port = await waitForDevToolsPort(child, profileDirectory);
        browser = await chromium.connectOverCDP(
            `http://127.0.0.1:${port}`,
            {
                noDefaults: true,
            },
        );
        const contexts = browser.contexts();
        if (contexts.length !== 1) {
            throw new Error(
                `External Chrome exposed ${contexts.length} contexts`,
            );
        }
        const context = contexts[0];
        const page = context.pages()[0] ?? await context.newPage();

        certificateSession = await context.newCDPSession(page);
        await certificateSession.send(
            "Security.setIgnoreCertificateErrors",
            { ignore: true },
        );
        const session = await browser.newBrowserCDPSession();
        let commandLine;
        try {
            commandLine = await session.send(
                "Browser.getBrowserCommandLine",
            );
        } finally {
            await session.detach();
        }
        if (
            !Array.isArray(commandLine.arguments)
            || commandLine.arguments[0] !== resolved.executable
        ) {
            throw new Error(
                "External Chrome executable changed after resolution",
            );
        }
        const effectiveArguments = commandLine.arguments.slice(1);
        assertNoAcceptanceBypass({ args: effectiveArguments });
        const lifecycleArgumentAudit = auditLifecycleArguments(
            effectiveArguments,
            projectName,
        );
        for (const required of [
            `--user-data-dir=${profileDirectory}`,
            "--remote-debugging-address=127.0.0.1",
            "--remote-debugging-port=0",
            ...browserInspectionArguments,
        ]) {
            if (!effectiveArguments.includes(required)) {
                throw new Error(
                    `External Chrome omitted required argument: ${required}`,
                );
            }
        }
        if (effectiveArguments.includes("--remote-debugging-pipe")) {
            throw new Error(
                "External lifecycle Chrome retained Playwright's pipe",
            );
        }

        return Object.freeze({
            browser,
            close: cleanup,
            context,
            identity: resolved.identity,
            lifecycleArgumentAudit,
            page,
            profileMode: "temporary-external-cdp",
        });
    } catch (error) {
        await cleanup();
        throw error;
    }
}
