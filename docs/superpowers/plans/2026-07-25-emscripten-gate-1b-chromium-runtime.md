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
- `vcpkgOverlayPortsWasm/qtbase/**` and
  `vcpkgOverlayPortsWasm/qtmultimedia/**` for Emscripten-only Qt runtime seams;
- scoped ignore entries in the repository `.gitignore`;
- scoped LF/binary evidence and schema rules in the repository
  `.gitattributes`;
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
| `chromium-cft` | yes | Playwright `channel: "chromium"`; regular CfT, new headless for fast tests plus the dedicated headed `chromium-cft-headed-stress` project for the full audio/growth run | Reproducible Chromium/Wasm functional result |
| `chrome-stable` | yes | Playwright `channel: "chrome"`, headed | Branded-Chrome GPU, audio, video, native browser integration result |
| `chrome-beta` | yes | Playwright `channel: "chrome-beta"`, headed; provision through the pinned Playwright CLI when absent | Branded upcoming-Chrome compatibility result |

The runner records Playwright version, channel, executable path normalized to a
machine-independent suffix, executable SHA-256, `browser.version()`, full launch
arguments, headless/headed state, and profile mode. All three blocking lanes
must resolve to distinct executable hashes. There are no retries. Because the
branded Beta channel is mutable rather than Playwright-content-pinned, evidence
binds its exact executable hash/version and becomes stale when that installation
updates.

`chromium-cft-headed-stress` is an acceptance execution project, not a fourth
browser identity. It must resolve to the exact `chromium-cft` executable hash
and contributes that lane's blocking 1,000-cycle record. The ordinary
`chromium-cft` project remains headless for bounded core and negative tests;
Stable and Beta remain headed for their complete blocking runs.

“Full launch arguments” means the effective browser command line obtained from
CDP `Browser.getBrowserCommandLine`, with the temporary profile path redacted
to a stable token. Playwright's user-supplied `launchOptions.args` alone is not
represented as the effective command line.

Pinned Playwright normally injects `--disable-back-forward-cache` in every
Chromium launch and also injects `--disable-background-timer-throttling`,
`--disable-backgrounding-occluded-windows`, and
`--disable-renderer-backgrounding`. The harness must remove the BFCache switch
with `launchOptions.ignoreDefaultArgs` in every lane. The two headed branded
lanes must remove all four switches so their hidden-tab evidence exercises
shipping Chrome lifecycle behavior; headless CfT retains the three background
switches because it does not claim hidden-tab coverage. The core test obtains
the effective command line through CDP and fails if any switch that should have
been removed is still present. A shared source policy and static source
contract prevent the config, runtime audit, and browser-identity helper from
drifting apart.

`Browser.getBrowserCommandLine` itself is available only when Chrome receives
`--enable-automation`, so that single inspection argument is harness-owned and
required in every blocking lane. It enables provenance inspection; it does not
enable a product capability or waive an acceptance condition.

Playwright also sends `Emulation.setFocusEmulationEnabled({enabled:true})` for
each managed main frame, which makes its multiple pages unsuitable as evidence
of real Page Visibility transitions. Therefore the main managed-page test never
claims hidden-tab coverage. Stable and Beta each run a separate `@headed`
lifecycle test: the harness resolves and hashes the branded executable through
the pinned Playwright channel, starts that exact executable with a temporary
profile and loopback CDP port, and reconnects with `noDefaults: true`. The
external Chrome command line is audited again. A real second tab must then
produce visible-to-hidden-to-visible transitions. CDP lifecycle/visibility
emulation is forbidden in this lane. Because `noDefaults` is a pinned
Playwright-internal connection contract, the runner verifies the exact installed
`playwright-core/lib/coreBundle.js` SHA-256 and the focus-emulation guard markers
before launching the lane. The generated loopback certificate remains a test
fixture, so only certificate error handling is enabled through CDP and the
evidence records `certificateTrustValidated: false`.

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
Permissions-Policy: fullscreen=(self), gamepad=(self), hid=(self), unload=()
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

The `unload=()` feature policy prevents the probe or a dependency from adding
the legacy unload handler that would make BFCache behavior browser-dependent;
the owned lifecycle contract uses `pagehide`/`pageshow`.

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
   - create one Wasm Worker with its own preallocated stack/TLS region;
   - send and verify the cycle nonce before creating the audio node;
   - create and connect one AudioWorklet node;
   - observe a non-silent callback/checksum;
   - at cycle 500 only, grow shared memory while that same Worker and node both
     remain live, then round-trip the above-old-boundary sentinel through main
     Wasm, an already-live pthread-pool worker, that same cycle Worker, and the
     AudioWorklet callback;
   - disconnect the node, wait the declared grace interval, and reject late
     callbacks;
   - request termination of the Wasm Worker;
   - publish `audio-cycle-awaiting-browser-ack`;
   - have Playwright independently observe Worker close, matching CDP target
     destruction, and return to the four-pthread baseline;
   - accept the matching `ack-audio-cycle-browser-teardown` command before the
     next cycle begins;
8. require exactly one shared-memory growth transition, in cycle 500;
9. after cycle 999, disconnect everything and use an owned JS-library adapter
   to `await AudioContext.close()`;
10. require browser state `closed`, no remaining probe node, and the original
    worker baseline.

The raw per-browser NDJSON trace contains, for every cycle, the exact causal
records below in this order. Browser observations retain their own producer
sequence and are joined through explicit run, cycle, node, Worker, and CDP
target IDs; timestamps from different clocks never establish order:

```text
cycle-start
worker-created
worker-nonce-processed
node-created
node-connected
audio-callback
audio-checksum
growth-started                         # cycle 500 only
growth-round-trip-completed            # cycle 500 only
node-disconnected
node-grace-completed
node-destroyed
worker-termination-requested
audio-cycle-awaiting-browser-ack
playwright-worker-closed
cdp-worker-target-destroyed
resource-baseline-restored
ack-audio-cycle-browser-teardown
cycle-completed
```

There is no retry, skipped ID, duplicate ID, timeout recovery, late callback,
or next-cycle start before the browser acknowledgement. A stale, duplicate,
wrong-cycle, or wrong-owned-Worker-resource acknowledgement is terminal.
Playwright Worker or CDP target disagreement is terminal in the runner before
it may send an acknowledgement. The evidence stores the complete traces and an
independent SHA-256 hash chain.

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

Task 6 produces only a headed chooser smoke record because its implementation
commit necessarily changes the source-bound build ID. The blocking FSA record
and screenshot are always regenerated after the Task 7 qualifier source commit
and fresh qualification build; an earlier smoke artifact is stale by
definition.

References:

- <https://developer.chrome.com/docs/capabilities/web-apis/file-system-access>
- <https://chromedevtools.github.io/devtools-protocol/tot/Browser/#type-PermissionType>

## Task 1: Lock the browser harness and fast Gate 1B contract

**Files:**

- Create: `tools/wasm-probe/browser/package.json`
- Create: `tools/wasm-probe/browser/package-lock.json`
- Create: `tools/wasm-probe/browser/run-browser-tool.mjs`
- Create: `tools/wasm-probe/browser/lib/browser-matrix.mjs`
- Create: `tools/wasm-probe/browser/lib/chromium-lifecycle-policy.mjs`
- Create: `tools/wasm-probe/browser/lib/external-lifecycle-browser.mjs`
- Create: `tools/wasm-probe/browser/playwright.config.mjs`
- Create: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/scripts/Invoke-WithToolchains.ps1`
- Modify: `tools/wasm-probe/tests/test_toolchain_scripts.py`
- Modify: `.gitignore`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`
- Create: `.superpowers/sdd/gate1b-progress.md` (ignored execution ledger)

- [x] **Step 1: Add failing fast contract tests**

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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_gate1b_source_contract.py" -v
```

Expected: FAIL because the browser harness does not exist.

- [x] **Step 2: Implement the authenticated Node/npm dispatcher**

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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs npm-lock

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs install-chromium

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs install-chrome-beta
```

The first command must populate only the ignored pinned Playwright browser
root. The Beta command must go through the pinned Playwright CLI and then
resolve the branded `chrome-beta` channel. Do not download or replace Stable;
only hash/qualify the installed Stable executable. Record no browser PASS yet.

- [ ] **Step 5: Make all focused tests green**

Run:

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_gate1b_source_contract.py" -v
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_toolchain_scripts.py" -v
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

- [x] **Step 1: Add failing policy/server/artifact tests**

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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test
```

Expected: FAIL because the server and shell do not exist.

- [x] **Step 2: Implement the server without framework fallbacks**

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

- [x] **Step 3: Replace the generic Qt shell**

Set target property:

```cmake
NO_WASM_DEFAULT_FILES TRUE
```

Link the application with both `-sDYNAMIC_EXECUTION=0` and
`-sEMBIND_AOT=1`. The former is mandatory for the frozen CSP because it removes
runtime `new Function` paths from both Embind and Emval; the latter generates
the binding invokers at build time so the CSP-safe path does not trade away
binding latency. Task 2 source contracts require both settings and its
packager/verifier rejects executable dynamic-code constructors in the actual
generated and patched main JavaScript. Do not relabel the locked Gate 1A
response-file audit; Task 7's separate Gate 1B effective-argument audit proves
both settings from the authenticated link record.

Copy the repository shell sources and the exact target Qt
`plugins/platforms/qtloader.js` into the runtime packaging inputs. The final
HTML contains only:

```html
<link rel="icon" href="data:,">
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

The pinned Qt 6.11.1 Wasm platform plugin synchronously appends one inline
`<style>` element to the initial screen's open shadow root. The frozen
`style-src 'self'` policy correctly blocks that operation, so the Gate 1B
bootstrap installs a Chromium-first compatibility adapter immediately before
`qtLoad()`. It shadows only `ShadowRoot.prototype.appendChild`, passes every
non-owned call through to the exact original descriptor, and accepts only the
single initial `#screen > #qt-shadow-container` stylesheet with all of:

