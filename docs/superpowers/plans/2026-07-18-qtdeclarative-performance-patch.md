# Patched qtdeclarative vcpkg Overlay Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make every vcpkg-backed RhythmGame build install Qt Declarative 6.11.1 with upstream performance commit 24205cda961ca1138e60e715c5334e344a8e9373 applied, including fresh GitHub Actions runners.

**Architecture:** Restore the Qt dependency declarations that the current branch removed from the root manifest, then shadow the pinned built-in qtdeclarative port with a complete common overlay port. The overlay keeps the built-in 6.11.1 source metadata and recipe, adds a local port revision, and applies a vendored patch through qt_install_submodule so vcpkg ABI tracking and CI cache hashing include it.

**Tech Stack:** vcpkg manifest mode, vcpkg overlay ports, CMake presets, PowerShell, GitHub Actions.

## Global Constraints

- Keep builtin-baseline at a0400024711b283056538ac19ced80b91a83c24c.
- Keep Qt Declarative at 6.11.1; do not upgrade Qt or the vcpkg baseline.
- Apply the patch through the common vcpkgOverlayPorts directory on every platform.
- Vendor C:/Users/PC/Downloads/24205cd.diff unchanged; its SHA-256 must be 2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50.
- Patch drift must fail vcpkg configuration instead of silently building unpatched Qt.
- Do not modify RhythmGame application code or introduce a Qt fork.
- Preserve all unrelated tracked and untracked worktree changes.

---

### Task 1: Restore the Qt manifest dependency graph

**Files:**
- Modify: vcpkg.json

**Interfaces:**
- Consumes: the dependency graph from vcpkg.json immediately before commit 2d4ab381ac4bd7e9013d0abfba146ac45f963556
- Produces: fresh manifest installs that include qtdeclarative, its required Qt modules, deployment tooling, and platform backends

- [ ] **Step 1: Demonstrate the current manifest regression**

Run:

~~~powershell
$manifest = Get-Content -Raw vcpkg.json | ConvertFrom-Json
$names = @($manifest.dependencies | ForEach-Object {
    if ($_ -is [string]) { $_ } else { $_.name }
})
if ('qtdeclarative' -notin $names) {
    throw 'qtdeclarative is absent from the current manifest'
}
~~~

Expected: the command fails with qtdeclarative is absent from the current manifest.

- [ ] **Step 2: Restore the removed Qt dependencies**

Restore these exact entries from 2d4ab381a^:vcpkg.json without changing the existing non-Qt dependencies:

~~~json
{
  "name": "qtbase",
  "default-features": false,
  "features": [
    "dnslookup",
    "harfbuzz",
    "jpeg",
    "opengl",
    "openssl",
    "vulkan"
  ]
},
{
  "name": "vulkan-loader",
  "features": [
    "wayland",
    "xcb",
    "xlib"
  ],
  "platform": "linux"
},
{
  "name": "qtbase",
  "default-features": false,
  "features": [
    "xcb",
    "xcb-xlib",
    "xrender",
    "egl",
    "dbus"
  ],
  "platform": "linux"
},
{
  "name": "qtbase",
  "default-features": false,
  "features": [
    "windeployqt"
  ],
  "platform": "windows"
},
"qtdeclarative",
"qtwebsockets",
"qtshadertools",
"qtsvg",
"qtinterfaceframework",
"qttranslations",
{
  "name": "qttools",
  "features": [
    "linguist",
    "qml"
  ]
},
"qtkeychain-qt6"
~~~

Restore these entries immediately after libxml2:

~~~json
{
  "name": "qtmultimedia",
  "default-features": false,
  "features": [
    "qml",
    "ffmpeg"
  ]
},
{
  "name": "qtwayland",
  "platform": "linux"
}
~~~

- [ ] **Step 3: Format and validate the manifest**

Run:

~~~powershell
& "$env:VCPKG_ROOT/vcpkg.exe" format-manifest vcpkg.json
$manifest = Get-Content -Raw vcpkg.json | ConvertFrom-Json
$names = @($manifest.dependencies | ForEach-Object {
    if ($_ -is [string]) { $_ } else { $_.name }
})
if ('qtdeclarative' -notin $names) { throw 'qtdeclarative was not restored' }
if ('qtwebsockets' -notin $names) { throw 'qtwebsockets was not restored' }
if ('qtmultimedia' -notin $names) { throw 'qtmultimedia was not restored' }
~~~

Expected: vcpkg reports the manifest is formatted and the assertions exit successfully.

- [ ] **Step 4: Commit the restored dependency graph**

Run:

~~~powershell
git add -- vcpkg.json
git diff --cached --check
git commit -m "Restore Qt dependencies to vcpkg manifest"
~~~

Expected: the commit contains only vcpkg.json and leaves unrelated worktree files untouched.

### Task 2: Add the patched qtdeclarative overlay port

**Files:**
- Create: vcpkgOverlayPorts/qtdeclarative/port.data.cmake
- Create: vcpkgOverlayPorts/qtdeclarative/portfile.cmake
- Create: vcpkgOverlayPorts/qtdeclarative/vcpkg.json
- Create: vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch
- Create: .gitattributes

