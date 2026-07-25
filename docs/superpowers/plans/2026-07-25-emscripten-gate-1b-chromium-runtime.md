# Emscripten Gate 1B Chromium Runtime Qualification Plan

> **For agentic workers:** REQUIRED SUB-SKILL: use
> `superpowers:subagent-driven-development` to execute this plan one task at a
> time. Use `superpowers:verification-before-completion` before any PASS claim.
> Task 3 also requires the repository `qt-qml` skill. Check boxes are the
> execution ledger; do not skip a red test, review, or commit checkpoint.

**Goal:** Prove or reject the already-qualified Qt 6.11.1/Emscripten 4.0.7
combination in real Chromium for QML/QSB/WebGL2, native exceptions, pthreads,
QtConcurrent, JSPI, QNAM, WSS, Qt Multimedia, generated AudioWorklet and Wasm
Worker paths, shared-memory growth, teardown, OPFS, and File System Access.

**Architecture:** Extend only the isolated `tools/wasm-probe` application. A
repository-owned strict-CSP shell loads a content-addressed Qt/Wasm bundle from
one local HTTPS/WSS origin. Focused C++ probe objects publish an append-only,
schema-versioned event stream to a browser bridge. Playwright 1.62.0 runs
positive and fail-closed negative lanes in regular Chrome for Testing,
installed branded Chrome Stable, and provisioned branded Chrome Beta. One
trusted browser click starts media and one AudioContext; 1,000 sequential
node/Wasm-Worker cycles run inside that single context, followed by an owned
awaited `AudioContext.close()`. OPFS is automated; the native
`showDirectoryPicker()` flow has a separate headed OS-assisted evidence lane.

**Tech Stack:** Qt 6.11.1, Emscripten 4.0.7, CMake 4.2.3/Ninja 1.13.2,
authenticated emsdk Node 20.18.0/npm 10.8.2, Playwright 1.62.0, regular Chrome
for Testing 151.0.7922.34, installed Chrome Stable, provisioned Chrome Beta,
Node `https` plus `ws` 8.21.1, `pngjs` 7.0.0, and `selfsigned` 5.5.0.

## Authority and scope

Gate 1A is a historical technical PASS at commit
`94fdcd315b02f909f7e5a4b6fa5989d5aa3cee28`. Its evidence remains immutable:

- path: `docs/superpowers/evidence/emscripten-gate-1a.json`
- bytes: `88,931`
- SHA-256:
  `e350f90cd0edb83fa587012bc2c26a97821d211f4611bc3c263277ae94e49d87`

Gate 1B may prove a technical browser combination, but it cannot satisfy Gate
0, authorize formal Gate 1 entry, adapt production startup, or select a Qt
license. The Gate 1B evidence must always contain these independent authority
fields:

```json
{
  "gate1bTechnicalPassed": false,
  "gate0Satisfied": false,
  "formalGate1EntryAuthorized": false,
  "gate1Passed": false,
  "productionPortAuthorized": false
}
```

Only `gate1bTechnicalPassed` may become `true`, and only after every blocking
technical and headed evidence item in this plan passes. The other four values
remain `false` until the user/legal owner separately records:

1. commercial Qt or a GPLv3-compliant distribution route;
2. the ranked-client threat model;
3. native-baseline disposition;
4. hermetic Linux builder and production-trusted HTTPS evidence;
5. target SPDX/SBOM and source/license distribution closure.

Allowed changes:

- `tools/wasm-probe/**`;
- scoped ignore entries in the repository `.gitignore`;
- this Gate 1B plan, its progress ledger, and Gate 1B evidence/result docs.
- only an as-built Gate 1B result section in
  `docs/superpowers/specs/2026-07-23-emscripten-web-port-design.md`.

Any architectural/spec requirement change outside that result section still
requires explicit user approval.

Forbidden changes:

- `src/**`;
- `RhythmGameQml/**`;
- the native root `vcpkg.json` dependency graph;
- production deployment, service worker, authentication, database, scanner, or
  gameplay code.

If a required browser combination fails, record a technical Gate 1B FAIL and
the exact failing combination. Switching the probe-side approach or adding an
owned adapter is allowed. Weakening CSP, enabling blocking on the main thread,
using legacy JS exceptions, using ScriptProcessor, bypassing user activation,
or silently reducing a required cycle is not.

## Frozen qualification semantics

### Browser lanes

| Lane | Blocking | Mode | Claim |
|---|---:|---|---|
| `chromium-cft` | yes | Playwright `channel: "chromium"`; regular CfT, new headless for fast tests and headed for the full stress | Reproducible Chromium/Wasm functional result |
| `chrome-stable` | yes | Playwright `channel: "chrome"`, headed | Branded-Chrome GPU, audio, video, native browser integration result |
| `chrome-beta` | yes | Playwright `channel: "chrome-beta"`, headed; provision through the pinned Playwright CLI when absent | Branded upcoming-Chrome compatibility result |

The runner records Playwright version, channel, executable path normalized to a
machine-independent suffix, executable SHA-256, `browser.version()`, full launch
arguments, headless/headed state, and profile mode. All three blocking lanes
must resolve to distinct executable hashes. There are no retries. Because the
branded Beta channel is mutable rather than Playwright-content-pinned, evidence
binds its exact executable hash/version and becomes stale when that installation
updates.

“Full launch arguments” means the effective browser command line obtained from
CDP `Browser.getBrowserCommandLine`, with the temporary profile path redacted
to a stable token. Playwright's user-supplied `launchOptions.args` alone is not
represented as the effective command line.

Do not pass any of:

```text
--enable-features=SharedArrayBuffer
--enable-features=WebAssemblyJSPromiseIntegration
--disable-web-security
--autoplay-policy=no-user-gesture-required
```

Do not set Playwright `bypassCSP`. A real `page.mouse.click()` at the fixed Qt
Quick button coordinates supplies transient user activation.

Playwright 1.62.0 is the 2026-07-24 release and pins regular Chromium
151.0.7922.34. Use the authenticated emsdk Node/npm, `npm ci`, the checked-in
lockfile, and a worktree-local ignored Playwright browser root.

References:

- <https://github.com/microsoft/playwright/releases/tag/v1.62.0>
- <https://playwright.dev/docs/browsers>
- <https://playwright.dev/docs/api/class-filechooser>

### Origin, transport, and policy

Use one IPv4 loopback HTTPS/WSS origin. It serves the document, Qt loader,
JavaScript, Wasm, QML-backed application, generated `.aw.js`, generated
`.ww.js`, media, QNAM endpoint, artifact manifest, and WebSocket upgrade.

The local server generates an ephemeral test-only certificate. The automated
runner may use Playwright `ignoreHTTPSErrors` only if it records
`certificateTrustValidated: false`; that lane proves the runtime and response
policy, not Gate 0 production certificate trust. No blanket browser
certificate-error command-line flag is allowed. A caller may provide a
locally trusted certificate/key pair, in which case the runner performs a
second request without ignore mode and records
`certificateTrustValidated: true`.

Every first-party HTTP response emits:

```text
Cross-Origin-Opener-Policy: same-origin
Cross-Origin-Embedder-Policy: require-corp
Cross-Origin-Resource-Policy: same-origin
X-Content-Type-Options: nosniff
Referrer-Policy: no-referrer
Permissions-Policy: fullscreen=(self), gamepad=(self), hid=(self)
```

The custom shell contains no inline script, inline style, or event-handler
attribute. Its exact CSP is:

```text
default-src 'self';
script-src 'self' 'wasm-unsafe-eval';
worker-src 'self';
connect-src 'self' wss://127.0.0.1:<server-port>;
img-src 'self' data:;
media-src 'self';
font-src 'self';
style-src 'self';
object-src 'none';
base-uri 'none';
frame-ancestors 'none';
form-action 'self';
manifest-src 'self'
```

`'unsafe-eval'`, `'unsafe-inline'`, and `blob:` worker fallback are forbidden.
The server independently records status, MIME, policy headers, bytes, and
SHA-256 for each request. `application/wasm` is mandatory for `.wasm`;
JavaScript and worker files use a JavaScript MIME; the video uses `video/webm`.

