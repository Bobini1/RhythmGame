import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const expectedNodeVersion = "v20.18.0";
const expectedNpmVersion = "10.8.2";
const browserRoot = path.dirname(fileURLToPath(import.meta.url));
const repositoryRoot = path.resolve(browserRoot, "../../..");
const playwrightBrowsersPath = path.join(
    repositoryRoot,
    ".toolchains",
    "playwright",
);
const npmCli = path.join(
    path.dirname(process.execPath),
    "node_modules",
    "npm",
    "bin",
    "npm-cli.js",
);
const playwrightCli = path.join(
    browserRoot,
    "node_modules",
    "@playwright",
    "test",
    "cli.js",
);
const commands = new Set([
    "npm-lock",
    "npm-ci",
    "install-chromium",
    "install-chrome-beta",
    "node-test",
    "playwright-test",
    "qualify",
    "fsa",
]);
const childEnvironment = {
    ...process.env,
    PLAYWRIGHT_BROWSERS_PATH: playwrightBrowsersPath,
};

function spawnNode(arguments_, options = {}) {
    const result = spawnSync(process.execPath, arguments_, {
        cwd: browserRoot,
        env: childEnvironment,
        shell: false,
        stdio: options.capture ? "pipe" : "inherit",
        encoding: options.capture ? "utf8" : undefined,
    });
    if (result.error) {
        throw result.error;
    }
    return result;
}

function requireSuccess(result, description) {
    if (result.status !== 0) {
        const detail = [result.stdout, result.stderr]
            .filter(Boolean)
            .map((value) => value.trim())
            .filter(Boolean)
            .join("\n");
        throw new Error(
            `${description} failed with exit code ${result.status}` +
                (detail ? `:\n${detail}` : ""),
        );
    }
}

function rejectArguments(command, arguments_) {
    if (arguments_.length !== 0) {
        throw new Error(`${command} does not accept additional arguments`);
    }
}

function verifyRuntime() {
    if (process.version !== expectedNodeVersion) {
        throw new Error(
            `Expected authenticated Node ${expectedNodeVersion}, ` +
                `got ${process.version}`,
        );
    }
    if (!existsSync(npmCli)) {
        throw new Error(`Pinned npm CLI is missing: ${npmCli}`);
    }
    const npmVersion = spawnNode([npmCli, "--version"], { capture: true });
    requireSuccess(npmVersion, "Pinned npm version probe");
    const actualNpmVersion = npmVersion.stdout.trim();
    if (actualNpmVersion !== expectedNpmVersion) {
        throw new Error(
            `Expected authenticated npm ${expectedNpmVersion}, ` +
                `got ${actualNpmVersion}`,
        );
    }
}

async function main() {
    verifyRuntime();
    const [command, ...arguments_] = process.argv.slice(2);
    if (!commands.has(command)) {
        throw new Error(
            `Expected exactly one browser tool subcommand: ` +
                `${[...commands].join(", ")}`,
        );
    }

    if (command === "npm-lock") {
        rejectArguments(command, arguments_);
        if (existsSync(path.join(browserRoot, "node_modules"))) {
            throw new Error(
                "npm-lock refuses to run while node_modules exists",
            );
        }
        return spawnNode([
            npmCli,
            "install",
            "--package-lock-only",
            "--ignore-scripts",
            "--no-audit",
            "--no-fund",
        ]);
    }
    if (command === "npm-ci") {
        rejectArguments(command, arguments_);
        return spawnNode([
            npmCli,
            "ci",
            "--ignore-scripts",
            "--no-audit",
            "--no-fund",
        ]);
    }
    if (command === "install-chromium") {
        rejectArguments(command, arguments_);
        return spawnNode([playwrightCli, "install", "chromium"]);
    }
    if (command === "install-chrome-beta") {
        rejectArguments(command, arguments_);
        const install = spawnNode([
            playwrightCli,
            "install",
            "chrome-beta",
        ]);
        if (install.status !== 0) {
            return install;
        }
        const {
            describeBrowser,
            resolveBrowserLane,
        } = await import("./lib/browser-matrix.mjs");
        const launchOptions = { channel: "chrome-beta", headless: true };
        const browser = await resolveBrowserLane(
            "chrome-beta",
            launchOptions,
        );
        try {
            const identity = await describeBrowser(browser, launchOptions);
            process.stdout.write(`${JSON.stringify(identity)}\n`);
        } finally {
            await browser.close();
        }
        return install;
    }
    if (command === "node-test") {
        return spawnNode(["--test", ...arguments_]);
    }
    if (command === "playwright-test") {
        return spawnNode([playwrightCli, "test", ...arguments_]);
    }
    if (command === "qualify") {
        return spawnNode([
            path.join(browserRoot, "run-qualification.mjs"),
            ...arguments_,
        ]);
    }
    return spawnNode([
        path.join(browserRoot, "run-fsa-qualification.mjs"),
        ...arguments_,
    ]);
}

try {
    const result = await main();
    process.exitCode = result.status ?? 1;
} catch (error) {
    process.stderr.write(
        `${error instanceof Error ? error.message : String(error)}\n`,
    );
    process.exitCode = 1;
}