- 5,238 UTF-8 bytes;
- SHA-256
  `6b7168686da79590ea116889998716dfa624e1467411daa2bffee066a867d53e`;
- 37 parsed rules and the `.qt-screen`/`.qt-window` selectors;
- no `@import` or `url(` token.

The adapter uses `CSSStyleSheet.replaceSync()` and
`ShadowRoot.adoptedStyleSheets`, then awaits the cryptographic fingerprint
before publishing the Emscripten instance. Any second owned stylesheet fails
synchronously before construction or adoption. The hook remains owned for the
instance lifetime, monitors its exact property descriptor, restores on
rejection/abort/exit, and never overwrites a later third-party replacement.
This probe deliberately supports one Qt screen container; a target-only Qt
overlay/upstream patch is the production cleanup if multi-screen container
support becomes required. Weakening CSP, adding a nonce/hash for the inline
style, or patching `Node.prototype`, `Document.createElement`, or
`HTMLElement.style` is not accepted.

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

- [x] **Step 4: Generate the runtime manifest deterministically**

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
- rejects missing/extra required assets, unknown generated shapes, host
  filesystem paths, and build timestamps in extended or basic ISO form,
  date-only ISO form, C/C++ `__DATE__`/`__TIME__` form, or RFC-style form.
  Quote-aware scanning permits only
  the exact reviewed slash literals currently owned by the pinned inputs:
  `/`, `//`, `/dev`, `/dev/null`, `/dev/shm`, `/dev/shm/tmp`, `/dev/stderr`,
  `/dev/stdin`, `/dev/stdout`, `/dev/tty`, `/dev/tty1`, `/home`,
  `/home/web_user`, `/proc`, `/proc/self`, `/proc/self/fd`, `/tmp`, the browser
  route `/probe/ws`, and Qt loader's documentation placeholder
  `/path/to/destination`. Every other quoted slash literal is rejected.
  Separate scans reject drive-letter paths, raw or JavaScript-escaped UNC and
  device paths, any case variant of a quoted `file:` URL except Emscripten's
  exact `file://` marker, and multi-component POSIX paths in unquoted
  comments/CSS after quoted spans are masked. Before applying those rules,
  direct JavaScript and CSS slash/backslash escapes (`\/`, `\xNN`, `\uNNNN`,
  `\u{...}`, and CSS hexadecimal escapes) are normalized; an encoded literal
  cannot bypass the same path contract. This boundary covers direct packaged
  literals, not arbitrary runtime string concatenation.

The final runtime directory intentionally omits the unused `qtlogo.svg`.
Historical Gate 1A evidence continues to describe the old default shell and
logo; no current Gate 1B verifier may rewrite that historical artifact set.

CMake models every fixed staging copy plus the fixed manifest and HTML as a
`BYPRODUCT` of one post-build command. Content-addressed leaf names are not
known until after linking and therefore are intentionally modeled by the
canonical runtime manifest rather than falsely claimed as configure-time Ninja
outputs. A separate explicit, read-only runtime-verification target validates
the exact runtime-directory file set, filename digests, byte counts, SHA-256,
SRI, MIME, and HTML CSS/bootstrap URL+SRI references against that manifest.
Deleting, adding, or changing any manifest-modeled leaf must make this target
fail even when the linked executable is otherwise up to date. The verification
target is not `ALL`, so a normal no-op build can remain a real
`ninja: no work to do`; browser tests depend on the explicit verification
target. The runtime-package target has explicit dependencies on the linked
executable and copied Qt/shell/media inputs. Ninja must not race packaging
against linking or worker stamping. Once Task 5 adds stamping, the fixed order
is: link, stamp generated main/worker message guards and worklet heap-view
refresh, hash the workers, patch/hash the main runtime, then generate manifest
and HTML.

The WebM fixture is a repository-owned two-second 64x64 VP8/Opus color-motion
clip. `fixtures/README.md` records the exact generation command, codec
parameters, license/ownership, bytes, and SHA-256. Do not use a remote media
URL.

- [ ] **Step 5: Run tests and a current-probe smoke request**

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_gate1b_source_contract.py" -v
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_probe_source_contract.py" -v

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --preset wasm-release -S tools/wasm-probe
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build tools/wasm-probe/build/wasm-release --target `
  RhythmGameWasmProbeRuntimeVerify --verbose

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test
```

Expected: PASS. Delete one manifest-referenced hash leaf in a disposable
packager fixture and assert the explicit verifier fails closed. Start/stop the
server around the existing Gate 1A output and assert all non-runtime routes and
header captures. After the current Gate 1B package is built and explicitly
verified, run a bounded real `chromium-cft` startup smoke against the positive
origin: `qtLoad()` must return and be retained, no CSP/page/request
warning/error may occur, and the request log must not contain
`/favicon.ico`. This is a startup regression check, not a Gate 1B browser PASS;
the full behavioral proof remains in later tasks.

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
- Modify: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Create: `tools/wasm-probe/browser/tests/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/browser/server/policy.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.test.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_toolchain_contract.py`
- Modify: `tools/wasm-probe/tests/test_verify_build.py`
- Modify: `tools/wasm-probe/tests/verify_build.py`
- Modify: `tools/wasm-probe/toolchain-lock.json`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Create:
  `vcpkgOverlayPortsWasm/qtbase/preserve-wasm-event-composed-path.patch`
- Modify: `vcpkgOverlayPortsWasm/qtbase/portfile.cmake`
- Modify: `vcpkgOverlayPortsWasm/qtbase/vcpkg.json`
- Modify only if configure creates a new immutable control path:
  `tools/wasm-probe/build-control-manifest.txt`

The Emscripten-only QtBase addition is fixed as follows:

- `preserve-wasm-event-composed-path.patch` SHA-256 is
  `f67e8e1eda7dc7abd208d113b3bc4057bbff7d45d79fdebd133c3fde3c0466ec`;
- that patch snapshots `Event.composedPath()` at queue time, restores the outer
  `currentEvent()` after nested native delivery, and provides two distinct
  Embind pumps. `qtSendPendingEvents` is a raw synchronous native-only export
  for lifecycle-bound input; it is deliberately not promising and therefore
  cannot form a second suspended Qt stack. `qtSendPendingApplicationEvents` is
  the sole promising owner and runs one guarded
  posted/native/timer/deferred-delete application cycle. Both reject the wrong
  runtime thread and an owned `suspendExclusive()` stack before touching
  queues; the full pump also defers while another Qt event-delivery stack owns
  the thread. Delivery depth uses owned increment/decrement accounting rather
  than snapshot rollback, and queued current-event contexts erase their exact
  owner on LIFO unwind;
- QtBase overlay `port-version` is `4`, required in both target and host status
  records even though the patch is applied only under
  `VCPKG_TARGET_IS_EMSCRIPTEN`;
- the exact QtBase overlay is 36 files with aggregate SHA-256
  `773114c93e22d01b1c39e0f1c8b3eca9c421b06964666619fd9cd0cafeb9b3f4`;
- the exact QtDeclarative overlay remains 4 files with aggregate SHA-256
  `979c7916e031af468a1a183a61e8bdc85412f591a39bbe8111a1f4152bfa2091`.

Evidence must record the exact added files, complete file-hash inventory,
aggregate, reviewed delta, and host/target status versions. Validation
recomputes the aggregate and rejects missing or phantom files. The sorted
110-entry `input-manifest.txt` includes the QtBase and Qt Multimedia Wasm
patches, `toolchain-lock.json`,
and `test_toolchain_contract.py` exactly once.

- [x] **Step 1: Add failing source and browser contract tests**

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

The reported JSPI elapsed time begins before the nested `exec()` and includes
the headed runner's deliberate pre-click adversarial work. It is diagnostic,
not a latency gate. Require it to be positive and long enough to prove a real
asynchronous turn, but do not impose an independent two-second upper bound:
the owned five-second Promise watchdog and the outer test timeout already own
hang detection, and a runner delay must not turn a correct resumption into a
false failure.

- [x] **Step 2: Implement the append-only report and bridge**

Use all `EM_JS` and `EM_ASYNC_JS` declarations only in
`BrowserRuntimeBridge.cpp`, including the owned JSPI nonce import consumed by
`JspiNestedLoopProbe`. JSON crosses the boundary as UTF-8 and is parsed by the
pre-existing bootstrap object. The bridge must not create an alternative event
loop, use `emscripten_sleep`, or enable Asyncify link settings.

Bootstrap owns the only append point and validates the exact event/snapshot
schemas before cloning and freezing them. Its four C++-to-browser entry points
are fixed, non-enumerable, non-writable, and non-configurable:

```text
appendEvent(event)
publishSnapshot(snapshot)
resolveReady(snapshot)
rejectReady({code, detail})
```

`resolveReady` accepts only a schema-valid snapshot equivalent to the snapshot
already published. All five authority fields must be exactly `false` at this
boundary. Public `report.command()` is the opposite direction only. Task 3
initially introduces `probe-ping` and `set-shader-phase`; the currently
integrated media and browser-lifecycle slices extend the ordinary allowlist to
exactly:

```text
ack-media-frame-capture
arm-bfcache-resume-probe
arm-hidden-timer-probe
arm-visible-resume-timer-probe
begin-foreground-latency-sampling
probe-ping
set-shader-phase
```

The complete ordinary set must equal that list, with no extra route. The only
destructive commands are `trigger-native-depth-limit` on the
`native-depth-limit` path and `trigger-native-suspension-trap` on the
`native-suspension-trap` path; they are unavailable everywhere else. Do not
multiplex publication through command names, expose the raw Emscripten
instance, or perform arbitrary retained-instance property lookup.

Feature detection requires:

```js
typeof WebAssembly.Suspending === "function"
&& typeof WebAssembly.promising === "function"
```

but marks JSPI passed only after the real nested loop returns.