The loopback server is a technical test fixture. It does not satisfy the Gate 0
production-trusted HTTPS deliverable or authorize deployment to
`play.rhythmgame.eu`.

References:

- <https://www.w3.org/TR/secure-contexts/#is-origin-trustworthy>
- <https://doc.qt.io/qt-6/wasm.html#web-server-setup>
- <https://developer.chrome.com/blog/enabling-shared-array-buffer/>
- <https://www.w3.org/TR/CSP3/>

### Runtime report

The bootstrap owns:

```js
globalThis.__rhythmGameGate1b = {
  schemaVersion: 1,
  instance: null,
  events: [],
  snapshot: null,
  ready: Promise,
  command(name, payload) {}
};
```

The bootstrap retains the object returned by `qtLoad()`. `onLoaded` means only
that the Qt loader completed. The application resolves `ready` only after:

1. C++ set the pre-return marker;
2. `main()` returned;
3. a zero-delay `post-main-tick` scheduled immediately before return actually
   fired;
4. the QML root exists;
5. the Qt scene graph rendered two post-return frames;
6. the browser/C++ command bridge responds.

Each event has exactly:

```json
{
  "sequence": 0,
  "monotonicMicroseconds": 0,
  "type": "lower-kebab-case",
  "payload": {}
}
```

Sequence numbers start at zero and are contiguous. Unknown fields fail schema
validation. A terminal snapshot includes `phase`, `checks`, `capabilities`,
`failures`, `cycleSummary`, and the authority object. No machine-specific
absolute path, profile path, certificate private key, environment dump, or
selected user directory path enters committed evidence.

### The 1,000-cycle definition

This plan deliberately does not create 1,000 AudioContexts. Emscripten 4.0.7
requires one Wasm AudioWorklet thread per AudioContext, created once, and its
public destroy function only suspends and drops a handle.

One uninterrupted blocking-browser run is:

1. one genuine browser click;
2. one `AudioContext` created/resumed synchronously from that activation;
3. one generated-and-version-stamped `.aw.js` worklet scope;
4. one preallocated AudioWorklet stack;
5. four Qt pthread-pool workers kept alive;
6. exactly 1,000 strictly sequential cycle IDs `0..999`;
7. for each cycle:
   - create one AudioWorklet node;
   - connect it and observe a non-silent callback/checksum;
   - create one Wasm Worker with its own preallocated stack/TLS region;
   - send and verify the cycle nonce;
   - destroy the node and reject late callbacks;
   - terminate the Wasm Worker;
   - observe the dedicated-worker count return to the four-pthread baseline;
8. at cycle 500, grow shared memory while the node is actively rendering;
9. after cycle 999, disconnect everything and use an owned JS-library adapter
   to `await AudioContext.close()`;
10. require browser state `closed`, no remaining probe node, and the original
    worker baseline.

The raw per-browser NDJSON trace contains, for every cycle:

```text
cycle-start
worker-created
worker-nonce-processed
audio-callback
audio-checksum
node-destroyed
worker-terminated
resource-baseline-restored
```

There is no retry, skipped ID, duplicate ID, timeout recovery, or late callback.
The evidence stores the complete traces and an independent SHA-256 hash chain.

At memory growth:

- record old/new page and byte counts;
- require new bytes greater than old bytes and at most 512 MiB;
- construct an aligned sentinel and a second compact SPSC ring above the old
  byte boundary;
- round-trip the sentinel through main Wasm, a QtConcurrent pthread, a
  preallocated-stack Wasm Worker, and the AudioWorklet C callback;
- keep the pre-growth ring advancing;
- observe callbacks before and after growth;
- require memory to plateau after the designated growth.

If the pinned generated worklet cannot satisfy this, Gate 1B fails unless an
owned, source-bound postprocessor/adapter fixes the generated bootstrap without
weakening the combination.

References:

- <https://github.com/emscripten-core/emscripten/blob/4.0.7/system/include/emscripten/webaudio.h>
- <https://github.com/emscripten-core/emscripten/blob/4.0.7/src/lib/libwebaudio.js>
- <https://github.com/emscripten-core/emscripten/blob/4.0.7/src/audio_worklet.js>
- <https://emscripten.org/docs/api_reference/wasm_audio_worklets.html>
- <https://emscripten.org/docs/tools_reference/settings_reference.html#allow-memory-growth>

### Storage evidence classes

Automated OPFS qualification:

- page write/read;
- dedicated-worker synchronous access write/read/flush/close;
- reload persistence;
- persistent-profile process-restart persistence;
- cleanup;
- explicit failure propagation.

Automated File System Access coverage is capability detection plus deterministic
fake-adapter state-machine tests. It is not chooser evidence.

Blocking headed evidence uses Chrome Stable, a genuine Qt Quick button click,
`showDirectoryPicker({mode: "readwrite"})`, and the native directory chooser.
The selected directory is a generated, non-sensitive canary directory under an
ignored Gate 1B run root. The record contains browser version, origin, user
activation state, before/reload permission state, relative canary manifest,
read/write/enumeration result, timestamp, operator class (`os-ui-assisted` or
`human`), and screenshot. It never records the absolute selected path. Capture
the raw OS screenshot only under the ignored run root; create the committed
evidence image by redacting/cropping only machine-specific parent paths or
unrelated desktop content while preserving browser/native-chooser chrome and
the generated canary leaf. Record the sanitized image hash and redaction
rectangles; do not commit the raw capture.

If the native chooser is cancelled or cannot be driven, the File System Access
subgate and therefore `gate1bTechnicalPassed` remain false. An `<input
type=file>`, Playwright `FileChooser`, fake handle, or CDP diagnostic is not a
substitute.

References:

- <https://developer.chrome.com/docs/capabilities/web-apis/file-system-access>
- <https://chromedevtools.github.io/devtools-protocol/tot/Browser/#type-PermissionType>

## Task 1: Lock the browser harness and fast Gate 1B contract

**Files:**

- Create: `tools/wasm-probe/browser/package.json`
- Create: `tools/wasm-probe/browser/package-lock.json`
- Create: `tools/wasm-probe/browser/run-browser-tool.mjs`
- Create: `tools/wasm-probe/browser/lib/browser-matrix.mjs`
- Create: `tools/wasm-probe/browser/playwright.config.mjs`
- Create: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/scripts/Invoke-WithToolchains.ps1`
- Modify: `tools/wasm-probe/tests/test_toolchain_scripts.py`
- Modify: `.gitignore`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`
- Create: `.superpowers/sdd/gate1b-progress.md` (ignored execution ledger)

- [ ] **Step 1: Add failing fast contract tests**

The tests must initially fail for missing browser files. They assert:

- exact package pins:
  `@playwright/test=1.62.0`, `ws=8.21.1`, `pngjs=7.0.0`,
  `selfsigned=5.5.0`;
- `package-lock.json` lockfile v3 and exact direct versions;
- zero Playwright retries and one worker;
- regular Chromium via `channel: "chromium"`;
- blocking project names `chromium-cft`, `chrome-stable`, and `chrome-beta`;
- no banned browser flags and no `bypassCSP`;
- browser cache, profiles, traces, reports, node modules, certificates, and
  native-chooser run directories are ignored but the lockfile is not;
- every new tracked control/source file is present exactly once in the
  case-insensitively sorted manifests.

Run:

```powershell
python -m unittest tools/wasm-probe/tests/test_gate1b_source_contract.py -v
```

Expected: FAIL because the browser harness does not exist.

- [ ] **Step 2: Implement the authenticated Node/npm dispatcher**

`run-browser-tool.mjs` accepts only these subcommands:

```text
npm-lock
npm-ci
install-chromium
install-chrome-beta
node-test
playwright-test
qualify
fsa
```

It derives npm's CLI from `path.dirname(process.execPath)`, refuses a Node
version other than `v20.18.0`, refuses an npm version other than `10.8.2`,
spawns child Node processes with argument arrays, and sets
`PLAYWRIGHT_BROWSERS_PATH` to the ignored worktree `.toolchains` descendant.
It does not invoke a shell.

