import assert from "node:assert/strict";
import test from "node:test";
import { fileURLToPath } from "node:url";

import {
    describeBrowser,
    resolveBrowserLane,
} from "../lib/browser-matrix.mjs";

const bannedEffectiveArguments = [
    "--enable-features=SharedArrayBuffer",
    "--enable-features=Other,SharedArrayBuffer",
    "--enable-features=WebAssemblyJSPromiseIntegration",
    (
        "--enable-features=Other," +
        "WebAssemblyJSPromiseIntegration"
    ),
    "--disable-web-security",
    "--disable-web-security=true",
    "--autoplay-policy=no-user-gesture-required",
    "--ignore-certificate-errors",
    "--ignore-certificate-errors=true",
];
const executableFixture = fileURLToPath(import.meta.url);
const missingExecutable = `${executableFixture}.does-not-exist`;

function mockCommandLine(browser, executable, ...arguments_) {
    browser.newBrowserCDPSession = async () => ({
        send: async () => ({
            arguments: [executable, ...arguments_],
        }),
        detach: async () => {},
    });
}

test("browser identity is bound to resolved launch provenance", async (t) => {
    const requestedOptions = { headless: true };
    const browser = await resolveBrowserLane(
        "chromium-cft",
        requestedOptions,
    );
    const realNewBrowserCDPSession = browser.newBrowserCDPSession;
    mockCommandLine(
        browser,
        executableFixture,
        "--enable-automation",
    );
    try {
        await t.test(
            "caller may omit the forced channel",
            async () => {
                const identity = await describeBrowser(
                    browser,
                    requestedOptions,
                );
                assert.equal(identity.lane, "chromium-cft");
                assert.equal(identity.channel, "chromium");
                assert.equal(identity.headless, true);
                assert.equal(identity.profileMode, "temporary");
            },
        );

        await t.test(
            "missing and relabeled caller metadata is rejected",
            async () => {
                await assert.rejects(
                    describeBrowser(browser),
                    /caller launch metadata is required/i,
                );
                mockCommandLine(
                    browser,
                    executableFixture,
                    "--enable-automation",
                );
                for (const metadata of [
                    {},
                    { headless: false },
                    {
                        channel: "chrome",
                        headless: true,
                    },
                    {
                        headless: true,
                        profileMode: "persistent",
                    },
                ]) {
                    await assert.rejects(
                        describeBrowser(browser, metadata),
                        /launch provenance mismatch/i,
                    );
                }
            },
        );

        await t.test(
            "effective CDP arguments reject every acceptance bypass",
            async (effectiveArgumentsTest) => {
                for (const argument of bannedEffectiveArguments) {
                    await effectiveArgumentsTest.test(
                        argument,
                        async () => {
                            mockCommandLine(
                                browser,
                                missingExecutable,
                                argument,
                            );
                            await assert.rejects(
                                describeBrowser(
                                    browser,
                                    requestedOptions,
                                ),
                                /Forbidden browser argument/,
                            );
                        },
                    );
                }
            },
        );
    } finally {
        browser.newBrowserCDPSession = realNewBrowserCDPSession;
        await browser.close();
    }
});