The pinned Qt 6.11.1 Wasm runtime needs an explicit post-return event-drain
contract. With `wasm_jspi` enabled, `QWasmSuspendResumeControl` queues native
events while no `processEvents()` suspension is active; after this probe's
intentional early `main()` return, timers, Qt Quick animation/rendering, and
QML input can otherwise remain queued indefinitely. After retaining the
`qtLoad()` instance, bootstrap must therefore bind the fixed
`instance.qtSendPendingEvents` native-only export and
`instance.qtSendPendingApplicationEvents` full-cycle export, then install one
bounded pump:

- idle and command progress owns one primary full-cycle promising export call;
- commands await every active pump call before crossing the fixed synchronous
  command boundary;
- visible idle progress is driven by `requestAnimationFrame`; a hidden document
  switches to one owned, cancelable 50 ms timer fallback because animation
  frames are suspended in background tabs. The separate external-CDP headed
  branded-Chrome lane must put the application behind a real foreground tab,
  arm an owned 125 ms
  `Qt::PreciseTimer`, and observe its C++ sentinel while
  `document.visibilityState === "hidden"` from that same hidden-fallback pump
  serial. Returning visible cancels the fallback and resumes animation-frame
  scheduling. The test captures a fresh `idleFrames` baseline after the hidden
  sentinel, requires an automatic visible animation frame beyond that fresh
  baseline before issuing another command, and only then arms a second owned
  125 ms Qt timer. That timer must be delivered by a visible animation-frame
  pump and produce a later C++ sentinel. Headless CfT records this headed-only
  coverage as not run; it must not synthesize or override Page Visibility
  state;
- bubbling Qt input, context-menu, and title-bar wakeups invoke the pump
  directly from the outer screen's bubble listener, after Qt's shadow-DOM
  target handler has queued the native event but before dispatch ends;
- non-bubbling `pointerenter` and `pointerleave` use same-target listeners on
  every inner `.qt-window`, registered after Qt. A `MutationObserver` attaches
  and detaches those listeners for dynamic windows, and the runtime workload
  dispatches both event types while the primary call is suspended;
- Chromium clipboard wakeups use a `document` bubble listener registered after
  `qtLoad()`, so Qt's same-target listener queues first and the pump drains
  while `clipboardData` and `preventDefault()` are still effective;
- when the one promising application call is suspended, each lifecycle-bound
  native dispatch opens a raw synchronous native event-drain stack instead of
  reusing the unresolved primary Promise; this is required for `preventDefault()`,
  clipboard transfer, drag acceptance, and trusted-activation work to complete
  before native dispatch returns when Qt is not inside `suspendExclusive()`;
- the pump tracks synchronous native dispatch depth in `try`/`finally`, enforces
  a hard depth limit of four, and fails closed before opening a fifth native
  stack. The browser report exposes the fixed limit and observed native depth;
- the guarded Qt wrapper reports an exclusive deferral as `false`. Bootstrap
  records and coalesces that deferral for a later full-cycle retry. Browser
  lifecycle APIs requiring same-dispatch completion remain JS-first;
- the native input pump must return a boolean synchronously. Any exception,
  non-boolean result, or attempted suspending import is terminal; the Wasm
  instance is never reused because a JSPI trap may bypass C++ unwinding;
- the Emscripten-only QtBase overlay snapshots `Event.composedPath()` when
  `QWasmSuspendResumeControl` clones a queued event, because the browser returns
  an empty path after dispatch; this keeps any event that must remain queued
  routable after resumption;
- an ordinary browser-to-C++ command first awaits all pump calls, invokes the
  fixed synchronous command export, and only then issues its post-command pump
  kick. A destructive command instead disables/cancels idle-frame scheduling
  while the pump is quiescent, skips that full-cycle kick, and queues its DOM
  trigger as the next browser microtask after the raw command export returns;
- abort, exit, and bootstrap rejection stop the pump, cancel the outstanding
  animation frame/timer, and remove owned input listeners;
- persisted `pagehide` pauses scheduling without destroying the retained
  instance; matching persisted `pageshow` resumes exactly once and kicks one
  full application cycle. The positive lane must perform a real same-origin
  away navigation and browser back navigation against revalidating
  `Cache-Control: no-cache` HTML, require the actual
  `pageshow.persisted === true` branch, and deliver a retained run nonce through
  a C++ `QWasmSuspendResumeControl` handler. Arming the nonce cancels and
  suppresses idle scheduling until the history navigation begins, so no
  unrelated application pump can straddle the cache transition. A
  session-scoped nonce expectation is cleared only by the retained
  `pageshow.persisted` handler; if the document reloads instead, the new
  bootstrap reports `bfcache-restoration-missed` immediately rather than
  timing out. A constructed `PageTransitionEvent` is not evidence.
  Non-persisted `pagehide` remains terminal cleanup.

Expose read-only pump telemetry. The focused workload must observe at least one
reentrant input call, both non-bubbling window wakeups, exactly one current
`.qt-window` wake target, at least one real-DOM `exclusiveDeferral`, and
`1 <= maxNativeDispatchDepth <= nativeDispatchDepthLimit == 4`.
`maxConcurrentCalls >= 2` is retained only as evidence that real input ran while
the explicitly observed primary workload was suspended; at most five calls are
active when the one primary owner and four native levels coexist. Do not use
arbitrary Module-property lookup, unbounded non-input pump calls, an interval
loop, or `JSPI_EXPORTS`. Only the full-cycle export uses
`emscripten::async()` when `wasm_jspi` is enabled; it is the required outer JSPI
call path for the owned `EM_ASYNC_JS` import.

The telemetry retains all 64 bounded foreground input samples. Playwright
generates sequential trusted `page.mouse` movement, the screen capture listener
starts each sample, and the matching spontaneous `QQuickWindow` mouse event
must publish its C++ ordinal during the same raw native dispatch. Playwright
recomputes nearest-rank p95 from the raw samples and requires at most 8 ms.

An independently owned `Qt::PreciseTimer` runs 32 sequential 5 ms samples.
C++ records the nonnegative lateness of every delivered timeout, exposes the
raw microseconds, and the browser binds every sample to the active full
application-pump serial. Playwright recomputes nearest-rank p95 with a 34 ms
maximum. JavaScript `dispatchEvent()` and browser `setTimeout()` cannot satisfy
either metric. These are foreground DOM-to-Qt and retained-Qt-timer
qualification gates; they are not the production keyboard/WebHID-to-session or
input-to-audio latency qualification defined by the web-port design.

Run three isolated adversarial workloads in addition to the positive core:

- keep the primary application pump suspended, synchronously nest four native
  handlers, and verify exact LIFO enter/exit order, strict current-event object
  identity, a stable nonempty queued `composedPath()` snapshot, four native
  stack canaries, and the suspended primary stack canary;
- attempt a fifth native level in a disposable page and require terminal
  `runtime-native-event-pump-depth-limit` before a fifth probe handler opens;
- attempt a definitely-pending `EM_ASYNC_JS` import from a native handler in a
  separate disposable page and require terminal
  `runtime-native-event-pump-failed`. The Promise never resolves, so a runtime
  which incorrectly permits the suspension hangs/fails rather than reaching an
  accepted bypass throw. Both destructive pages first finish the complete
  positive core and quiesce every promising pump; never continue using a
  trapped Wasm instance.

Qt's synchronous `qstdweb::Promise::suspendExclusive()` deliberately suppresses
all unrelated handlers, so it cannot provide browser lifecycle semantics
during that interval. The web architecture therefore keeps chooser, file,
clipboard, drag/drop, and activation-critical work in the browser bridge's
native asynchronous APIs and does not use Qt's synchronous Blob/File System
Access wrappers for those paths. The guard preserves queued Qt events for the
next normal drain; it does not misrepresent delayed delivery as same-dispatch
success.

Immediately before the Emscripten `main()` return, schedule a zero-delay Qt
callback that publishes `post-main-tick`, then publish `main-returning` and
return. Readiness requires `post-main-tick` after `main-returning`. All required
render, timer, network, thread, media, and audio completion events must have a
larger sequence number. Merely observing `main-returning` is not a lifetime
proof.

Before an idle `probe-ping` command returns, queue one discriminating retained
application cycle: a posted custom event, a Qt Wasm native handler, an owned
registered `QTimer` started at zero, and a `deleteLater()` sentinel. Do not use
`QTimer::singleShot(0)`, which the pinned Qt optimizes into a queued invocation
rather than the timer phase. The command awaits its post-kick and must observe
exact order `[posted, native, timer, deferred-delete]` before its Promise
resolves. The browser observer binds the completed check to the active
`runtime-command` application-pump serial; a source-only inspection of the Qt
patch cannot satisfy this check.

Task 3 also introduces the fixed Qt Quick activation `Button` and
`ProbeState::beginUserActivatedProbes()`. The synchronous `onClicked` call
records `navigator.userActivation.isActive` and starts all browser APIs that
need the same trusted gesture. Task 4 adds media to this method and Task 5 adds
audio; neither task creates a second activation path.

- [x] **Step 3: Execute explicit pthread and QtConcurrent work**

`ProbeState` starts:

- the existing static-library throw/catch check;
- a `QtConcurrent::run` task that returns `42`, its thread identity, and a
  main-thread comparison;
- an explicit `pthread_create` task that transforms a nonce and records its
  thread identity.

The browser main thread never waits. Emscripten 4.0.7 calls
`emscripten_check_blocking_allowed()` unconditionally from `pthread_join`, even
when the worker has already completed, so `pthread_join` is forbidden on this
path. Compile the probe with `_GNU_SOURCE`; a `QTimer` polls explicit-pthread
completion and then calls nonblocking `pthread_tryjoin_np`. `EBUSY` keeps
polling, zero records the result, and every other result is a named failure.
Inline/fallback execution fails. Static tests reject `pthread_join` and require
the exact `pthread_tryjoin_np`/`EBUSY` contract.

