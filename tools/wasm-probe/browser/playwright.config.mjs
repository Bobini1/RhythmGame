import { defineConfig } from "@playwright/test";

export default defineConfig({
    testDir: "./tests",
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
            use: {
                channel: "chromium",
                headless: true,
            },
        },
        {
            name: "chrome-stable",
            use: {
                channel: "chrome",
                headless: false,
            },
        },
        {
            name: "chrome-beta",
            use: {
                channel: "chrome-beta",
                headless: false,
            },
        },
    ],
});