**Interfaces:**
- Consumes: qtdeclarative 6.11.1 from builtin-baseline a0400024711b283056538ac19ced80b91a83c24c and source patch SHA-256 2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50
- Produces: overlay port qtdeclarative 6.11.1#1 with the QQuickWindow stacking optimization applied for target and host triplets

- [ ] **Step 1: Demonstrate that no qtdeclarative overlay exists**

Run:

~~~powershell
if (-not (Test-Path vcpkgOverlayPorts/qtdeclarative/portfile.cmake)) {
    throw 'qtdeclarative still resolves from the built-in registry'
}
~~~

Expected: the command fails with qtdeclarative still resolves from the built-in registry.

- [ ] **Step 2: Add the exact source metadata**

Create port.data.cmake with:

~~~cmake
set(qtdeclarative_HASH "91d657a33b3bf8fc79f1116561f125b3905b590e94504dcb3e4feead71f209b35b42a9da783d4568f756b22fd7a0e9802e1a9039553d400fe33417562776a1d6")
set(qtdeclarative_URL "https://download.qt.io/archive/qt/6.11/6.11.1/submodules/qtdeclarative-everywhere-src-6.11.1.tar.xz;https://mirrors.ocf.berkeley.edu/qt/archive/qt/6.11/6.11.1/submodules/qtdeclarative-everywhere-src-6.11.1.tar.xz")
set(qtdeclarative_FILENAME "qtdeclarative-everywhere-src-6.11.1.tar.xz")
~~~

- [ ] **Step 3: Add the patched port recipe**

Copy the pinned built-in 6.11.1 portfile from .vcpkg-buildtrees/versioning_/versions/qtdeclarative/846c872082b8bf0c50d13dc8ead681ae6fc6280a/portfile.cmake and replace its empty patch declaration with:

~~~cmake
set(${PORT}_PATCHES
    24205cd-qquickwindow-child-window-stacking.patch
)
~~~

Keep qt_install_submodule passing PATCHES ${${PORT}_PATCHES}; do not change its tool list or configure options.

- [ ] **Step 4: Add the overlay manifest**

Copy the pinned built-in 6.11.1 vcpkg.json and add this property immediately after version:

~~~json
"port-version": 1
~~~

Keep the built-in qtbase, host qtdeclarative, qtlanguageserver, qtshadertools, and qtsvg dependencies unchanged.

- [ ] **Step 5: Vendor and verify the patch**

Create 24205cd-qquickwindow-child-window-stacking.patch with the exact bytes from C:/Users/PC/Downloads/24205cd.diff. Add this path-specific rule to `.gitattributes` so a Windows checkout with `core.autocrlf=true` cannot rewrite those bytes:

~~~gitattributes
vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch -text
~~~

Then run:

~~~powershell
$expected = '2A015242AF462BE117A2924D4D8DB2C753B29891921E714C23BF1AB4355C4C50'
$actual = (Get-FileHash -Algorithm SHA256 vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch).Hash
if ($actual -ne $expected) {
    throw "Vendored patch hash mismatch: $actual"
}
$textAttribute = git check-attr text -- vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch
if (-not $textAttribute.EndsWith('text: unset')) {
    throw "Vendored patch is still subject to line-ending conversion: $textAttribute"
}
~~~

Expected: both assertions exit successfully, including when Git is configured with `core.autocrlf=true`.

- [ ] **Step 6: Validate overlay structure and formatting**

Run:

~~~powershell
& "$env:VCPKG_ROOT/vcpkg.exe" format-manifest vcpkgOverlayPorts/qtdeclarative/vcpkg.json
$port = Get-Content -Raw vcpkgOverlayPorts/qtdeclarative/portfile.cmake
if (-not $port.Contains('24205cd-qquickwindow-child-window-stacking.patch')) {
    throw 'Port recipe does not apply the vendored patch'
}
if (-not $port.Contains('qt_install_submodule')) {
    throw 'Port recipe no longer uses the Qt submodule helper'
}
git diff --check -- vcpkg.json vcpkgOverlayPorts/qtdeclarative ':(exclude)vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch'
~~~

Expected: formatting succeeds, both assertions pass, and git diff --check emits no output. The byte-identical unified diff is excluded from this outer whitespace scan because its required context-line prefixes are checked by the SHA-256 assertion instead.

- [ ] **Step 7: Commit the overlay port**

Run:

~~~powershell
git add -- .gitattributes vcpkgOverlayPorts/qtdeclarative
git diff --cached --check -- . ':(exclude)vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch'
git commit -m "Patch qtdeclarative vcpkg port"
~~~

Expected: the commit contains only the root attribute rule and four overlay-port files, and leaves unrelated worktree files untouched.

### Task 3: Prove vcpkg and CI consume the patched port