- [x] **Step 4: Execute the real JSPI nested event loop**

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
browser `Promise`. Its success path is armed only after `exec()` is active and
requires a trusted primary-button `pointerup` captured on the owned screen; a
short timer then resolves the random run nonce. Before that click, the browser
test dispatches a clipboard event and a trusted right-click while the primary
pump is suspended. It requires same-dispatch clipboard suppression, trusted
context-menu suppression, a reentrant drain, and no nonce resolution. Because
Chromium automation and the right-click can both leave transient activation
active, the test then consumes that state through an owned one-shot popup,
closes it, and requires `navigator.userActivation.isActive === false`
immediately before the primary click. The subsequent fixed Qt Quick activation
click must run QML synchronously in its reentrant lifecycle drain while user
activation is active. Its report event is ordered after `jspi-before-import`
but before `jspi-promise-resolved`. The import then publishes
`jspi-promise-resolved` and quits the nested loop while `exec()` is active.
Record ordered `jspi-before-exec`, `jspi-before-import`,
`jspi-promise-resolved`, `jspi-quit-delivered`, and `jspi-after-exec` events,
elapsed time, and exact nonce agreement. The browser test inspects that the
owned import went through the JSPI import path; an ordinary Qt timer alone
cannot pass. A main-thread blocking warning,
`ALLOW_BLOCKING_ON_MAIN_THREAD`, missing post-loop sentinel, or completion only
after `exec()` returns fails.

The nested event loop starts from a callback drained by the primary
`qtSendPendingApplicationEvents` promising export. Its Promise remains active across
nested `QEventLoop::exec()` suspension and the owned async nonce import.
Lifecycle-bound native dispatch may start only the direct reentrant drain
described above; Embind command export during that suspension remains a
blocking correctness failure.

The same core run must execute a discriminating regression for the overlay's
exclusive-suspension guard. Register dedicated handlers `N1`, `N2`, `E`, and
`completion`, then:

1. enter `suspendExclusive({E})`;
2. queue `N1`;
3. call the raw synchronous `qtSendPendingEvents` export and require `false`,
   the same exclusive owner, and an unchanged `[N1]` probe queue;
4. queue `N2` only after that first guard call, then repeat the foreign call
   and require `false`, the same owner, and unchanged `[N1, N2]` FIFO state;
5. invoke `E` from a capture listener on a real bubbling DOM event, require the
   bootstrap input pump to coalesce its `false` result, require that only the
   exclusive owner resumes, and have its
   unchanged direct C++ drain deliver exactly `E`;
6. enter ordinary suspension, call the raw export again, require `true` and
   exact once-only `N1`, `N2` FIFO delivery, then use `completion` to resume
   the ordinary owner;
7. require final C++ delivery order
   `[E, N1, N2, completion]`, one exclusive-owner drain, one ordinary-owner
   completion drain, and no queue loss.

Then keep the one promising application owner suspended and synchronously nest
four raw native handlers. Require enter order `[1, 2, 3, 4]`, exit order
`[4, 3, 2, 1]`, strict `currentEvent()` identity before and after each nested
call, stable nonempty queued `composedPath()` snapshots, intact stack canaries,
and a `false` full-pump attempt while the primary owner is live. Raw handlers
must not suspend. The two disposable negative routes separately prove that the
bootstrap rejects a fifth raw level and terminal-traps a definitely-pending
suspending import with no older promising Wasm continuation in flight.

Queuing `N2` after the first foreign call is intentional: without the guard,
the first bad drain clears Qt's exclusive sentinel and `N2` resumes the wrong
stack. A workload that never exercises an `exclusiveDeferral` does not
exercise this failure branch and cannot satisfy the regression.

Use a separately owned asynchronous watchdog only to turn hangs into a named
failure; its callback must not be the successful quit path. Rejection of
`report.ready` immediately cancels the owned pointer listener and its timers
without resuming an already terminal Wasm stack.

Do not reject generated JavaScript merely because it contains Emscripten's
internal `Asyncify` implementation identifiers under JSPI. The audited link
arguments—not a broad string search—must reject literal legacy
`-sASYNCIFY`/fallback settings.

- [x] **Step 5: Instrument the actual Qt render path**

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

- [x] **Step 6: Build and run the focused browser lane**

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --preset wasm-release -S tools/wasm-probe
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build tools/wasm-probe/build/wasm-release --verbose

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --project chromium-cft --grep "@core"
```

Expected: the positive `chromium-cft` core lane passes; all browser console
warnings/errors, page errors, failed requests, CSP violations, and blocking
warnings fail unless an exact expected negative-lane code owns them.

- [x] **Step 7: Review and commit**

Review report ordering, object lifetime after early return, the single
promising-owner boundary, synchronous native depth and terminal traps, no
main-thread join, actual scene-graph context evidence, QML
bindings/accessibility, shader determinism, retained application-cycle order,
trusted foreground DOM-to-Qt latency, owned Qt-timer lateness, headed hidden-tab
native progress, and real persisted pagehide/pageshow recovery.

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
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.h`
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.cpp`
- Modify: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Modify: `tools/wasm-probe/browser/server/artifact-manifest.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.test.mjs`
- Modify: `tools/wasm-probe/browser/tests/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_probe_source_contract.py`
- Modify: `tools/wasm-probe/tests/test_toolchain_contract.py`
- Modify: `tools/wasm-probe/tests/test_verify_build.py`
- Modify: `tools/wasm-probe/tests/verify_build.py`
- Modify: `tools/wasm-probe/toolchain-lock.json`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`
- Modify: `.gitattributes`
- Create: `vcpkgOverlayPortsWasm/qtmultimedia/port.data.cmake`
- Create: `vcpkgOverlayPortsWasm/qtmultimedia/portfile.cmake`
- Create: `vcpkgOverlayPortsWasm/qtmultimedia/vcpkg.json`
- Create:
  `vcpkgOverlayPortsWasm/qtmultimedia/defer-wasm-media-device-notifications.patch`
- Create:
  `vcpkgOverlayPortsWasm/qtmultimedia/canonicalize-wasm-build-paths.patch`
- Create:
  `vcpkgOverlayPortsWasm/qtmultimedia/correct-wasm-media-lifecycle.patch`

- [x] **Step 1: Add failing end-to-end assertions**

QNAM must prove:

- exact same scheme/host/port;
- status 200;
- `application/json`;
- random run nonce in request/body;
- exact expected body and the matching append-only server `probeLogs` record,
  correlated by run nonce and request ID;
- no redirect and no CORS header dependency.

WSS must prove:

- `wss://<same-host>:<same-port>/probe/ws?nonce=<run-nonce>`;
- correct server-observed `Origin`;
- opened, text echo, binary echo, server message, application heartbeat, clean
  close, with one nonce- and connection-ID-correlated `probeLogs` stream;
- every Qt handler ran on `qApp->thread()`.

Media must prove:

- source is the fixed same-origin
  `/fixtures/probe.webm?nonce=<run-nonce>` alias, not Emscripten FS;
- metadata loaded;
- playing state;
- position advanced by at least 500 ms;
- the QML-owned `VideoOutput` has an attached sink and emits at least two
  `videoFrameChanged` events, each sampling `QMediaPlayer::position`, with
  strictly increasing sampled positions;
- a nonblank browser capture from the `VideoOutput` after those two frames and
  before end of media;
- seek to 1,000 ms only after the screenshot;
- at least two strictly increasing post-seek video frames, beginning at or
  beyond 1,000 ms and advancing to at least 1,100 ms;
- end position within 125 ms of the positive reported duration, so an
  immediate erroneous `EndOfMedia` after a successful seek cannot reuse the
  pre-seek frame proof;
- natural `QMediaPlayer::EndOfMedia`, exact media-element source reset/removal,
  `QMediaPlayer` destruction, a post-destruction browser resource-state proof,
  and independent `QAudioOutput` destruction acknowledgement before clean
  teardown. Calling public
  `QMediaPlayer::stop()` after completion is forbidden because it would replace
  the natural-end evidence.

The server exposes append-only, read-only `probeLogs` to the acceptance runner.
Every QNAM, WSS, and media record contains the validated run nonce plus its
transport-specific request or connection ID. C++/browser events carry the same
IDs. Acceptance joins those exact records; it never selects “the latest request
for this path” or infers a match from timestamps.

- [x] **Step 2: Implement `NetworkProbe`**

Own `QNetworkAccessManager`, `QNetworkReply`, and `QWebSocket` in this object.
Derive URLs only through `BrowserRuntimeBridge::sameOriginUrl()`. Bound response
bytes, validate status/MIME/nonce before JSON parse, close WSS explicitly, and
make every terminal transition idempotent. Put the run nonce in both the QNAM
query and WSS query; use an application heartbeat carrying that nonce rather
than `QWebSocket::ping()`. Do not use arbitrary handshake headers or a Wasm-side
server.

- [x] **Step 3: Implement `MediaProbe`**

Own `QMediaPlayer` and muted `QAudioOutput` in C++. Let QML `VideoOutput` own
its read-only `videoSink`, and synchronously pass `videoOutput.videoSink` to
`MediaProbe::attachVideoSink(QVideoSink *)` before playback. The C++ player uses
that sink and reports every `videoFrameChanged` with the current player
position. Mapping `QVideoFrame` data is useful diagnostic evidence when the
browser backend makes frames mappable, but is not a blocking requirement; the
blocking visual proof is the pre-end `VideoOutput` screenshot plus two
increasing frame-position samples.

The fixed `/fixtures/probe.webm` alias resolves only the current artifact
manifest's media entry. Before every 200, 206, or 416 response, the server
revalidates the manifest length, bytes, SHA-256, and SRI against the current
fixture, preserves correct range semantics, and emits `Cache-Control: no-store`.
It never bypasses the artifact check or inherits the immutable cache policy of
the content-addressed leaf.

Start playback synchronously from the Task 3
`beginUserActivatedProbes()` callback; do not pass an autoplay bypass. After the
two frame samples and screenshot, seek to 1,000 ms, wait for natural
`EndOfMedia`, then arm an observation-only browser tracker and destroy the
owned player. The tracker binds the exact nonce-owned element and installs an
`emptied` listener, but performs no `pause()`, source mutation, `load()`, or DOM
removal. `QWasmVideoOutput` destruction owns terminal element cleanup; the
player disconnects that output from the dying player before deleting it. The
retained browser reference must remain stable for 250 ms and prove
`NETWORK_EMPTY`, `HAVE_NOTHING`, paused/non-seeking state, zero time, cleared
duration/error/source candidates, no connected owned element, and the same
element/run identity. Chromium may retain the old selected URL in `currentSrc`
after this reset; accept only empty or that exact pre-destruction URL as
diagnostic identity, never as the resource-release oracle. Do not call public
`stop()` or use tracker-side cleanup on the successful end path.