Extend `Invoke-WithToolchains.ps1` so the exact requests `node` and `node.exe`
resolve to the already-authenticated `$node`, just as `python` resolves to the
pinned Python. Add focused PowerShell contract tests for that alias; do not add
the ambient Node directory to `PATH`.

Generate the lockfile once using `npm-lock`, then install only with `npm-ci`:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs npm-lock

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs npm-ci
```

`npm-lock` is the only path that may invoke `npm install --package-lock-only`;
it refuses to run when `node_modules` exists. Normal runs use only `npm ci`.

- [ ] **Step 3: Implement browser identity capture**

`browser-matrix.mjs` exports:

```js
export const blockingBrowserLanes =
    ["chromium-cft", "chrome-stable", "chrome-beta"];
export async function resolveBrowserLane(name, options);
export async function describeBrowser(browser, launchOptions);
export function assertNoAcceptanceBypass(launchOptions);
```

`describeBrowser` computes the executable hash in Node, records the normalized
suffix/version/channel/arguments, and rejects duplicate hashes across blocking
lanes. Missing Stable or Beta Chrome is a hard failure after the explicit
provisioning step.

- [ ] **Step 4: Provision the pinned and Beta browsers**

After `npm-ci`, use only the dispatcher:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs install-chromium

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs install-chrome-beta
```

The first command must populate only the ignored pinned Playwright browser
root. The Beta command must go through the pinned Playwright CLI and then
resolve the branded `chrome-beta` channel. Do not download or replace Stable;
only hash/qualify the installed Stable executable. Record no browser PASS yet.

- [ ] **Step 5: Make all focused tests green**

Run:

```powershell
python -m unittest `
  tools/wasm-probe/tests/test_gate1b_source_contract.py `
  tools/wasm-probe/tests/test_toolchain_scripts.py -v
```

Expected: PASS.

- [ ] **Step 6: Review and commit**

Review scope, package integrity, shell avoidance, manifest sorting, ignored
outputs, and absence of ambient Node trust. Record the task result in
`.superpowers/sdd/gate1b-progress.md`.

Commit:

```text
test: lock Gate 1B browser harness
```

## Task 2: Build the strict-CSP HTTPS/WSS shell and artifact protocol

**Files:**

- Create: `tools/wasm-probe/browser/server/probe-server.mjs`
- Create: `tools/wasm-probe/browser/server/policy.mjs`
- Create: `tools/wasm-probe/browser/server/artifact-manifest.mjs`
- Create: `tools/wasm-probe/browser/server/probe-server.test.mjs`
- Create: `tools/wasm-probe/browser/web/RhythmGameWasmProbe.html.in`
- Create: `tools/wasm-probe/browser/web/probe.css`
- Create: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Create: `tools/wasm-probe/browser/web/preflight-worker.mjs`
- Create: `tools/wasm-probe/browser/fixtures/probe.webm`
- Create: `tools/wasm-probe/browser/fixtures/README.md`
- Create: `tools/wasm-probe/scripts/package_runtime_artifacts.py`
- Create: `tools/wasm-probe/tests/test_package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`

- [ ] **Step 1: Add failing policy/server/artifact tests**

Use Node's built-in test runner. Test:

- one random loopback port serves HTTPS and upgrades WSS on the same port;
- the exact response policy and CSP above occur on document, JS, Wasm,
  workers, media, API, 404, redirect, and error responses;
- exact MIME types and `nosniff`;
- path traversal, encoded traversal, duplicate separators, unknown files, and
  symlink/reparse escapes fail closed;
- WSS rejects a wrong `Origin`;
- `/probe/qnam?nonce=...` returns exact JSON and echoes the nonce;
- `/probe/ws` performs text echo, binary echo, server message, heartbeat, and
  clean close;
- request logs contain method, normalized route, status, bytes, response
  SHA-256, and policy/MIME fields but no absolute path;
- negative modes `missing-coop`, `missing-coep`, `wrong-wasm-mime`,
  `missing-wasm-unsafe-eval`, `blocked-worker-src`, `corrupt-bootstrap`,
  `corrupt-main-js`, `corrupt-wasm`, and `corrupt-qtloader` alter only the
  named contract;
- positive CSP permits Wasm compilation but blocks `eval()` and
  `new Function()`; generic `'unsafe-eval'`, `'unsafe-inline'`, and `blob:`
  worker permission are structurally absent;
- the artifact manifest covers HTML, CSS, bootstrap, Qt loader, main JS, Wasm,
  preflight worker, `.aw.js`, `.ww.js`, and media with
  bytes/SHA-256/SRI/build ID.

Run:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test
```

Expected: FAIL because the server and shell do not exist.

- [ ] **Step 2: Implement the server without framework fallbacks**

Use Node `https.createServer`, `ws.WebSocketServer({noServer: true})`, and
`selfsigned` only for an ephemeral test certificate. Accept optional
`--certificate`/`--private-key`; never log key bytes. Bind explicitly to
`127.0.0.1`, choose a random port, and return the origin to the parent over a
structured IPC message. Keep the server and exact scheme/host/port alive for
all reload and process-restart storage checks in one qualification run.

The server has no SPA fallback. Unknown routes are 404 with the same isolation
headers. Range requests for the WebM fixture return correct 206/416 responses
and policy headers. Cache control is `no-store` for the shell/manifest/API and
`public, max-age=31536000, immutable` only for content-addressed artifacts.
The normal server validates the digest in each artifact URL against current
bytes before every response and rejects disagreement. Test-only corrupt modes
use isolated negative routes and are never accepted by the normal allowlist.
The WebSocket 101 handshake carries the applicable security headers. A browser
preflight opens/closes a real WSS connection before Qt is loaded; Node-only WSS
tests are insufficient.

- [ ] **Step 3: Replace the generic Qt shell**

Set target property:

```cmake
NO_WASM_DEFAULT_FILES TRUE
```

Copy the repository shell sources and the exact target Qt
`plugins/platforms/qtloader.js` into the runtime packaging inputs. The final
HTML contains only:

```html
<link rel="stylesheet"
      href="probe.<sha256>.css"
      integrity="sha256-<base64>"
      crossorigin="anonymous">
<script type="module"
        src="bootstrap.<sha256>.mjs"
        integrity="sha256-<base64>"
        crossorigin="anonymous"></script>
```

`bootstrap.mjs`:

1. creates `globalThis.__rhythmGameGate1b`;
2. fetches the runtime artifact manifest with `cache: "no-store"`;
3. fetches every executable asset and validates status, MIME, required response
   policy headers, URL digest, manifest digest, and SRI;
4. before any Qt/Wasm execution, validates `isSecureContext`,
   `crossOriginIsolated`, `SharedArrayBuffer`, JSPI API names, WebGL2,
   AudioWorklet, a nonce round-trip through the content-addressed diagnostic
   dedicated worker followed by its termination, and the real WSS preflight;
5. gives each failure an exact owned terminal code; wrong Wasm MIME cannot fall
   through to Emscripten's ArrayBuffer fallback;
6. dynamically injects the already-verified, content-addressed main JS and Qt
   loader with SRI, in that order;
7. supplies `locateFile` from the manifest so the main runtime fetches the
   content-addressed Wasm URL;
8. calls `qtLoad()` with `entryFunction:
   window.RhythmGameWasmProbe_entry` and one `#screen` container;
9. retains the returned Emscripten instance;
10. exposes only the fixed command allowlist;
11. records `onExit`, abort, unhandled rejection, and CSP violation as terminal
    failures. Task 5 attaches `processorerror` to each concrete node.

Neither `RhythmGameWasmProbe.js` nor `qtloader.js` may execute before Steps
1–5. Fetching an unhashed basename and later executing a separate response is
not accepted as verification.

Freeze the negative-mode outcomes in tests:

| Mode | Required terminal code | Boundary |
|---|---|---|
| `missing-coop` | `policy-coop-missing` | bootstrap response audit, before Qt |
| `missing-coep` | `policy-coep-missing` | bootstrap response audit, before Qt |
| `wrong-wasm-mime` | `artifact-wasm-mime` | bootstrap artifact audit, before Qt |
| `missing-wasm-unsafe-eval` | `policy-wasm-eval-missing` | CSP structural audit, before Qt |
| `blocked-worker-src` | `preflight-worker-csp-blocked` | real browser Worker construction, before Qt |
| `corrupt-bootstrap` | `sri-bootstrap-rejected` | Playwright observes SRI rejection and absence of bootstrap marker; never a generic timeout |
| `corrupt-main-js` | `sri-main-js-rejected` | bootstrap script `error`, before Qt |
| `corrupt-wasm` | `artifact-wasm-digest` | bootstrap artifact audit, before Qt |
| `corrupt-qtloader` | `sri-qtloader-rejected` | bootstrap script `error`, before Qt |

`blocked-worker-src` proves the dedicated-worker CSP destination only;
AudioWorklet module policy follows the script-source mapping and is not claimed
to be blocked by this mode.

- [ ] **Step 4: Generate the runtime manifest deterministically**

`package_runtime_artifacts.py` is a source-shape-checked post-link packager. It:

- consumes already-stamped generated `.aw.js`/`.ww.js` files when worker
  stamping exists, then hashes/copies CSS, bootstrap, preflight worker, Qt
  loader, Wasm, generated workers, and media under content-addressed names;
- patches the pinned generated main-JS worker/worklet basename literals to the
  content-addressed worker URLs, with exact occurrence counts;
- hashes/copies the resulting main JS;
- writes sorted canonical JSON with URL, bytes, SHA-256, SRI, MIME, and current
  input-digest `buildId`;
- generates the final HTML from the `.in` template with only the hashed/SRI
  CSS and bootstrap references;
- rejects missing/extra required assets, unknown generated shapes, and any
  absolute path or timestamp.

The final runtime directory intentionally omits the unused `qtlogo.svg`.
Historical Gate 1A evidence continues to describe the old default shell and
logo; no current Gate 1B verifier may rewrite that historical artifact set.

CMake models every copied/patched/manifest/HTML output as a `BYPRODUCT` of one
post-build command, gives the runtime-package target explicit dependencies on
the linked executable and copied Qt/shell/media inputs, and makes browser tests
depend on that target. Ninja must not race packaging against linking or worker
stamping. Once Task 5 adds stamping, the fixed order is: link, stamp generated
main/worker message guards and worklet heap-view refresh, hash the workers,
patch/hash the main runtime, then generate manifest and HTML.

The WebM fixture is a repository-owned two-second 64x64 VP8/Opus color-motion
clip. `fixtures/README.md` records the exact generation command, codec
parameters, license/ownership, bytes, and SHA-256. Do not use a remote media
URL.

- [ ] **Step 5: Run tests and a current-probe smoke request**

```powershell
python -m unittest `
  tools/wasm-probe/tests/test_gate1b_source_contract.py `
  tools/wasm-probe/tests/test_probe_source_contract.py -v

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test
```

Expected: PASS. Start/stop the server around the existing Gate 1A output and
assert all non-runtime routes and header captures; no browser PASS is claimed
yet.

- [ ] **Step 6: Review and commit**

Adversarially review CSP, traversal, WSS Origin, negative-route isolation,
manifest TOCTOU limitations, certificate truthfulness, and private-data
redaction.

Commit:

```text
feat: add isolated Gate 1B browser origin
```

## Task 3: Add the post-main browser bridge, JSPI, threads, and QML/QSB proof

**Files:**

- Create: `tools/wasm-probe/src/Gate1bReport.h`
- Create: `tools/wasm-probe/src/Gate1bReport.cpp`
- Create: `tools/wasm-probe/src/BrowserRuntimeBridge.h`
- Create: `tools/wasm-probe/src/BrowserRuntimeBridge.cpp`
- Create: `tools/wasm-probe/src/JspiNestedLoopProbe.h`
- Create: `tools/wasm-probe/src/JspiNestedLoopProbe.cpp`
- Create: `tools/wasm-probe/src/RenderProbe.h`
- Create: `tools/wasm-probe/src/RenderProbe.cpp`
- Modify: `tools/wasm-probe/src/main.cpp`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/qml/Main.qml`
- Modify: `tools/wasm-probe/qml/pulse.frag`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Create: `tools/wasm-probe/browser/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`

- [ ] **Step 1: Add failing source and browser contract tests**

Static tests require:

```cpp
struct BrowserCapabilities {
    bool secureContext;
    bool crossOriginIsolated;
    bool sharedArrayBuffer;
    bool jspiApi;
    bool webGl2Api;
    bool audioWorklet;
    bool opfs;
    bool fileSystemAccess;
};

class Gate1bReport final : public QObject {
    Q_OBJECT
public:
    void append(QStringView type, QJsonObject payload = {});
    void pass(QStringView check, QJsonObject detail = {});
    void fail(QStringView code, QStringView detail);
    [[nodiscard]] QJsonObject snapshot() const;
};

[[nodiscard]] BrowserCapabilities browserCapabilities();
void publishGate1bEvent(const QJsonObject &event);
void publishGate1bSnapshot(const QJsonObject &snapshot);

struct JspiNestedLoopResult {
    bool promiseResolvedWhileExec;
    bool quitDelivered;
    bool postLoopSentinel;
    quint32 requestedNonce;
    quint32 resolvedNonce;
    qint64 elapsedMicroseconds;
};
[[nodiscard]] JspiNestedLoopResult runJspiNestedLoop();
```

The CMake configuration must request the `QuickControls2` Qt component and link
`Qt6::QuickControls2`; the probe's existing vcpkg `qtdeclarative` dependency is
the expected provider. Do not add a second UI framework or change the root
manifest if that imported target is already present.

Browser tests initially require, then fail waiting for:

- API feature detection;
- an actual JSPI nested-loop result;
- distinct main, explicit pthread, and QtConcurrent identities;
- post-`main()` QML frames and timer;
- actual Qt scene-graph WebGL major version 2;
- two fixed shader-phase captures with expected pixel tolerances.

- [ ] **Step 2: Implement the append-only report and bridge**

Use all `EM_JS` and `EM_ASYNC_JS` declarations only in
`BrowserRuntimeBridge.cpp`, including the owned JSPI nonce import consumed by
`JspiNestedLoopProbe`. JSON crosses the boundary as UTF-8 and is parsed by the
pre-existing bootstrap object. The bridge must not create an alternative event
loop, use `emscripten_sleep`, or enable Asyncify link settings.

Feature detection requires:

```js
typeof WebAssembly.Suspending === "function"
&& typeof WebAssembly.promising === "function"
```

but marks JSPI passed only after the real nested loop returns.

Immediately before the Emscripten `main()` return, schedule a zero-delay Qt
callback that publishes `post-main-tick`, then publish `main-returning` and
return. Readiness requires `post-main-tick` after `main-returning`. All required
render, timer, network, thread, media, and audio completion events must have a
larger sequence number. Merely observing `main-returning` is not a lifetime
proof.

Task 3 also introduces the fixed Qt Quick activation `Button` and
`ProbeState::beginUserActivatedProbes()`. The synchronous `onClicked` call
records `navigator.userActivation.isActive` and starts all browser APIs that
need the same trusted gesture. Task 4 adds media to this method and Task 5 adds
audio; neither task creates a second activation path.

- [ ] **Step 3: Execute explicit pthread and QtConcurrent work**

`ProbeState` starts:

- the existing static-library throw/catch check;
- a `QtConcurrent::run` task that returns `42`, its thread identity, and a
  main-thread comparison;
- an explicit `pthread_create` task that transforms a nonce and records its
  thread identity.

The browser main thread never waits. A `QTimer` polls explicit-pthread
completion and calls `pthread_join` only after the completion atomic is
already set. Inline/fallback execution fails.

- [ ] **Step 4: Execute the real JSPI nested event loop**

After `post-main-tick`, a zero-delay Qt callback invokes:

```cpp
QEventLoop loop;
QTimer::singleShot(0, &loop, [&] {
    reportJspiBeforeImport();
    const auto resolvedNonce = awaitOwnedBrowserNonce(requestedNonce);
    reportJspiPromiseResolved(resolvedNonce);
    loop.quit();
});
const auto started = std::chrono::steady_clock::now();
loop.exec();
```

`awaitOwnedBrowserNonce()` is an owned `EM_ASYNC_JS` import that awaits a real
browser `Promise`/timer and resolves with a random run nonce. The timer callback
must start only after `exec()` is active, suspend at the import, resume with the
nonce, publish `jspi-promise-resolved`, and quit the nested loop while `exec()`
is active. Record ordered `jspi-before-exec`, `jspi-before-import`,
`jspi-promise-resolved`, `jspi-quit-delivered`, and `jspi-after-exec` events,
elapsed time, and exact nonce agreement. The browser test inspects that the
owned import went through the JSPI import path; an ordinary Qt timer alone
cannot pass. A main-thread blocking warning,
`ALLOW_BLOCKING_ON_MAIN_THREAD`, missing post-loop sentinel, or completion only
after `exec()` returns fails.

Use a separately owned asynchronous watchdog only to turn hangs into a named
failure; its callback must not be the successful quit path.

Do not reject generated JavaScript merely because it contains Emscripten's
internal `Asyncify` implementation identifiers under JSPI. The audited link
arguments—not a broad string search—must reject literal legacy
`-sASYNCIFY`/fallback settings.

- [ ] **Step 5: Instrument the actual Qt render path**

`RenderProbe` attaches to the loaded `QQuickWindow`. In the render callback it
records:

- `QSGRendererInterface::graphicsApi()`;
- current Emscripten WebGL context handle;
- the `EMSCRIPTEN_RESULT` from
  `emscripten_webgl_get_context_attributes(context, &attributes)` and
  `attributes.majorVersion`;
- frame sequence after `main-returning`.

Connect the scene-graph hook with `Qt::DirectConnection`. Because it can execute
on the render thread, it may update only fixed atomics there; a queued
GUI-thread callback converts those values into report events.

The QML keeps one full-window custom QSB `ShaderEffect`. Its phase normally
animates, but the bridge can stop animation and set exact phases `0.20` and
`0.80`. The browser clips a fixed interior region, decodes PNG with `pngjs`,
and checks expected RGBA values with a small declared tolerance plus a
different phase hash. A generic detached canvas WebGL2 check is only a
capability preflight and cannot satisfy the Qt render check.

Freeze the Playwright page to viewport `640x360`, `deviceScaleFactor: 1`, and
the same color scheme. For each exact phase, wait for two scene-graph frames
after the phase acknowledgement, capture the same declared interior rectangle,
and sample fixed declared coordinates. Store the expected RGBA formula and
per-channel tolerance in the test rather than deriving the expectation from the
captured image.

Use `QtQuick.Controls.Basic` `Button` for the fixed browser-probe activation
control. User-visible strings use `qsTr()`. The animation runs only while the
window is visible.

- [ ] **Step 6: Build and run the focused browser lane**

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --preset wasm-release
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build --preset wasm-release --verbose

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep "@core"
```