**Files:**
- Verify: CMakePresets.json
- Verify: .github/workflows/ci.yml
- Generated: .vcpkg-buildtrees/qtdeclarative/*
- Generated: build/vcpkg_installed/*

**Interfaces:**
- Consumes: qtdeclarative 6.11.1#1 overlay from Task 2 and the restored root manifest from Task 1
- Produces: a successful CI-equivalent Windows configure/build/test using the patched Qt package, plus static proof that every CI preset and dependency cache includes the common overlay

- [ ] **Step 1: Assert CI preset and cache routing**

Run:

~~~powershell
$presets = Get-Content -Raw CMakePresets.json
$workflow = Get-Content -Raw .github/workflows/ci.yml
foreach ($preset in 'ci-coverage', 'ci-sanitize', 'ci-macos', 'ci-ubuntu', 'ci-windows') {
    if (-not $presets.Contains('"' + $preset + '"')) {
        throw "Missing CI preset: $preset"
    }
}
if (-not $presets.Contains('"${sourceDir}/vcpkgOverlayPorts"')) {
    throw 'Common vcpkg overlay is not configured'
}
if (-not $workflow.Contains("'vcpkgOverlayPorts*/**'")) {
    throw 'CI cache key does not hash overlay contents'
}
~~~

Expected: every assertion exits successfully.

- [ ] **Step 2: Configure exactly as Windows CI does**

Run:

~~~powershell
cmake --preset=ci-windows
~~~

Expected: vcpkg installs qtdeclarative 6.11.1#1 from vcpkgOverlayPorts/qtdeclarative, source extraction applies 24205cd-qquickwindow-child-window-stacking.patch without rejects, and CMake configuration completes.

- [ ] **Step 3: Verify vcpkg ABI evidence includes the patch**

Run:

~~~powershell
$abi = Get-Content -Raw .vcpkg-buildtrees/qtdeclarative/w-sqt.vcpkg_abi_info.txt
if (-not $abi.Contains('24205cd-qquickwindow-child-window-stacking.patch')) {
    throw 'qtdeclarative ABI inputs do not include the performance patch'
}
if (-not $abi.Contains('triplet w-sqt')) {
    throw 'qtdeclarative was not built for the Windows CI triplet'
}
~~~

Expected: both assertions exit successfully.

- [ ] **Step 4: Build and test with CI-equivalent commands**

Run:

~~~powershell
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure -j 2
~~~

Expected: the Release build succeeds and every test passes.

### Task 4: Commit, push, and monitor CI

**Files:**
- Publish: the reviewed documentation, manifest-restoration, and overlay-port commits

**Interfaces:**
- Consumes: verified implementation from Tasks 1-3
- Produces: pushed branch codex/online-arena and a successful GitHub Actions run for the branch or its open pull request

- [ ] **Step 1: Review the final scoped diff**

Run:

~~~powershell
$implementationBase = '49756400297485bb2ddcc123fe482b8103fb61fd'
$implementationHead = git rev-parse HEAD
git status --short
git diff "$implementationBase..$implementationHead" --check -- . ':(exclude)vcpkgOverlayPorts/qtdeclarative/24205cd-qquickwindow-child-window-stacking.patch'
git diff "$implementationBase..$implementationHead" -- .gitattributes vcpkg.json vcpkgOverlayPorts/qtdeclarative docs/superpowers/specs/2026-07-18-qtdeclarative-performance-patch-design.md docs/superpowers/plans/2026-07-18-qtdeclarative-performance-patch.md
~~~

Expected: the committed implementation diff shows the documentation, line-ending guard, qtdeclarative manifest, and overlay changes; the committed-range whitespace check passes; unrelated dirty files remain untouched.

- [ ] **Step 2: Confirm the implementation commits and clean index**

Run:

~~~powershell
$implementationBase = '49756400297485bb2ddcc123fe482b8103fb61fd'
git log --oneline "$implementationBase..HEAD" -- .gitattributes vcpkg.json vcpkgOverlayPorts/qtdeclarative docs/superpowers/specs/2026-07-18-qtdeclarative-performance-patch-design.md docs/superpowers/plans/2026-07-18-qtdeclarative-performance-patch.md
git diff --cached --quiet
if ($LASTEXITCODE -eq 0) {
    'INDEX_CLEAN=True'
} elseif ($LASTEXITCODE -eq 1) {
    throw 'Unexpected staged changes remain before push'
} else {
    throw "Unable to inspect the index; git diff exited $LASTEXITCODE"
}
~~~

Expected: the path-scoped log shows the design, plan, manifest-restoration, overlay-port, and review-fix commits, and `INDEX_CLEAN=True` is printed.

- [ ] **Step 3: Push the current branch**

Run:

~~~powershell
git push origin codex/online-arena
~~~

Expected: origin/codex/online-arena advances to the implementation commit.

- [ ] **Step 4: Monitor GitHub Actions**

Find the workflow run associated with the pushed commit or its open pull request and monitor it until all required jobs complete.

Expected: lint, test, coverage, sanitizer, and deploy jobs finish successfully. If a job fails, inspect its exact log, implement the smallest in-scope correction, re-run local verification, commit, push, and monitor the replacement run.