Qt Multimedia 6.11.1 has a deterministic Wasm self-deadlock before player
creation completes. `QCachedValue::ensure()` holds the audio-output cache write
lock while `QWasmMediaDevices::getOpenALAudioDevices()` synchronously calls
`onAudioOutputsChanged()`, whose cache reset recursively takes the same lock.
The exact symbolized runtime chain is:

```text
QWasmMediaDevices::getOpenALAudioDevices
  -> QPlatformAudioDevices::onAudioOutputsChanged
  -> QCachedValue<QList<QAudioDevice>>::reset
  -> QBasicReadWriteLock::contendedTryLockForWrite
  -> QReadWriteLockPrivate::lockForWrite
  -> std::condition_variable::wait
  -> __pthread_cond_timedwait
  -> Emscripten main-thread blocking abort
```

The Wasm-only qtmultimedia overlay backports the relevant device-state part of
upstream Qt commit `7f71286a9f22ae69936a21b561570c4ea1af2431`
plus commits `35b0ea686685cb591d598d503bfa110daf6c69e2`,
`6cedb5d96f36c5d406d1bb58352dc05523c62fa4`,
`9018988e854ce6b7689e716b4afe93d6173a135c`, and
`191cda01b86bc6b28e663426bb7a3eef6b2d39cc`. It publishes the singleton before
starting device enumeration, routes both initial and `devicechange`
enumerations through serialized, coalesced, non-suspending Promise callbacks,
corrects the capture/playback cache keys, and reconciles browser devices on
every enumeration while excluding the OpenAL source/sink keys from stale
removal. The Promise callback path is mandatory because a synchronous native
input drain can deliver `devicechange`; awaiting there would attempt to suspend
through a non-promising export. Coalescing keeps one enumeration in flight and
starts exactly one fresh enumeration when changes arrive while it is pending,
so an older Promise cannot complete after and overwrite newer device state.
Those fallbacks remain the sole defaults
because Qt 6.11 opens their IDs through `alcOpenDevice`; browser `deviceId`
values are retained as
non-default inventory. The patch queues all five Qt 6.11 notification sites:
the three browser enumeration callbacks plus both still-present OpenAL
callbacks.

The Wasm media-lifecycle patch fixes three independent Qt 6.11.1 URL-video
defects. A normal `HTMLMediaElement.load()` resource reset queues `emptied`;
that event now revokes readiness without masquerading as
`QMediaPlayer::EndOfMedia`, while the real browser `ended` event remains the
only natural-end source. The patch also backports upstream commits
`4e61fa7da7e7db730e6e4762839de24f99e7803c`,
`8a2093d1dda70eae63a0522537c93605f8932041`, and
`d6f64920e9024de8cf0d8761f304fc8999700783`: an idempotent idle stop is
silent, natural end no longer recursively tears down the element, stop guards
re-entry before member access, and the optional capture-stream pointer is
initialized and null-checked. `QWasmVideoOutput` destruction now latches
terminal state before DOM mutation, clears URL-video resource selection with
`load()`, clears only this element's non-null `srcObject`, and detaches it. It
does not stop the singleton capture stream, which may be shared by another
camera output. The media-player destructor disconnects only the dying player
from the output before deletion, so ordinary `QObject::destroyed` observers
remain valid. This backend-owned boundary retains sole teardown ownership after
natural end.

The path-canonicalization Wasm-only patch applies `-ffile-prefix-map` and
`-fmacro-prefix-map` to the actual qtmultimedia source and binary roots. Every
qtmultimedia compile-database command must prove both maps and their canonical
`/qt/qtmultimedia/source` and `/qt/qtmultimedia/build` destinations. This keeps
`__FILE__` data in the bundled Resonance Audio archive independent of vcpkg's
random source suffix and workspace location.

All three patch digests, upstream commit identities, expected vcpkg ABI, and the
canonical path/bytes/SHA-256 set for every qtmultimedia-owned `.a`/`.o` payload
are part of `toolchain-lock.json`. Verification requires:

1. the ABI-named vcpkg binary-cache package and its embedded ABI-input record;
2. exact equality between the package list and every installable cache member,
   plus byte equality between all of those cache members and the installed
   tree;
3. a deterministic non-static payload aggregate, excluding only the two SBOM
   records and ABI record whose generated metadata is separately authenticated;
4. exact ownership of every target `.a`/`.o` by one installed vcpkg package,
   so a renamed stale archive cannot enter the target tree;
5. a source-locked inventory of all 385 target static link inputs, cross-bound
   to the final application's complete linked-archive superset;
6. exact equality between the 222-entry compile database and expanded Ninja
   compiler edges, including the pinned compiler, both prefix-map pairs, and
   exact compiler arguments after allowing only Ninja's depfile bookkeeping;
   the 217 compiled Wasm objects must exactly equal the installed archive/object
   member multiset. Every `.o` digest is also bound to its individual canonical
   source/argument/output edge, so two same-basename objects cannot be swapped
   without changing the locked aggregate. Canonical output directories are
   omitted because CMake legitimately replaces long object subdirectories with
   hashes under the longer qualification root. The five non-installable PCH
   byte streams are deliberately excluded from cross-root byte identity because
   Clang embeds physical-root state in them; their source, arguments, kind,
   basename, magic, count, and expanded-Ninja parity remain mandatory;
7. one canonical `lib/libQt6Multimedia.a` occurrence in the audited final link,
   with the locked size and SHA-256; and
8. a mandatory second package build under source/build roots 72 characters
   longer, reproducing the complete qtmultimedia `.a`/`.o` member set byte for
   byte. Its manifest must equal the primary manifest with only
   `builtin-baseline` removed, it must use the authenticated current ports tree
   with vcpkg versions disabled. The verifier binds both outer and port CMake
   executables, toolchain and chainload files, target and host triplets, ordered
   overlay ports, overlay triplets, package install prefix, and an explicit
   `clear` then exact local `files,...,readwrite` binary-source policy. Its
   buildtrees, packages, installed tree, binary cache, downloads, manifest, and
   outer CMake roots are independently checked and emitted in evidence.

Cross-root equality covers all 16 static package members plus the 348-file
deterministic non-static payload. The complete 367-file package inventory and
cache-to-installed bytes are checked independently in each root. The generated
SPDX record is intentionally not claimed byte-identical across roots because
its UUID/timestamp metadata differs; the authenticated exclusion is explicit.

Static contracts require exactly five queued notifications and no synchronous
device-change callback in the patched view.
Never enable main-thread blocking or move this path to a pthread as a
workaround: the recursive same-thread lock has no possible wake-up.

- [x] **Step 4: Run CfT and branded Chrome focused lanes**

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep '@network|@media'
```

Expected: all three blocking browsers pass. CfT must pass the open VP8/Opus
fixture; Chrome Stable and Chrome Beta supply the two blocking branded-media
results. Within each lane the acceptance scenario waits for post-main readiness,
arms nonce-correlated server logging, completes QNAM and WSS including clean
close, performs the one trusted activation click, pauses after two advancing
frames, and publishes capture readiness only after Qt reports
`PausedState`. The core lane deliberately holds that decoded frame for 2,500
ms—longer than the complete fixture duration—and requires a stable,
non-ended browser element and a nonblank screenshot. The acknowledgement seeks
the exact owned element to 1,000 ms while paused; playback resumes only after
the bounded browser `seeking`/`seeked` proof, and the observed resume position
must remain within 125 ms of that target. Two increasing post-resume frames,
real natural end, backend-owned source removal, and independent
player/audio-output destruction follow before validation of the complete
`probeLogs` slice.
Reordering teardown or server-log validation ahead of the corresponding
terminal browser/C++ event fails.

Before accepting the ordinary lane, run five qtmultimedia-specific variants in
pinned Chromium:

1. normal secure-origin `navigator.mediaDevices`;
2. `navigator.mediaDevices` shadowed as unavailable, proving OpenAL-only
   fallback;
3. deterministic audio-input-first `enumerateDevices()` ordering;
4. a held first enumeration with two rapid synthetic `devicechange` events,
   proving no second browser enumeration begins until the first settles and
   exactly one fresh enumeration then reconciles the latest device list;
5. delayed `enumerateDevices()` long enough for zero-delay Qt wakeups, with a
   fixed number of browser timer turns before resolution, followed by a
   deterministic synthetic `devicechange` on a fully controlled `EventTarget`
   that moves list A to outputless list B including removals. This proves Qt's
   listener/reconciliation path, not physical hardware delivery.

Every variant rejects a blocking warning, `runtime-abort`, repeated enumeration
loop, or terminal failure. The normal lane additionally requires the complete
source/load/play/two-frame/capture/seek/natural-end/backend-removal/destruction
contract. The delayed lane requires exactly one initial enumeration, proving
that queued OpenAL invalidations and the retained browser pump do not re-enter
the initializing backend before its Promise resolves. A deliberately injected
second same-cache `QMediaDevices::audioOutputs()` lookup while the first
`QCachedValue::ensure()` owns its write lock is outside the supported
single-promising-owner contract; the probe does not mislabel that upstream
recursive-lock violation as hot-plug support.
The native observer debounces the queued Qt callbacks into a bounded
`qt-media-device-batch-settled` event carrying cumulative input and output
signal counts. The controlled browser IDs must remain non-default; exactly one
stable unknown OpenAL fallback per direction remains default across the
outputless change. The synthetic dispatch must synchronously start the second
enumeration within 50 ms, and both Qt change-signal counts must advance by
exactly one. The browser then requires two animation frames plus a timer turn
with an unchanged settlement checkpoint. Only after that fence does the
trusted activation construct `QMediaPlayer` and `QAudioOutput`; their single
construction event must select the same default OpenAL output before the source
is set. The complete capture, seek, post-seek progress, natural-end, and
two-object teardown lane therefore exercises a fresh Qt media backend after
the outputless change. The final checkpoint must still equal the
pre-activation settled checkpoint. Exact allowed console tuples have bounded
occurrence counts; the unavailable lane permits at most one expected
`No media devices found` warning.

- [x] **Step 5: Review and commit**

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
- Modify: `tools/wasm-probe/src/Gate1bReport.h`
- Modify: `tools/wasm-probe/src/Gate1bReport.cpp`
- Modify: `tools/wasm-probe/scripts/package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/tests/test_package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Modify: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Modify: `tools/wasm-probe/browser/lib/browser-matrix.mjs`
- Modify: `tools/wasm-probe/browser/playwright.config.mjs`
- Modify: `tools/wasm-probe/browser/server/artifact-manifest.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.test.mjs`
- Modify: `tools/wasm-probe/browser/tests/gate1b.spec.mjs`
- Modify: `tools/wasm-probe/browser/tests/browser-matrix.test.mjs`
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
  forbidden;
