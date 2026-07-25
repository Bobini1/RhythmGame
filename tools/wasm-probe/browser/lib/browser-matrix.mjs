import { createHash } from "node:crypto";
import { createReadStream } from "node:fs";
import path from "node:path";
import { chromium } from "@playwright/test";

export const blockingBrowserLanes = [
    "chromium-cft",
    "chrome-stable",
    "chrome-beta",
];

const laneChannels = new Map([
    ["chromium-cft", "chromium"],
    ["chrome-stable", "chrome"],
    ["chrome-beta", "chrome-beta"],
]);
const bannedArguments = [
    "--disable-web-security",
    "--autoplay-policy=no-user-gesture-required",
    "--ignore-certificate-errors",
];
const bannedFeatures = new Set([
    "SharedArrayBuffer",
    "WebAssemblyJSPromiseIntegration",
]);
const blockingHashes = new Map();
const browserProvenance = new WeakMap();
const launchMetadataKeys = [
    "channel",
    "headless",
    "profileMode",
];

function hasOwn(value, key) {
    return Object.prototype.hasOwnProperty.call(value, key);
}

function captureLaunchMetadata(options) {
    return Object.freeze(
        Object.fromEntries(
            launchMetadataKeys.map((key) => [
                key,
                Object.freeze({
                    present: hasOwn(options, key),
                    value: options[key],
                }),
            ]),
        ),
    );
}

function assertMatchingLaunchMetadata(options, expected) {
    if (
        options === null ||
        typeof options !== "object" ||
        Array.isArray(options)
    ) {
        throw new Error("Caller launch metadata is required");
    }
    const actual = captureLaunchMetadata(options);
    for (const key of launchMetadataKeys) {
        if (
            actual[key].present !== expected[key].present ||
            !Object.is(actual[key].value, expected[key].value)
        ) {
            throw new Error(
                `Browser launch provenance mismatch for ${key}`,
            );
        }
    }
}

function normalizedExecutableSuffix(executable) {
    const normalized = path.resolve(executable).replaceAll("\\", "/");
    const folded = normalized.toLocaleLowerCase("en-US");
    for (const marker of [
        "/.toolchains/playwright/",
        "/google/chrome beta/",
        "/google/chrome/",
    ]) {
        const index = folded.indexOf(marker);
        if (index !== -1) {
            return normalized.slice(index + 1);
        }
    }
    return `<machine>/${normalized.split("/").slice(-4).join("/")}`;
}

function normalizedCommandLine(arguments_, executableSuffix) {
    return arguments_.map((argument, index) => {
        if (index === 0) {
            return executableSuffix;
        }
        if (argument.startsWith("--user-data-dir=")) {
            return "--user-data-dir=<profile>";
        }
        return argument;
    });
}

async function sha256File(file) {
    const digest = createHash("sha256");
    for await (const chunk of createReadStream(file)) {
        digest.update(chunk);
    }
    return digest.digest("hex");
}

export function assertNoAcceptanceBypass(launchOptions) {
    if (
        Object.prototype.hasOwnProperty.call(
            launchOptions,
            "bypassCSP",
        )
    ) {
        throw new Error("Playwright bypassCSP must not be set");
    }
    for (const argument of launchOptions.args ?? []) {
        const enabledFeatures = argument.startsWith("--enable-features=")
            ? argument
                  .slice("--enable-features=".length)
                  .split(",")
                  .map((feature) => feature.trim())
            : [];
        const banned = bannedArguments.some(
            (candidate) =>
                argument === candidate ||
                argument.startsWith(`${candidate}=`),
        );
        const bannedFeature = enabledFeatures.find((feature) =>
            bannedFeatures.has(feature),
        );
        if (banned || bannedFeature) {
            throw new Error(`Forbidden browser argument: ${argument}`);
        }
    }
}

export async function resolveBrowserLane(name, options = {}) {
    if (!blockingBrowserLanes.includes(name)) {
        throw new Error(`Unknown blocking browser lane: ${name}`);
    }
    const channel = laneChannels.get(name);
    if (hasOwn(options, "channel") && options.channel !== channel) {
        throw new Error(
            `Browser launch provenance mismatch for channel`,
        );
    }
    if (
        hasOwn(options, "profileMode") &&
        options.profileMode !== "temporary"
    ) {
        throw new Error(
            `Browser launch provenance mismatch for profileMode`,
        );
    }
    const callerMetadata = captureLaunchMetadata(options);
    const arguments_ = [...(options.args ?? [])];
    if (!arguments_.includes("--enable-automation")) {
        arguments_.push("--enable-automation");
    }
    const {
        profileMode: _profileMode,
        ...playwrightOptions
    } = options;
    const launchOptions = {
        ...playwrightOptions,
        args: arguments_,
        channel,
    };
    assertNoAcceptanceBypass(launchOptions);
    try {
        const browser = await chromium.launch(launchOptions);
        browserProvenance.set(
            browser,
            Object.freeze({
                lane: name,
                channel,
                headless: options.headless !== false,
                profileMode: "temporary",
                callerMetadata,
            }),
        );
        return browser;
    } catch (error) {
        throw new Error(
            `Required browser lane ${name} is unavailable after ` +
                `provisioning: ${
                    error instanceof Error ? error.message : String(error)
                }`,
            { cause: error },
        );
    }
}

export async function describeBrowser(browser, launchOptions) {
    const provenance = browserProvenance.get(browser);
    if (!provenance) {
        throw new Error(
            "Browser is missing authoritative launch provenance",
        );
    }
    assertMatchingLaunchMetadata(
        launchOptions,
        provenance.callerMetadata,
    );
    assertNoAcceptanceBypass(launchOptions);
    const session = await browser.newBrowserCDPSession();
    let commandLine;
    try {
        commandLine = await session.send("Browser.getBrowserCommandLine");
    } finally {
        await session.detach();
    }
    if (
        !Array.isArray(commandLine.arguments) ||
        commandLine.arguments.length === 0
    ) {
        throw new Error("Chromium did not expose its effective command line");
    }
    assertNoAcceptanceBypass({
        args: commandLine.arguments.slice(1),
    });

    const executable = commandLine.arguments[0];
    const executableSuffix = normalizedExecutableSuffix(executable);
    const executableSha256 = await sha256File(executable);
    const existingLane = blockingHashes.get(executableSha256);
    if (existingLane && existingLane !== provenance.lane) {
        throw new Error(
            `Blocking browser lanes ${existingLane} and ` +
                `${provenance.lane} ` +
                `resolved to the same executable SHA-256`,
        );
    }
    blockingHashes.set(executableSha256, provenance.lane);

    return {
        lane: provenance.lane,
        channel: provenance.channel,
        executableSuffix,
        executableSha256,
        version: browser.version(),
        arguments: normalizedCommandLine(
            commandLine.arguments,
            executableSuffix,
        ),
        headless: provenance.headless,
        profileMode: provenance.profileMode,
    };
}
