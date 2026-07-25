import { createHash } from "node:crypto";
import { readFile, realpath } from "node:fs/promises";
import path from "node:path";

export const requiredArtifactRoles = Object.freeze([
    "audioWorklet",
    "bootstrap",
    "css",
    "html",
    "mainJs",
    "media",
    "preflightWorker",
    "qtloader",
    "wasm",
    "wasmWorker",
]);

const roleSet = new Set(requiredArtifactRoles);
const expectedMimes = Object.freeze({
    audioWorklet: "text/javascript; charset=utf-8",
    bootstrap: "text/javascript; charset=utf-8",
    css: "text/css; charset=utf-8",
    html: "text/html; charset=utf-8",
    mainJs: "text/javascript; charset=utf-8",
    media: "video/webm",
    preflightWorker: "text/javascript; charset=utf-8",
    qtloader: "text/javascript; charset=utf-8",
    wasm: "application/wasm",
    wasmWorker: "text/javascript; charset=utf-8",
});
const digestPattern = /^[0-9a-f]{64}$/;
const buildIdPattern = /^[0-9a-f]{64}$/;
const sriPattern = /^sha256-[A-Za-z0-9+/]{43}=$/;
const safeLeafPattern = /^[A-Za-z0-9][A-Za-z0-9._-]*$/;
const artifactNames = Object.freeze({
    audioWorklet: ["RhythmGameWasmProbe.aw", "js"],
    bootstrap: ["bootstrap", "mjs"],
    css: ["probe", "css"],
    mainJs: ["RhythmGameWasmProbe", "js"],
    media: ["probe", "webm"],
    preflightWorker: ["preflight-worker", "mjs"],
    qtloader: ["qtloader", "js"],
    wasm: ["RhythmGameWasmProbe", "wasm"],
    wasmWorker: ["RhythmGameWasmProbe.ww", "js"],
});

export function sha256(bytes) {
    return createHash("sha256").update(bytes).digest("hex");
}

export function sha256Sri(bytes) {
    return `sha256-${createHash("sha256").update(bytes).digest("base64")}`;
}

function assertPlainObject(value, description) {
    if (
        value === null
        || typeof value !== "object"
        || Array.isArray(value)
        || Object.getPrototypeOf(value) !== Object.prototype
    ) {
        throw new Error(`${description} must be a JSON object`);
    }
}

function expectedArtifactUrl(role, digest) {
    if (role === "html") {
        return "RhythmGameWasmProbe.html";
    }
    const [stem, extension] = artifactNames[role];
    return `${stem}.${digest}.${extension}`;
}

export function validateArtifactManifest(manifest) {
    assertPlainObject(manifest, "runtime artifact manifest");
    if (manifest.schemaVersion !== 1) {
        throw new Error("runtime artifact manifest schemaVersion must be 1");
    }
    if (
        typeof manifest.buildId !== "string"
        || !buildIdPattern.test(manifest.buildId)
    ) {
        throw new Error("runtime artifact manifest buildId must be SHA-256");
    }
    assertPlainObject(manifest.artifacts, "runtime artifact roles");
    const roles = Object.keys(manifest.artifacts).sort();
    if (
        roles.length !== requiredArtifactRoles.length
        || roles.some((role, index) => role !== requiredArtifactRoles[index])
    ) {
        const unknown = roles.filter((role) => !roleSet.has(role));
        throw new Error(
            unknown.length
                ? `unknown runtime artifact roles: ${unknown.join(", ")}`
                : "runtime artifact manifest has missing or extra roles",
        );
    }

    const urls = new Set();
    for (const role of requiredArtifactRoles) {
        const artifact = manifest.artifacts[role];
        assertPlainObject(artifact, `artifact ${role}`);
        if (typeof artifact.url !== "string") {
            continue;
        }
        const foldedUrl = artifact.url.toLowerCase();
        if (urls.has(foldedUrl)) {
            throw new Error(`duplicate artifact URL: ${artifact.url}`);
        }
        urls.add(foldedUrl);
    }
    for (const role of requiredArtifactRoles) {
        const artifact = manifest.artifacts[role];
        assertPlainObject(artifact, `artifact ${role}`);
        const expectedKeys = [
            "buildId",
            "bytes",
            "mime",
            "sha256",
            "sri",
            "url",
        ];
        const actualKeys = Object.keys(artifact).sort();
        if (
            actualKeys.length !== expectedKeys.length
            || actualKeys.some((key, index) => key !== expectedKeys[index])
        ) {
            throw new Error(`artifact ${role} has an invalid shape`);
        }
        if (artifact.buildId !== manifest.buildId) {
            throw new Error(`artifact ${role} has a mismatched buildId`);
        }
        if (
            !Number.isSafeInteger(artifact.bytes)
            || artifact.bytes < 0
        ) {
            throw new Error(`artifact ${role} has invalid byte length`);
        }
        if (artifact.mime !== expectedMimes[role]) {
            throw new Error(`artifact ${role} has invalid MIME`);
        }
        if (
            typeof artifact.sha256 !== "string"
            || !digestPattern.test(artifact.sha256)
        ) {
            throw new Error(`artifact ${role} has invalid SHA-256`);
        }
        if (
            typeof artifact.sri !== "string"
            || !sriPattern.test(artifact.sri)
        ) {
            throw new Error(`artifact ${role} has invalid SRI`);
        }
        if (
            typeof artifact.url !== "string"
            || !safeLeafPattern.test(artifact.url)
            || path.isAbsolute(artifact.url)
            || artifact.url === "."
            || artifact.url === ".."
        ) {
            throw new Error(`artifact ${role} has unsafe URL`);
        }
        if (artifact.url !== expectedArtifactUrl(role, artifact.sha256)) {
            throw new Error(`artifact ${role} has invalid URL`);
        }
    }
    return manifest;
}

export async function loadArtifactManifest(runtimeDirectory) {
    const root = await realpath(runtimeDirectory);
    const manifestPath = path.join(root, "runtime-artifacts.json");
    const resolvedManifest = await realpath(manifestPath);
    if (
        resolvedManifest !== manifestPath
        && resolvedManifest.toLowerCase() !== manifestPath.toLowerCase()
    ) {
        throw new Error("runtime artifact manifest may not be a link");
    }
    const bytes = await readFile(resolvedManifest);
    let manifest;
    try {
        manifest = JSON.parse(bytes.toString("utf8"));
    } catch (error) {
        throw new Error(
            `invalid runtime artifact manifest JSON: ${error.message}`,
        );
    }
    validateArtifactManifest(manifest);
    return { bytes, manifest, root };
}