- each cycle creates and nonce-proves its Wasm Worker before creating its
  AudioWorklet node;
- C++ cannot begin cycle N+1 until the browser returns the exact
  `ack-audio-cycle-browser-teardown` for cycle N after independent Playwright
  Worker-close, CDP-target-destroyed, and four-pthread-baseline evidence;
- the full 1,000-cycle CfT acceptance runs through a dedicated headed
  `chromium-cft-headed-stress` project using the same executable identity as
  canonical `chromium-cft`.

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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_stamp_worker_bootstraps.py" -v
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_gate1b_source_contract.py" -v
```

Expected: FAIL.

- [ ] **Step 2: Version the generated worker bootstraps**

Configure `gate1b-pre.js.in` with the current input digest. It wraps only the
main-window `Worker` and `AudioWorkletNode` construction/termination points to
assign owned per-resource IDs and maintain resource metrics. It delegates all
native behavior, does not rewrite Emscripten bootstrap messages, and never
synthesizes success. Playwright worker events and CDP target lifecycle events
cross-check the wrapper metrics; none is sufficient alone.

Task 5 extends the fixed command allowlist only with
`ack-audio-cycle-browser-teardown`. Its payload schema is exactly
`{runNonce, cycleId, ownedWorkerResourceId}`, matching values native C++ already
published in `audio-cycle-awaiting-browser-ack`. Playwright Worker and CDP
target IDs are forbidden in this native acknowledgement. The acceptance runner
first validates the raw-trace binding from `ownedWorkerResourceId` to the
matching Playwright Worker close and CDP `Target.targetDestroyed` records, and
only then sends the three-field acknowledgement. Bootstrap rejects an
acknowledgement until C++ has published the matching wait event and rejects
stale, duplicate, skipped, or cross-cycle native identities.

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

1. create a Wasm Worker with that cycle's preallocated region, publish its
   `ownedWorkerResourceId`, bind that resource in the raw runner trace to the
   Playwright Worker and CDP target-created identities, post the cycle nonce
   transform, and prove the matching atomic result;
2. create/connect an AudioWorklet node with a cycle-specific preallocated
   callback slot, attaching a cycle-correlated `processorerror` listener before
   connecting;
3. wait asynchronously for its non-silent callback/checksum;
4. at cycle 500 only, keep this Worker and node live while Step 5 performs and
   acknowledges the growth proof;
5. disconnect the node, wait two render quanta, reject a late callback, remove
   that exact `processorerror` listener, and record node destruction;
6. request termination of the Worker and publish
   `audio-cycle-awaiting-browser-ack` with the native run nonce, cycle ID, and
   `ownedWorkerResourceId`;
7. the acceptance runner observes the matching Playwright Worker close, CDP
   `Target.targetDestroyed`, and browser-observed return to the four-pthread
   baseline, appends those raw records and their binding to
   `ownedWorkerResourceId`, then invokes
   `ack-audio-cycle-browser-teardown` with exactly the three native fields;
8. C++ validates only the exact
   `{runNonce, cycleId, ownedWorkerResourceId}` acknowledgement and then records
   `cycle-completed`; it never consumes or validates Playwright/CDP IDs;
9. require the `$EmAudio` handle count to equal `activeContextBaseline` and all
   page/browser resource baselines before beginning cycle N+1.

No C++/Qt main-thread blocking wait is permitted. Polling in C++ cannot replace
the browser acknowledgement, and an in-page wrapper count cannot replace the
Playwright/CDP observations.

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
5. perform the causally acknowledged round trip from main Wasm to an
   already-live member of the four-pthread pool, to the same cycle-500 Wasm
   Worker created before the node, and finally to the still-live AudioWorklet
   callback;
6. require that Worker's nonce identity, the node's callback sequence, and the
   original ring all advance across the growth;
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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --project chromium-cft --project chrome-stable --project chrome-beta `
  --grep '@audio-negative|@audio-smoke'

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --project chromium-cft-headed-stress --project chrome-stable `
  --project chrome-beta --grep "@audio-1000"
```

Expected in all three blocking browsers:

- cycle IDs exactly `0..999`;
- one memory growth at 500;
- `.aw.js` and `.ww.js` successful requests;
- post-growth sentinel/ring round trip;
- exactly one matching browser teardown acknowledgement per cycle before the
  next `cycle-start`;
- no late callback or worker leak;
- final AudioContext state `closed`;
- both negative classes for each worker type fail with the named reason and no
  callback/resource leak.

- [ ] **Step 7: Review and commit**

The adversarial reviewer checks realtime-safety, atomic ordering, stack/TLS
ownership, callback lifetimes, every cycle record, worker baseline, actual
close, browser acknowledgement causality, worker-first ordering, the
cycle-500-live Worker/node growth proof, dedicated headed CfT execution, and
mismatch behavior.

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
- Create: `tools/wasm-probe/browser/lib/persistent-profile-browser.mjs`
- Create:
  `tools/wasm-probe/browser/tests/persistent-profile-browser.test.mjs`
- Create: `tools/wasm-probe/browser/run-fsa-qualification.mjs`
- Create: `tools/wasm-probe/browser/manual/FSA-QUALIFICATION.md`
- Modify: `tools/wasm-probe/browser/run-browser-tool.mjs`
- Modify: `tools/wasm-probe/browser/lib/browser-matrix.mjs`
- Modify: `tools/wasm-probe/browser/tests/browser-matrix.test.mjs`
- Modify: `tools/wasm-probe/browser/web/bootstrap.mjs`
- Modify: `tools/wasm-probe/browser/server/artifact-manifest.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.mjs`
- Modify: `tools/wasm-probe/browser/server/probe-server.test.mjs`
- Modify: `tools/wasm-probe/scripts/package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/tests/test_package_runtime_artifacts.py`
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.h`
- Modify: `tools/wasm-probe/src/BrowserRuntimeBridge.cpp`
- Modify: `tools/wasm-probe/src/ProbeState.h`
- Modify: `tools/wasm-probe/src/ProbeState.cpp`
- Modify: `tools/wasm-probe/qml/Main.qml`
- Modify: `tools/wasm-probe/CMakeLists.txt`
- Modify: `tools/wasm-probe/browser/tests/gate1b.spec.mjs`
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
explicit cleanup. Persistent-launcher tests require an exact owned profile
directory, the selected lane's audited executable/arguments, a full process
close, a relaunch against the same origin and profile, and rejection of
`storageState`, an incognito `browser.newContext()`, or a substituted profile.

Fake-adapter tests may simulate initial picker and post-reload permission
states, but the headed smoke requires both the initial
`showDirectoryPicker()` prompt and any later `requestPermission()` prompt to be
entered from separate genuine Qt Quick clicks.

- [ ] **Step 2: Implement automated OPFS**

The page path uses `navigator.storage.getDirectory()`. The dedicated module
worker uses `createSyncAccessHandle()` where Chromium exposes it, performs
write/read/flush/close, and returns exact hashes. A persistent Playwright
profile proves reload and full browser-process restart persistence. The
repository-owned persistent launcher uses the lane's exact executable and
`chromium.launchPersistentContext()` with one ignored owned `userDataDir`;
`storageState` is not used as an OPFS backup. Keep the same HTTPS server alive
on the exact same scheme, host, and port; close the entire persistent context
and browser process, relaunch with the same `userDataDir`, audit the effective
arguments again, and fail if the origin or profile identity changes.

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
awaiting anything else. Playwright may produce that trusted click with
`page.mouse.click()` at the fixed QML coordinates, but neither page JavaScript,
`page.evaluate()`, CDP, nor Playwright's file-chooser API may select a directory
or synthesize the returned handle.

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
  recording the transition. The runner must return control to the OS-assisted
  or human operator for any browser permission UI; it cannot pre-grant the
  permission through the browser context;
- if it is `denied`, missing, or a substituted handle, fail.

Persisted permission is not itself assumed or required; persisted handle
identity plus the explicit, browser-compliant reauthorization path is.

Before reload, create a random nonce canary through the selected real handle.
After IndexedDB retrieval and any required reauthorization, re-enumerate and
rehash the known directory entries, read the exact nonce bytes through the
restored handle, remove that canary, and prove its absence. Permission state
alone is not handle-identity evidence.

No fake adapter shares this result field.

- [ ] **Step 4: Run automated storage lanes**

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs node-test

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs playwright-test `
  --grep "@storage"
```

Expected: OPFS passes in all three blocking lanes through their owned
persistent profiles, including worker, reload, full process restart, and
cleanup. Fake-adapter tests pass. FSA remains separately false.

- [ ] **Step 5: Run headed Stable chooser smoke**

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
file input. The only automated page-side action at the chooser boundary is the
trusted click that opens it. Selection, confirmation, and any post-reload
permission UI are OS-level or human actions.

```powershell
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs fsa `
  --build tools/wasm-probe/build/wasm-release `
  --staging tools/wasm-probe/browser/.runs/fsa-smoke `
  --smoke
```

