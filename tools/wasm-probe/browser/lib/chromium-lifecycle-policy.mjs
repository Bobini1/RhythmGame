export const bfcacheIgnoredDefaultArguments = Object.freeze([
    "--disable-back-forward-cache",
]);

export const browserInspectionArguments = Object.freeze([
    "--enable-automation",
]);

export const headedLifecycleIgnoredDefaultArguments = Object.freeze([
    "--disable-background-timer-throttling",
    "--disable-backgrounding-occluded-windows",
    "--disable-back-forward-cache",
    "--disable-renderer-backgrounding",
]);

const blockingProjects = new Set([
    "chromium-cft",
    "chrome-stable",
    "chrome-beta",
]);

export function ignoredLifecycleDefaultArguments(projectName) {
    if (!blockingProjects.has(projectName)) {
        throw new Error(`Unknown blocking browser project: ${projectName}`);
    }
    return projectName === "chromium-cft"
        ? bfcacheIgnoredDefaultArguments
        : headedLifecycleIgnoredDefaultArguments;
}

export function auditLifecycleArguments(effectiveArguments, projectName) {
    if (!Array.isArray(effectiveArguments)) {
        throw new Error("Chromium effective arguments must be an array");
    }
    const requiredAbsent = ignoredLifecycleDefaultArguments(projectName);
    const unexpectedlyPresent = requiredAbsent.filter((argument) =>
        effectiveArguments.includes(argument),
    );
    if (unexpectedlyPresent.length !== 0) {
        throw new Error(
            "Chromium lifecycle behavior is disabled by Playwright defaults: "
            + unexpectedlyPresent.join(", "),
        );
    }
    const missingRequired = browserInspectionArguments.filter((argument) =>
        !effectiveArguments.includes(argument),
    );
    if (missingRequired.length !== 0) {
        throw new Error(
            "Chromium command-line inspection is unavailable without: "
            + missingRequired.join(", "),
        );
    }
    return Object.freeze({
        requiredAbsent: Object.freeze([...requiredAbsent]),
        requiredPresent: browserInspectionArguments,
        verifiedVia: "Browser.getBrowserCommandLine",
    });
}
