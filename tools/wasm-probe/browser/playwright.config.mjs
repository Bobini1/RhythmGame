import { defineConfig } from "@playwright/test";

import {
    bfcacheIgnoredDefaultArguments,
    browserInspectionArguments,
    headedLifecycleIgnoredDefaultArguments,
} from "./lib/chromium-lifecycle-policy.mjs";

export default defineConfig({
    testDir: "./tests",
    testMatch: "**/*.spec.mjs",
    retries: 0,
    workers: 1,
    outputDir: ".traces",
    reporter: [
        ["list"],
        [
            "html",
            {
                outputFolder: "playwright-report",
                open: "never",
            },
        ],
    ],
    projects: [
        {
            name: "chromium-cft",
            grepInvert: /@headed/,
            use: {
                channel: "chromium",
                headless: true,
                launchOptions: {
                    args: browserInspectionArguments,
                    ignoreDefaultArgs: bfcacheIgnoredDefaultArguments,
                },
            },
        },
        {
            name: "chrome-stable",
            use: {
                channel: "chrome",
                headless: false,
                launchOptions: {
                    args: browserInspectionArguments,
                    ignoreDefaultArgs:
                        headedLifecycleIgnoredDefaultArguments,
                },
            },
        },
        {
            name: "chrome-beta",
            use: {
                channel: "chrome-beta",
                headless: false,
                launchOptions: {
                    args: browserInspectionArguments,
                    ignoreDefaultArgs:
                        headedLifecycleIgnoredDefaultArguments,
                },
            },
        },
    ],
});