`run-browser-tool.mjs fsa` dispatches exactly
`run-fsa-qualification.mjs` under the authenticated pinned Node process and
forwards an argument array without a shell. This Task 6 output is an ignored
smoke artifact only: even a successful smoke cannot set
`gate1bTechnicalPassed` and cannot be reused by Task 7 after the qualifier
source commit changes the build ID.

- [ ] **Step 6: Review and commit**

Review evidence-class separation, user-activation timing, chooser authenticity,
profile isolation, permission semantics, cleanup, and path privacy.

Commit:

```text
feat: qualify browser storage capabilities
```

## Task 7: Generate fail-closed Gate 1B evidence across blocking browsers

**Files:**

- Create:
  `tools/wasm-probe/browser/contracts/build-audit.schema.json`
- Create:
  `tools/wasm-probe/browser/contracts/lane-record.schema.json`
- Create:
  `tools/wasm-probe/browser/contracts/trace-record.schema.json`
- Create:
  `tools/wasm-probe/browser/contracts/fsa-record.schema.json`
- Create:
  `tools/wasm-probe/browser/contracts/final-evidence.schema.json`
- Create: `tools/wasm-probe/browser/lib/trace-writer.mjs`
- Create: `tools/wasm-probe/browser/tests/trace-writer.test.mjs`
- Create: `tools/wasm-probe/browser/lib/acceptance-runner.mjs`
- Modify: `tools/wasm-probe/browser/tests/gate1b.spec.mjs`
- Create: `tools/wasm-probe/browser/run-gate1b.mjs`
- Modify: `tools/wasm-probe/browser/run-browser-tool.mjs`
- Create: `tools/wasm-probe/tests/verify_runtime_evidence.py`
- Create: `tools/wasm-probe/tests/test_verify_runtime_evidence.py`
- Create: `tools/wasm-probe/scripts/finalize_runtime_evidence.py`
- Create: `tools/wasm-probe/tests/test_finalize_runtime_evidence.py`
- Modify: `tools/wasm-probe/tests/verify_build.py`
- Modify: `tools/wasm-probe/tests/test_verify_build.py`
- Modify: `tools/wasm-probe/tests/test_gate1b_source_contract.py`
- Modify: `tools/wasm-probe/input-manifest.txt`
- Modify: `tools/wasm-probe/build-control-manifest.txt`
- Modify: `.gitattributes`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b.json`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b-fsa.json`
- Create only during validated finalization:
  `docs/superpowers/evidence/emscripten-gate-1b-fsa.png`

The evidence JSON, NDJSON traces, FSA JSON, and FSA screenshot are outputs, not
probe inputs. They must not enter `input-manifest.txt` or the runtime build ID.
The dispatcher, runner, trace writer, validator/finalizer, five schemas,
focused tests, and scoped `.gitattributes` rules do enter the source manifest
and therefore the source-bound build ID.

- [ ] **Step 1: Freeze schemas, serialization, and failing validator tests**

All five files use JSON Schema 2020-12, `schemaVersion: 1`, required fields, and
`additionalProperties: false` at every object boundary:

- `build-audit.schema.json` owns the authenticated compile/link/artifact audit;
- `lane-record.schema.json` owns one canonical browser identity, its run
  outcomes, build audit reference, and full-trace path/hash/count;
- `trace-record.schema.json` owns each raw NDJSON record;
- `fsa-record.schema.json` owns the headed chooser/operator/browser/build
  record and sanitized screenshot hash;
- `final-evidence.schema.json` owns immutable Gate 1A lineage, the three lane
  records, FSA reference, deterministic controls, digest invariant, and the five
  authority fields.

The trace schema requires `ingestSequence`, `producer`,
`producerSequence`, `runId`, `lane`, `eventType`, source-local
`monotonicMicroseconds`, explicit causal `ids`, `payload`, `previousHash`, and
`recordHash`. The `ids` object permits only the declared identifiers:
`runNonce`, `requestId`, `connectionId`, `cycleId`, `nodeId`,
`ownedWorkerResourceId`, `playwrightWorkerId`, `cdpTargetId`, `pumpSerial`, and
`storageOperationId`. Relevant events must carry every applicable ID. The
event payload is a discriminated `oneOf` keyed by `eventType`, never an
open-ended property bag. The writer emits canonical one-record-per-line UTF-8
JSON with LF and a hash chain; the record hash covers the canonical record
without its own `recordHash`.

Add these exact scoped attributes:

```gitattributes
tools/wasm-probe/browser/contracts/*.schema.json text eol=lf
docs/superpowers/evidence/emscripten-gate-1b.json text eol=lf
docs/superpowers/evidence/emscripten-gate-1b-*-cycles.ndjson text eol=lf
docs/superpowers/evidence/emscripten-gate-1b-fsa.json text eol=lf
docs/superpowers/evidence/emscripten-gate-1b-fsa.png binary
```

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
- failure of the exact
  `gitBlobInputDigest == cleanWorktreeInputDigest == packagedBuildId`
  invariant;
- `gate1bTechnicalPassed: true` while any required result is false;
- any authority field other than `gate1bTechnicalPassed` set true.

Unit fixtures separately remove, duplicate, reorder, truncate, or alter every
record class and hash-chain link. Run the validators against deliberately
incomplete fixtures first and require failure.

- [ ] **Step 2: Implement source binding and the Gate 1B build audit**

First extend `verify_build.py` with an explicit `gate1b-build-audit` mode and
the frozen `build-audit.schema.json`, with failing unit fixtures before
implementation. That mode writes only to a caller-supplied ignored staging
path, emits `"gate": "1B-build-audit"`, and has no field capable of asserting
Gate 1A PASS. Its tests prove it cannot overwrite, relabel, or mutate
`docs/superpowers/evidence/emscripten-gate-1a.json`.

The historical Gate 1A mode and its exact default-shell deployment set
(including `qtlogo.svg`) stay frozen. The Gate 1B mode instead accepts only the
content-addressed runtime set and requires `qtlogo.svg` to be absent. Do not run
the old Gate 1A deployment-set check against Gate 1B output.

The source-binding implementation reads `input-manifest.txt` from the recorded
`sourceCommit`, requires the manifest and every named input to be regular Git
blobs at that commit, hashes those blob bytes with the same canonical
path/hash/payload algorithm as CMake, and separately hashes the current clean
worktree bytes. The only accepted invariant is:

```text
gitBlobInputDigest == cleanWorktreeInputDigest == packagedBuildId
```

`sourceCommit` identifies the blobs used for that equality. It never identifies
the later evidence-only commit, the index, or merely the current branch name.
Dirty, missing, type-changed, or untracked runtime inputs fail before configure.
Ignored staging, profiles, and traces remain outside the input set.

The Gate 1B build audit records:

- source commit plus all three exact digest fields;
- immutable Gate 1A parent evidence hash/commit;
- every current C/C++ compile command with `-pthread` and native Wasm
  exceptions;
- exact final JSPI/AudioWorklet/WasmWorkers/pthread-pool/no-blocking/growth
  flags;
- absence of forbidden fallback settings;
- current HTML/JS/Wasm/aw/ww/Qt loader/media/server/test/package-lock hashes;
- artifact-manifest agreement.

Focused verifier tests cover both frozen Gate 1A fixtures and the new Gate 1B
fixture before any current build is accepted. Internal Emscripten JSPI
implementation identifiers are not treated as legacy Asyncify flags; the actual
build command is the authority. Literal `-sASYNCIFY`, negative Wasm exceptions,
or legacy JS exception settings fail.

- [ ] **Step 3: Implement the full-trace writer and one shared acceptance runner**

Both Playwright tests and `run-gate1b.mjs` call the same exported scenario.
The scenario:

1. launches one lane with no retries;
2. starts the exact server/build;
3. captures browser identity/arguments;
4. attaches every raw diagnostic source before navigation;
5. runs preflight and core;
6. arms nonce-correlated server logs, completes QNAM and WSS, performs the one
   trusted click, captures media frames/screenshot/natural end, then runs the
   exact audio/Worker/growth cycles;
7. runs automated OPFS persistence/cleanup and final teardown;
8. runs each negative server mode in a fresh context;
9. closes context/browser/server in `finally`;
10. returns a strict `lane-record.schema.json` value plus its complete raw
    trace.

Each lane trace includes, without filtering or summary substitution:

- every bootstrap/C++ report event, command acknowledgement, snapshot, pump,
  input/timer, Page Visibility, pagehide/pageshow, and BFCache record;
- browser console, page error, failed request, CSP violation, unhandled
  rejection, and abort/exit diagnostics, including an explicitly empty clean
  stream;
- every browser request/response and append-only server `probeLogs` record;
- WSS upgrade, message, heartbeat, error, and close records;
- wrapper resource events, AudioWorklet callback/checksum/`processorerror`,
  Playwright Worker events, and CDP target-created/destroyed events;
- every cycle, growth, storage, resource-baseline, and teardown record.

`ingestSequence` records only the runner's receipt order.
`monotonicMicroseconds` and producer sequences are source-local diagnostics.
No acceptance edge may be inferred by comparing clocks from C++, the page,
Node, Playwright, CDP, a Worker, or an AudioWorklet. QNAM/WSS/media
corroboration, cycle teardown, and growth are joined only through the explicit
IDs and acknowledgement events frozen above. The Python validator replays these
state machines from the raw records rather than trusting lane summary booleans.

There is no second permissive “evidence-only” path.
`run-browser-tool.mjs qualify` dispatches exactly `run-gate1b.mjs` under the
authenticated pinned Node process, forwards only an argument array without a
shell, and refuses an output path outside the ignored `.runs` root. Dispatcher
tests lock that target and prove `qualify` cannot write directly to
`docs/superpowers/evidence`.

- [ ] **Step 4: Commit the qualifier before producing evidence**

