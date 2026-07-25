const canonicalResponsePolicy = Object.freeze({
    "Cross-Origin-Opener-Policy": "same-origin",
    "Cross-Origin-Embedder-Policy": "require-corp",
    "Cross-Origin-Resource-Policy": "same-origin",
    "X-Content-Type-Options": "nosniff",
    "Referrer-Policy": "no-referrer",
    "Permissions-Policy": "fullscreen=(self), gamepad=(self), hid=(self)",
});

export const responsePolicy = Object.freeze(
    Object.fromEntries(
        Object.entries(canonicalResponsePolicy).map(([name, value]) => [
            name.toLowerCase(),
            value,
        ]),
    ),
);

export const negativeModes = Object.freeze([
    "missing-coop",
    "missing-coep",
    "wrong-wasm-mime",
    "missing-wasm-unsafe-eval",
    "blocked-worker-src",
    "corrupt-bootstrap",
    "corrupt-main-js",
    "corrupt-wasm",
    "corrupt-qtloader",
]);

const negativeModeSet = new Set(negativeModes);

export function isNegativeMode(value) {
    return negativeModeSet.has(value);
}

export function contentSecurityPolicy(port) {
    if (!/^[1-9][0-9]{0,4}$/.test(String(port))) {
        throw new TypeError(`Invalid CSP port: ${port}`);
    }
    return [
        "default-src 'self'",
        "script-src 'self' 'wasm-unsafe-eval'",
        "worker-src 'self'",
        `connect-src 'self' wss://127.0.0.1:${port}`,
        "img-src 'self' data:",
        "media-src 'self'",
        "font-src 'self'",
        "style-src 'self'",
        "object-src 'none'",
        "base-uri 'none'",
        "frame-ancestors 'none'",
        "form-action 'self'",
        "manifest-src 'self'",
    ].join("; ");
}

export function policyHeaders(port, {
    mode = null,
    route = "/",
} = {}) {
    const headers = { ...responsePolicy };
    if (mode === "missing-coop") {
        delete headers["cross-origin-opener-policy"];
    } else if (mode === "missing-coep") {
        delete headers["cross-origin-embedder-policy"];
    }

    let csp = contentSecurityPolicy(port);
    if (mode === "missing-wasm-unsafe-eval") {
        csp = csp.replace(" 'wasm-unsafe-eval'", "");
    } else if (
        mode === "blocked-worker-src"
        && route === "/RhythmGameWasmProbe.html"
    ) {
        csp = csp.replace("worker-src 'self'", "worker-src 'none'");
    }
    headers["content-security-policy"] = csp;
    return headers;
}