Expected: the positive `chromium-cft` core lane passes; all browser console
warnings/errors, page errors, failed requests, CSP violations, and blocking
warnings fail unless an exact expected negative-lane code owns them.

- [ ] **Step 7: Review and commit**

Review report ordering, object lifetime after early return, no main-thread
join, actual scene-graph context evidence, QML bindings/accessibility, and
shader determinism.

Commit:

```text
feat: execute Gate 1B Qt runtime core
```

## Task 4: Exercise same-origin QNAM, main-thread WSS, and served video

**Files:**

- Create: `tools/wasm-probe/src/NetworkProbe.h`
- Create: `tools/wasm-probe/src/NetworkProbe.cpp`
- Create: `tools/wasm-probe/src/MediaProbe.h`
- Create: `tools/wasm-probe/src/MediaProbe.cpp`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/qml/Main.qml`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Modify: `tools/wasm-probe/browser/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`

- [ ] **Step 1: Add failing end-to-end assertions**

QNAM must prove:

- exact same scheme/host/port;
- status 200;
- `application/json`;
- random run nonce in request/body;
- exact expected body and server-log request;
- no redirect and no CORS header dependency.

WSS must prove:

- `wss://<same-host>:<same-port>/probe/ws`;
- correct server-observed `Origin`;
- opened, text echo, binary echo, server message, application heartbeat, clean
  close;
- every Qt handler ran on `qApp->thread()`.

Media must prove:

- source is `/fixtures/probe.webm`, not Emscripten FS;
- metadata loaded;
- playing state;
- position advanced by at least 500 ms;
- the QML-owned `VideoOutput` has an attached sink and reaches changing
  playback timestamps;
- a nonblank browser capture from the `VideoOutput`;
- seek to 1,000 ms, end/stop, and clean teardown.

- [ ] **Step 2: Implement `NetworkProbe`**

Own `QNetworkAccessManager`, `QNetworkReply`, and `QWebSocket` in this object.
Derive URLs only through `BrowserRuntimeBridge::sameOriginUrl()`. Bound response
bytes, validate status/MIME/nonce before JSON parse, close WSS explicitly, and
make every terminal transition idempotent. Do not use `QWebSocket::ping()`,
arbitrary handshake headers, or a Wasm-side server.

- [ ] **Step 3: Implement `MediaProbe`**

Own `QMediaPlayer` and muted `QAudioOutput` in C++. Let QML `VideoOutput` own
its read-only `videoSink`, and synchronously pass `videoOutput.videoSink` to
`MediaProbe::attachVideoSink(QVideoSink *)` before playback. The C++ player uses
that sink and reports timestamps/state transitions. Mapping `QVideoFrame` data
is useful diagnostic evidence when the browser backend makes frames mappable,
but is not a blocking requirement; the blocking visual proof is the
`VideoOutput` screenshot plus changing playback/timing state. Start playback
synchronously from the Task 3 `beginUserActivatedProbes()` callback; do not pass
an autoplay bypass.

- [ ] **Step 4: Run CfT and branded Chrome focused lanes**

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep '@network|@media'
```

Expected: all three blocking browsers pass. CfT must pass the open VP8/Opus
fixture; Chrome Stable and Chrome Beta supply the two blocking branded-media
results.

- [ ] **Step 5: Review and commit**

Review same-origin enforcement, response bounds, main-thread identity,
heartbeat semantics, codec/fixture truthfulness, actual frame evidence, and
teardown.

Commit:

```text
feat: qualify Wasm network and media runtime
```

## Task 5: Qualify generated AudioWorklet, Wasm Workers, growth, and teardown

**Files:**

- Create: `tools/wasm-probe/src/SharedProbeRing.h`
- Create: `tools/wasm-probe/src/AudioWorkletProbe.h`
- Create: `tools/wasm-probe/src/AudioWorkletProbe.cpp`
- Create: `tools/wasm-probe/src/WasmWorkerCycleProbe.h`
- Create: `tools/wasm-probe/src/WasmWorkerCycleProbe.cpp`
- Create: `tools/wasm-probe/browser/web/gate1b-pre.js.in`
- Create: `tools/wasm-probe/browser/web/gate1b-library.js`
- Create: `tools/wasm-probe/scripts/stamp_worker_bootstraps.py`
- Create: `tools/wasm-probe/tests/test_stamp_worker_bootstraps.py`
- Modify: `tools/wasm-probe/scripts/package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/tests/test_package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Modify: `tools/wasm-probe/browser/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`

- [ ] **Step 1: Add failing unit/source/negative tests**

Require:

- generated `.aw.js` and `.ww.js` carry the exact current build ID;
- the main runtime injects that ID into both bootstrap messages;
- for each of `.aw.js` and `.ww.js`, corrupt bytes under the unmodified
  artifact manifest are rejected by the preloader before execution;
- for each worker type, a coherently content-addressed mixed-generation fixture
  (manifest/URL/hash all agree, but the worker has an old embedded build ID)
  produces `artifact-version-mismatch` inside the worker guard before Wasm
  instantiation or any callback;
- generated `.aw.js` refreshes cached HEAP views when
  `Module["wasmMemory"].buffer` identity changes;