Run the fast Python/Node contract tests, review the runner/validator diff, and
commit all Task 7 source files, schemas, dispatcher changes, manifests, and
`.gitattributes` rules before qualification:

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

- [ ] **Step 5: Produce a fresh source-bound build**

> **Deferred build-root parameterization:** the current qualification wrapper,
> verifier, and browser runtime intentionally accept only
> `tools/wasm-probe/build/wasm-release`. The commit-named commands below remain
> a design target and must not be run until all three consumers derive and lock
> the same requested build root. Until then, rejecting a different build root is
> required; authenticating the default graph while building another graph is
> forbidden.

```powershell
$gate1bSourceCommit = (git rev-parse HEAD).Trim()
$gate1bBuildDir = "tools/wasm-probe/build/gate1b-$($gate1bSourceCommit.Substring(0, 12))"
if (Test-Path -LiteralPath $gate1bBuildDir) {
    throw "Fresh Gate 1B build directory already exists"
}

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_*.py" -v

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --preset wasm-release -S tools/wasm-probe -B $gate1bBuildDir
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build $gate1bBuildDir --verbose
```

Do not reuse the Task 3–6 build directory. Run the Gate 1B build audit against
this fresh directory, then require the exact digest invariant before launching
a browser.

- [ ] **Step 6: Run the three-lane matrix into ignored staging**

```powershell
$gate1bSourceCommit = (git rev-parse HEAD).Trim()
$gate1bBuildDir = "tools/wasm-probe/build/gate1b-$($gate1bSourceCommit.Substring(0, 12))"
$gate1bRunRoot = "tools/wasm-probe/browser/.runs/$gate1bSourceCommit"
if (Test-Path -LiteralPath $gate1bRunRoot) {
    throw "Gate 1B staging directory already exists"
}

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs qualify `
  --source-commit $gate1bSourceCommit `
  --build $gate1bBuildDir `
  --staging $gate1bRunRoot
```

The staged qualification runs the canonical headless CfT fast/negative cases,
the headed `chromium-cft-headed-stress` 1,000-cycle case, and complete headed
Stable/Beta lanes. It produces exactly three lane records and three full raw
NDJSON traces, including all header/MIME/CSP/artifact/worker-generation
mismatch negatives. It writes no committed evidence path and cannot set
`gate1bTechnicalPassed: true`.

- [ ] **Step 7: Rerun the blocking headed FSA qualification**

Discard Task 6 smoke as stale. Using the exact same `sourceCommit`, packaged
build ID, fresh build directory, and qualified Stable executable identity, run:

```powershell
$gate1bSourceCommit = (git rev-parse HEAD).Trim()
$gate1bBuildDir = "tools/wasm-probe/build/gate1b-$($gate1bSourceCommit.Substring(0, 12))"
$gate1bRunRoot = "tools/wasm-probe/browser/.runs/$gate1bSourceCommit"

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs fsa `
  --source-commit $gate1bSourceCommit `
  --build $gate1bBuildDir `
  --staging $gate1bRunRoot
```

The initial picker and any post-reload permission prompt each require their own
genuine QML click and OS-assisted or human UI completion. The run creates a
fresh staged FSA JSON and sanitized PNG. A reused Task 6 record, mismatched
build ID/browser identity, fake handle, automated page-level selection, or
missing screenshot is terminal.

- [ ] **Step 8: Run deterministic controls before final PASS**

Run the exact build again. Require only the authenticated `VerifyGlobs.cmake`
check and `ninja: no work to do`. Regenerate the runtime artifact manifest from
unchanged bytes into two fresh staging subdirectories and require byte equality
for the complete packaged trees, not only equal manifest JSON. Rerun the build
audit and require byte equality with the staged original. Recompute the Git
blob, clean-worktree, and packaged digests and require the exact invariant
again. Runtime timestamps/browser versions belong only in runtime evidence and
do not contaminate build outputs.

```powershell
$gate1bSourceCommit = (git rev-parse HEAD).Trim()
$gate1bBuildDir = "tools/wasm-probe/build/gate1b-$($gate1bSourceCommit.Substring(0, 12))"
$gate1bRunRoot = "tools/wasm-probe/browser/.runs/$gate1bSourceCommit"

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- cmake `
  --build $gate1bBuildDir --verbose

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- node `
  tools/wasm-probe/browser/run-browser-tool.mjs qualify `
  --source-commit $gate1bSourceCommit `
  --build $gate1bBuildDir `
  --staging $gate1bRunRoot `
  --deterministic-controls-only
```

The controls are staged schema-valid records. Failure leaves all final evidence
paths absent and `gate1bTechnicalPassed` false.

- [ ] **Step 9: Validate and atomically finalize the evidence**

`finalize_runtime_evidence.py` first validates the complete staging tree,
replays all three raw trace state machines, checks the FSA record/screenshot,
checks deterministic controls, and recomputes every hash and digest without
writing a destination. Only after that read-only preflight passes may it install
the three NDJSON traces, sanitized PNG, and FSA JSON using same-directory
temporary files plus atomic replacement. It writes
`emscripten-gate-1b.json` last as the completion marker. A crash or failure
before that last replacement cannot leave an aggregate PASS.

```powershell
$gate1bSourceCommit = (git rev-parse HEAD).Trim()
$gate1bRunRoot = "tools/wasm-probe/browser/.runs/$gate1bSourceCommit"

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python tools/wasm-probe/scripts/finalize_runtime_evidence.py `
  --staging $gate1bRunRoot `
  --destination docs/superpowers/evidence `
  --source-commit $gate1bSourceCommit
```

This finalizer is the only code path allowed to set
`gate1bTechnicalPassed: true`, and only when all technical results, the blocking
fresh FSA run, and deterministic controls pass. The other four authority fields
remain false.

- [ ] **Step 10: Verify finalized bytes read-only and adversarially review**

Run the validator in read-only mode:

```powershell
$gate1bFinalEvidence = Get-Content -Raw `
  docs/superpowers/evidence/emscripten-gate-1b.json | ConvertFrom-Json
$gate1bSourceCommit = $gate1bFinalEvidence.sourceCommit

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python tools/wasm-probe/tests/verify_runtime_evidence.py `
  --evidence docs/superpowers/evidence/emscripten-gate-1b.json `
  --fsa-evidence docs/superpowers/evidence/emscripten-gate-1b-fsa.json `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson `
  --require-source-commit $gate1bSourceCommit `
  --read-only
```

An independent adversarial reviewer attempts to falsify every result and
checks every raw diagnostic, request/response, server, WSS, Worker, CDP,
worklet, cycle, growth, lifecycle, storage, and teardown stream rather than
trusting aggregate booleans. Any correction to a runtime-affecting or qualifier
source file invalidates the fresh build, all staged evidence, and FSA record;
remove the aggregate completion marker before making any PASS claim, then
return to Step 4 with a new qualifier commit. No user-facing technical PASS is
reported before Step 11's committed-blob verification succeeds.

- [ ] **Step 11: Commit evidence only, then verify the committed blobs**

Confirm the diff contains exactly the six finalized evidence files, then
commit:

```text
test: qualify Gate 1B Chromium runtime
```

The evidence commit is not `sourceCommit`. With a clean worktree, rerun the
read-only validator in committed-blob mode:

```powershell
$gate1bFinalEvidence = Get-Content -Raw `
  docs/superpowers/evidence/emscripten-gate-1b.json | ConvertFrom-Json
$gate1bSourceCommit = $gate1bFinalEvidence.sourceCommit

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python tools/wasm-probe/tests/verify_runtime_evidence.py `
  --evidence docs/superpowers/evidence/emscripten-gate-1b.json `
  --fsa-evidence docs/superpowers/evidence/emscripten-gate-1b-fsa.json `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chromium-cft-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-stable-cycles.ndjson `
  --trace docs/superpowers/evidence/emscripten-gate-1b-chrome-beta-cycles.ndjson `
  --require-source-commit $gate1bSourceCommit `
  --require-evidence-commit HEAD `
  --read-only
```

This final check reads and hashes the committed Git blobs, confirms their
`.gitattributes` serialization, and reasserts that the aggregate names the
earlier qualifier source commit.

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
pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
  python -m unittest discover -s tools/wasm-probe/tests `
  -p "test_*.py" -v

pwsh -NoProfile -File tools/wasm-probe/scripts/Invoke-WithToolchains.ps1 -- `
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
- [ ] every QNAM/WSS/media result joins its append-only server record through
      explicit run nonce and transport IDs;
- [ ] generated/versioned `.aw.js` and `.ww.js` paths execute;
- [ ] byte-corruption and coherent mixed-generation lanes for both worker types
      fail closed before callback use;
- [ ] headed CfT stress plus headed Stable and Beta complete exact 1,000-cycle
      traces;
- [ ] every cycle is Worker-first and receives exactly one browser teardown
      acknowledgement before the next cycle;
- [ ] live shared-memory growth reaches above-old-boundary data through every
      required execution environment;
- [ ] no node/worker/callback leak and actual AudioContext close is proven;
- [ ] OPFS page/worker/reload/persistent-profile process persistence and cleanup
      pass;
- [ ] fresh post-qualifier real headed File System Access chooser evidence
      passes with genuine initial and post-reload prompt clicks;
- [ ] all negative header/MIME/CSP/artifact lanes fail for the expected reason;
- [ ] console/page/network/CSP/worklet errors are clean;
- [ ] strict schemas validate complete raw full-stream traces and their hash
      chains without cross-clock ordering inference;
- [ ] `gitBlobInputDigest == cleanWorktreeInputDigest == packagedBuildId`;
- [ ] no-op build, two-tree repackage equality, repeated build audit, and all
      deterministic controls pass before the final PASS aggregate is written;
- [ ] finalized evidence is written aggregate-last, read-only verified,
      committed evidence-only, and verified again from committed blobs;
- [ ] evidence validator and independent adversarial review pass;
- [ ] `src`, `RhythmGameQml`, and native root manifest remain untouched;
- [ ] Gate 0/formal Gate 1/production authorization fields remain false.