- no `ScriptProcessor`, blob worker, generic `'unsafe-eval'`, legacy exception
  flag, or main-thread blocking option;
- `ALLOW_MEMORY_GROWTH=1`;
- exact initial memory 256 MiB, maximum 512 MiB, linear step 64 MiB, geometric
  growth disabled;
- public Emscripten audio destroy is not used as the close proof;
- Wasm Worker stacks/TLS are preallocated; `emscripten_malloc_wasm_worker()` is
  forbidden.

CMake expresses the Qt memory bounds exactly:

```cmake
set_target_properties(RhythmGameWasmProbe PROPERTIES
    QT_WASM_INITIAL_MEMORY "256MB"
    QT_WASM_MAXIMUM_MEMORY "512MB")
```

Keep the raw Emscripten linear/geometric growth controls needed for the 64 MiB
step, but do not also spell initial/maximum memory as competing raw linker
flags.

Run:

```powershell
python -m unittest `
  tools/wasm-probe/tests/test_stamp_worker_bootstraps.py `
  tools/wasm-probe/tests/test_gate1b_source_contract.py -v
```

Expected: FAIL.

- [ ] **Step 2: Version the generated worker bootstraps**

Configure `gate1b-pre.js.in` with the current input digest. It wraps only the
main-window `Worker` and `AudioWorkletNode` construction/termination points to
assign owned per-resource IDs and maintain resource metrics. It delegates all
native behavior, does not rewrite Emscripten bootstrap messages, and never
synthesizes success. Playwright worker events and CDP target lifecycle events
cross-check the wrapper metrics; none is sufficient alone.

`stamp_worker_bootstraps.py`:

- accepts exact generated main JS, `.aw.js`, `.ww.js`, and build ID arguments;
- checks the pinned 4.0.7 source shapes;
- patches the two generated main-JS bootstrap-message send sites to carry
  `rgBuildId`;
- patches the matching `.aw.js` and `.ww.js` receiver sites with embedded
  expected IDs and fail-closed checks;
- patches the generated `.aw.js` cached `HEAPU32`/`HEAPF32` setup at one
  source-asserted point so callback entry refreshes both views whenever
  `Module["wasmMemory"].buffer` changes;
- requires exact occurrence counts for every insertion;
- atomically writes deterministic bytes;
- inserts each build-ID check before Wasm instantiation/callback;
- fails on a second invocation or unknown generated shape.

CMake invokes it with pinned Python after link and before generating the
artifact manifest. The altered files remain Emscripten-generated bootstraps
with a small repository-owned compatibility/version guard.

The package script's negative-fixture mode runs after stamping.
Byte-corruption fixtures keep the original manifest and prove preloader
rejection. For each mixed-generation fixture it must:

1. copy the old-ID worker under its correct old-worker digest URL;
2. patch the already-stamped current main JS worker/worklet URL literal to that
   old-worker URL without changing the current `rgBuildId`;
3. hash the resulting main JS under its new digest URL;
4. update both main and worker manifest entries, SRI, bytes, and URLs.

The preloader must therefore accept the coherent alternate bundle; only the
old worker's embedded guard can produce the expected
`artifact-version-mismatch`. Execute both negative classes for both worker
types—static source assertions alone are insufficient.

- [ ] **Step 3: Implement the shared rings and one-context audio lifecycle**

`SharedProbeRing` is cache-line-aligned, fixed-capacity, allocation-free in the
callback, and uses explicit acquire/release atomics. The callback:

- confirms it is the AudioWorklet thread;
- consumes a cycle command;
- produces a low-amplitude non-silent deterministic waveform;
- writes checksum/callback sequence;
- never allocates, locks, logs, posts a JS message, or calls Qt.

`AudioWorkletProbe::startFromUserGesture()` records
`navigator.userActivation.isActive` and follows this owned asynchronous chain:

1. while still in the synchronous click stack, create the context and invoke
   `emscripten_resume_audio_context_async`;
2. only its running callback starts one Wasm AudioWorklet thread;
3. only the worklet-start callback calls the asynchronous named-processor
   creation;
4. only the processor-created callback creates/connects cycle nodes.

Call `emscripten_current_thread_is_audio_worklet()` once during worklet-thread
initialization and record it; do not call it on every realtime quantum.

The owned JS library depends explicitly on Emscripten's `$EmAudio` table and
adds:

```text
rg_gate1b_close_audio_context_async(handle, callback, userData)
rg_gate1b_audio_context_state(handle)
rg_gate1b_audio_handle_count()
```

Declare `__deps: ['$EmAudio']` and link the file with `--js-library`; accessing
an incidental global is forbidden. Close calls and awaits the actual
`AudioContext.close()`, requires native JS state `closed`, reports it, and only
then removes that exact handle from `EmAudio`. Record `preClickBaseline`; after
context creation record `activeContextBaseline == preClickBaseline + 1` and
require that active baseline after every cycle. Only after awaited close and
exact-handle removal may the count return to `preClickBaseline`. The PASS path
never calls
`emscripten_destroy_audio_context()` as evidence.

- [ ] **Step 4: Implement exactly 1,000 Wasm Worker/node cycles**

Before the click and before the designated growth, allocate all C++ cycle
records, callback slots, shared rings, one aligned region containing 1,000
non-overlapping `__builtin_wasm_tls_size() + stackSize` worker slots, and one
AudioWorklet stack. Verify these allocations do not themselves grow memory.

For each cycle:

1. create/connect an AudioWorklet node with a cycle-specific preallocated
   callback slot, attaching a cycle-correlated `processorerror` listener before
   connecting;
2. wait asynchronously for callback/checksum;
3. create a Wasm Worker with that cycle's preallocated region;
4. post a nonce transform and poll its atomic completion;
5. destroy the node;
6. wait two render quanta, reject a late callback, and remove that exact
   `processorerror` listener;
7. terminate the worker;
8. require the owned worker ID plus the independent Playwright worker event and
   CDP `Target.targetCreated`/`Target.targetDestroyed` identity for cycle N to
   close before cycle N+1; then require the browser-observed worker count to
   return to the four-pthread baseline;
9. append all required trace events;
10. require the `$EmAudio` handle count to equal `activeContextBaseline`, then
    proceed only after all baseline restoration.

No C++/Qt main-thread blocking wait is permitted.

- [ ] **Step 5: Force and prove live shared-memory growth**

At cycle 500, while callbacks advance:

1. record current `wasmMemory.buffer.byteLength`;
2. allocate one owning block whose size equals the old memory byte length,
   forcing one controlled growth request outside the callback;
3. obtain the new current buffer, require a different buffer identity, and
   record pages/bytes plus the refreshed `.aw.js` view identities; Emscripten
   may satisfy the one growth request with one or more 64 MiB linear
   increments;
4. select aligned storage wholly inside that allocation but at addresses
   greater than or equal to the old byte length, then construct the sentinel
   and second ring there;
5. perform the four-environment round trip;
6. require callbacks and the original ring to advance across the growth;
7. destroy the temporary above-boundary objects and free the owning block
   after acknowledgement;
8. require exactly one observed memory-length transition for the run and reject
   another change through cycle 999.

A raw unowned pointer into newly exposed memory is forbidden. A test that only
accesses pre-growth addresses is insufficient.

- [ ] **Step 6: Run mismatch, focused stress, and close tests**

Run first with a reduced test-only cycle count through a separately named unit
configuration, then the acceptance command must hard-code 1,000:

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep '@audio-negative|@audio-smoke'

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep "@audio-1000"
```

Expected in all three blocking browsers:

- cycle IDs exactly `0..999`;
- one memory growth at 500;
- `.aw.js` and `.ww.js` successful requests;
- post-growth sentinel/ring round trip;
- no late callback or worker leak;
- final AudioContext state `closed`;
- both negative classes for each worker type fail with the named reason and no
  callback/resource leak.

- [ ] **Step 7: Review and commit**

The adversarial reviewer checks realtime-safety, atomic ordering, stack/TLS
ownership, callback lifetimes, every cycle record, worker baseline, actual
close, memory-boundary proof, and mismatch behavior.

Commit:

```text
feat: stress Wasm audio and worker lifecycle
```

## Task 6: Qualify OPFS and the headed File System Access chooser

**Files:**

- Create: `tools/wasm-probe/src/StorageProbe.h`
- Create: `tools/wasm-probe/src/StorageProbe.cpp`
- Create: `tools/wasm-probe/browser/web/opfs-worker.mjs`
- Create: `tools/wasm-probe/browser/lib/storage-adapter.mjs`
- Create: `tools/wasm-probe/browser/lib/storage-adapter.test.mjs`
- Create: `tools/wasm-probe/browser/run-fsa-qualification.mjs`
- Create: `tools/wasm-probe/browser/manual/FSA-QUALIFICATION.md`
- Modify: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Modify: `tools/wasm-probe/scripts/package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/tests/test_package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.h`
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.cpp`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/qml/Main.qml`
- Modify: `tools/wasm-probe/browser/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`

- [ ] **Step 1: Add failing adapter and OPFS tests**

Fake-adapter tests cover:

- API absent;
- picker success/cancel/denial;
- read-only versus read-write permission;
- permission revocation;
- serialized-handle reload;
- missing/replaced directory;
- bounded enumeration;
- retry and terminal error mapping.

Browser tests require page/worker/reload/process-restart OPFS evidence and
explicit cleanup.

- [ ] **Step 2: Implement automated OPFS**

The page path uses `navigator.storage.getDirectory()`. The dedicated module
worker uses `createSyncAccessHandle()` where Chromium exposes it, performs
write/read/flush/close, and returns exact hashes. A persistent Playwright
profile proves reload and full browser-process restart persistence;
`storageState` is not used as an OPFS backup. Keep the same HTTPS server alive
on the exact same scheme, host, and port; close the entire browser process,
relaunch it with the same profile, and fail if the origin changes.

The packager content-addresses `opfs-worker.mjs`; bootstrap prefetches and
verifies it with every other executable asset. `BrowserRuntimeBridge` exposes
only the manifest-resolved worker URL through its fixed command allowlist, and
all C++-initiated worker construction remains inside that bridge so the Task 3
rule that `EM_JS` exists only there is preserved. A stable basename, blob URL,
or unverified worker construction fails.

- [ ] **Step 3: Implement the real chooser bridge**

Add a second fixed Qt Quick `Button`. Its synchronous `onClicked` reaches the
browser bridge, records active user activation, and calls
`showDirectoryPicker({id: "rhythmgame-gate1b", mode: "readwrite"})` before
awaiting anything else.

The chosen handle:

- enumerates the known relative canary files;
- validates bytes/hashes;
- creates, reads, and removes a round-trip canary;
- is structured-cloned into a dedicated IndexedDB record before reload and
  retrieved from that record afterward;
- records `queryPermission({mode: "readwrite"})` before and after reload;
- never publishes an absolute path.

After retrieving the exact handle from IndexedDB:

- if permission is `granted`, continue;
- if it is `prompt`, require a second genuine Qt Quick click and invoke
  `requestPermission({mode: "readwrite"})` synchronously from that gesture,
  recording the transition;
- if it is `denied`, missing, or a substituted handle, fail.

Persisted permission is not itself assumed or required; persisted handle
identity plus the explicit, browser-compliant reauthorization path is.

Before reload, create a random nonce canary through the selected real handle.
After IndexedDB retrieval and any required reauthorization, re-enumerate and
rehash the known directory entries, read the exact nonce bytes through the
restored handle, remove that canary, and prove its absence. Permission state
alone is not handle-identity evidence.

No fake adapter shares this result field.

- [ ] **Step 4: Run headed Stable qualification**

`run-fsa-qualification.mjs` prepares an ignored canary directory and a
persistent Stable profile, opens the probe headed, and waits at the native
chooser step. Use OS UI assistance or a human to select only that canary
directory. Capture:

- result JSON;
- browser/origin/profile identity;
- relative selected manifest;
- permission before/reload;
- an OS-level screenshot that includes the native chooser (a Playwright page
  screenshot cannot prove this), sanitized by the frozen path-privacy rule
  above;
- operator class and UTC timestamp.

Then validate the record against the current run/build ID. Cancellation,
timeout, wrong directory, or missing screenshot fails. If OS UI assistance
cannot safely identify/select the generated canary directory, pause for the
explicit user selection; never replace the chooser with a fake or page-level
file input.

- [ ] **Step 5: Run automated storage lanes**

```powershell
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep "@storage"
```

Expected: OPFS passes in all three blocking lanes, fake-adapter tests pass, and
FSA remains separately false until Step 4 evidence validates.

- [ ] **Step 6: Review and commit**

Review evidence-class separation, user-activation timing, chooser authenticity,
profile isolation, permission semantics, cleanup, and path privacy.

Commit:

```text
feat: qualify browser storage capabilities
```

## Task 7: Generate fail-closed Gate 1B evidence across blocking browsers

**Files:**

- Create: `tools/wasm-probe/browser/lib/acceptance-runner.mjs`
- Modify: `tools/wasm-probe/browser/gate1b.spec.mjs`
- Create: `tools/wasm-probe/browser/run-gate1b.mjs`
- Create: `tools/wasm-probe/tests/verify_runtime_evidence.py`
- Modify: `tools/wasm-probe/tests/verify_build.py`
- Modify: `tools/wasm-probe/tests/test_verify_build.py`
- Create after successful run:
  `docs/superpowers/evidence/emscripten-gate-1b.json`
- Create after successful runs:
  `docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson`
- Create after successful runs:
  `docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson`
- Create after successful runs:
  `docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson`
- Create after headed qualification:
  `docs/superpowers/evidence/emscripten-gate-1b-fsa.json`
- Create after headed qualification:
  `docs/superpowers/evidence/emscripten-gate-1b-fsa.png`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`

The evidence JSON, NDJSON traces, FSA JSON, and FSA screenshot are outputs, not
probe inputs. They must not enter `input-manifest.txt` or the runtime build ID.
Only the runner, validator, schemas, and tests enter the source manifest.

- [ ] **Step 1: Write the evidence validator first**

`verify_runtime_evidence.py` rejects:

- wrong/unknown schema fields;
- a different Gate 1A parent hash/commit;
- authority fields inconsistent with this plan;
- anything other than three distinct blocking-browser identities/hashes for
  CfT, Stable, and Beta;
- test certificate represented as trusted;
- skipped/retried/flaky tests;
- incomplete headers/MIME/request logs;
- absent or wrong negative-lane reason, including exact owned codes for every
  missing header, wrong MIME, CSP source block, corrupt artifact, and coherent
  worker-generation mismatch;
- any console warning/error, page error, failed request, CSP violation,
  unhandled rejection, worklet processor error, Emscripten abort, or
  main-thread blocking warning outside an exact negative expectation;
- generic WebGL2 substituted for Qt scene-graph WebGL2;
- QNAM/WSS/media checks lacking server-side corroboration;
- missing full traces or wrong trace hash chain;
- any cycle not exactly once in `0..999`;
- missing/extra/out-of-order lifecycle events;
- memory growth other than the declared event;
- missing above-old-boundary proof;
- final resource count mismatch or AudioContext state other than `closed`;
- OPFS missing persistence/cleanup;
- fake/automated FSA evidence substituted for native chooser evidence;
- `sourceCommit`, its Git-blob input digest, the clean worktree input digest,
  and packaged `buildId` differing in any way;
- `gate1bTechnicalPassed: true` while any required result is false;
- any authority field other than `gate1bTechnicalPassed` set true.

Run it against a deliberately incomplete fixture first and require failure.

- [ ] **Step 2: Implement one shared acceptance runner**

Both Playwright tests and `run-gate1b.mjs` call the same exported scenario.
The scenario:

1. launches one lane with no retries;
2. starts the exact server/build;
3. captures browser identity/arguments;
4. captures every request/response and server request log;
5. runs preflight, core, network, media, audio stress, storage, and teardown;
6. runs each negative server mode in a fresh context;
7. closes context/browser/server in `finally`;
8. returns canonical structured data.

There is no second permissive “evidence-only” path.

- [ ] **Step 3: Re-audit the current build commands and artifacts**

First extend `verify_build.py` with an explicit `gate1b-build-audit` mode and
schema, with failing unit fixtures before implementation. That mode writes only
to a caller-supplied ignored temporary output, emits
`"gate": "1B-build-audit"`, and has no field capable of asserting Gate 1A PASS.
Its tests prove it cannot overwrite, relabel, or mutate
`docs/superpowers/evidence/emscripten-gate-1a.json`.

The historical Gate 1A mode and its exact default-shell deployment set
(including `qtlogo.svg`) stay frozen. The Gate 1B mode instead accepts only the
content-addressed runtime set and requires `qtlogo.svg` to be absent. Do not run
the old Gate 1A deployment-set check against Gate 1B output.

The Gate 1B runner invokes this new mode and records:

- current source commit and input digest;
- immutable Gate 1A parent evidence hash/commit;
- every current C/C++ compile command with `-pthread` and native Wasm
  exceptions;
- exact final JSPI/AudioWorklet/WasmWorkers/pthread-pool/no-blocking/growth
  flags;
- absence of forbidden fallback settings;
- current HTML/JS/Wasm/aw/ww/Qt loader/media/server/test/package-lock hashes;
- artifact-manifest agreement.

Focused verifier tests cover both frozen Gate 1A fixtures and the new Gate 1B
fixture before any current build is accepted.

Internal Emscripten JSPI implementation identifiers are not treated as legacy
Asyncify flags; the actual build command is the authority. Literal
`-sASYNCIFY`, negative Wasm exceptions, or legacy JS exception settings fail.

- [ ] **Step 4: Commit the qualifier before producing evidence**

Run the fast Python/Node contract tests, review the runner/validator diff, and
commit all Task 7 source files before qualification:

```text
test: add Gate 1B runtime qualifier
```

The resulting commit is the `sourceCommit` recorded by evidence. Generated
evidence is not allowed to name the later evidence-only commit as its source.

Before any qualification build, enumerate the exact paths in
`input-manifest.txt` at `sourceCommit`. Require every path and the manifest
itself to exist as regular Git blobs, require the worktree bytes for that source
set to match those blobs, and recompute the build-input digest from the Git
blobs using the same canonical algorithm as CMake. Refuse dirty or untracked
runtime inputs. Generated evidence/profiles/traces remain outside that set and
may be untracked without invalidating source cleanliness.

- [ ] **Step 5: Run the full automated matrix**

```powershell
python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_*.py" -v

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --preset wasm-release
pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build --preset wasm-release --verbose

pwsh -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs qualify `
  --build tools/wasm-probe/build/wasm-release `
  --output docs/superpowers/evidence/emscripten-gate-1b.json
```

The full qualification runs `chromium-cft`, `chrome-stable`, and
`chrome-beta`, including one 1,000-cycle trace per lane and all
header/MIME/CSP/artifact/worker-generation mismatch negatives.
The runner requires the packaged `buildId` to equal both the Git-blob digest and
the clean worktree digest before launching the first browser.

- [ ] **Step 6: Validate headed FSA and aggregate**

Attach the separately generated FSA JSON/screenshot, validate its build ID and
browser identity, then run:

```powershell
python tools/wasm-probe/tests/verify_runtime_evidence.py `
  --evidence docs/superpowers/evidence/emscripten-gate-1b.json `
  --fsa-evidence docs/superpowers/evidence/emscripten-gate-1b-fsa.json `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson
```

Only this validated aggregation may set `gate1bTechnicalPassed: true`.

- [ ] **Step 7: Rebuild/no-op and deterministic-control checks**

Run the exact build again. Require only the authenticated `VerifyGlobs.cmake`
check and `ninja: no work to do`. Regenerate the runtime artifact manifest from
unchanged bytes and require byte equality. Runtime timestamps/browser versions
belong only in runtime evidence and do not contaminate build outputs.

- [ ] **Step 8: Review and commit the evidence**

An independent adversarial reviewer attempts to falsify every result and
checks the raw traces rather than trusting the aggregate booleans.

Commit:

```text
test: qualify Gate 1B Chromium runtime
```

## Task 8: Record the bounded result and stop at the Gate 0 decision

**Files:**

- Modify:
  `docs/superpowers/specs/2026-07-23-emscripten-web-port-design.md`
- Modify:
  `docs/superpowers/plans/2026-07-25-emscripten-gate-1b-chromium-runtime.md`
- Modify: `.superpowers/sdd/gate1b-progress.md`
- Modify only if generated evidence changes:
  `docs/superpowers/evidence/emscripten-gate-1b*`

- [ ] **Step 1: Add an as-built Gate 1B result**

Record exact commits, evidence paths/bytes/SHA-256, browser versions/hashes,
transport-trust mode, test counts/durations, trace counts/hashes, memory
old/new values, FSA evidence class, negative-lane outcomes, and every
limitation. Use “technical Gate 1B PASS” or “technical Gate 1B FAIL”; never
shorten it to “Gate 1 PASS”.

- [ ] **Step 2: Prove isolation from production code**

Run:

```powershell
git diff --exit-code `
  94fdcd315b02f909f7e5a4b6fa5989d5aa3cee28..HEAD -- `
  src RhythmGameQml vcpkg.json
```

Expected: no diff.

- [ ] **Step 3: Run complete local verification**

Run:

```powershell
python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_*.py" -v

python tools/wasm-probe/tests/verify_runtime_evidence.py `
  --evidence docs/superpowers/evidence/emscripten-gate-1b.json `
  --fsa-evidence docs/superpowers/evidence/emscripten-gate-1b-fsa.json `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson

git status --short
```

Also rerun the complete browser qualification if any runtime-affecting file
changed after Task 7.

- [ ] **Step 4: Final whole-branch review**

Review the entire range from
`94fdcd315b02f909f7e5a4b6fa5989d5aa3cee28` to `HEAD` for:

- spec compliance;
- false-positive evidence;
- ownership/UAF/race/realtime risks;
- browser-policy/security regressions;
- QML correctness;
- test-evidence integrity;
- machine-specific or private data;
- production-scope leakage.

Resolve every Blocking or Important finding and rerun affected verification.

- [ ] **Step 5: Commit the result**

Commit:

```text
docs: record bounded Gate 1B result
```

- [ ] **Step 6: Stop before Gate 2**

Present the evidence-backed technical result and ask the legal owner/user to
choose one of:

1. commercial Qt distribution; or
2. a GPLv3-compliant web distribution with Corresponding Source obligations.

Do not start Gate 2 dependency qualification or production application
adaptation until that decision and the remaining Gate 0 acceptance are
recorded.

## Final pass/fail checklist

Technical Gate 1B can pass only when all are true:

- [ ] immutable Gate 1A lineage is exact;
- [ ] current compile/link flags are mechanically audited;
- [ ] strict CSP has no unsafe fallback;
- [ ] cross-origin isolation/SAB/JSPI APIs and actual suspension pass;
- [ ] actual Qt WebGL2/QSB render and post-main lifetime pass;
- [ ] static-library exception, explicit pthread, and QtConcurrent execution
      pass with distinct thread evidence;
- [ ] QNAM, WSS, and served Qt Multimedia pass with server corroboration;
- [ ] generated/versioned `.aw.js` and `.ww.js` paths execute;
- [ ] byte-corruption and coherent mixed-generation lanes for both worker types
      fail closed before callback use;
- [ ] all three blocking browsers complete exact 1,000-cycle traces;
- [ ] live shared-memory growth reaches above-old-boundary data through every
      required execution environment;
- [ ] no node/worker/callback leak and actual AudioContext close is proven;
- [ ] OPFS page/worker/reload/process persistence and cleanup pass;
- [ ] real headed File System Access chooser evidence passes;
- [ ] all negative header/MIME/CSP/artifact lanes fail for the expected reason;
- [ ] console/page/network/CSP/worklet errors are clean;
- [ ] evidence validator and independent adversarial review pass;
- [ ] `src`, `RhythmGameQml`, and native root manifest remain untouched;
- [ ] Gate 0/formal Gate 1/production authorization fields remain false.
