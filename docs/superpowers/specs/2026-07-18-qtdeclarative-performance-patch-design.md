# Patched qtdeclarative vcpkg Overlay Design

**Status:** Approved on 2026-07-18

## Context

RhythmGame's pinned vcpkg baseline, `a0400024711b283056538ac19ced80b91a83c24c`, resolves `qtdeclarative` 6.11.1. The upstream Qt commit `24205cda961ca1138e60e715c5334e344a8e9373` avoids full Qt Quick item-tree scans when child-window stacking does not need to be updated, which substantially improves RhythmGame performance.

The repository already routes every vcpkg-backed local and CI configure preset through `vcpkgOverlayPorts`. Windows CI appends `vcpkgOverlayPortsWindows` after the common overlay. CI cache keys already hash both overlay directory trees.

The supplied patch was checked against the exact Qt 6.11.1 archive and SHA-512 hash referenced by the pinned vcpkg port. It applies cleanly without modification.

## Goals

- Build `qtdeclarative` with the performance patch in every vcpkg-backed configuration.
- Keep local development, Ubuntu CI, and Windows CI on the same patched source.
- Make patch application deterministic and fail the configure step if it stops applying.
- Invalidate CI dependency caches whenever the patched port or patch changes.

## Non-goals

- Upgrade Qt or the vcpkg baseline.
- Maintain a separate Qt fork.
- Modify RhythmGame application code.
- Add a second dependency bootstrap path in CI.

## Selected Approach

Add a complete `qtdeclarative` overlay port under `vcpkgOverlayPorts/qtdeclarative`. Base its metadata and build recipe on the exact built-in 6.11.1 port selected by the repository baseline, add a port revision for the local customization, vendor the upstream patch beside the port, and list it in the port's `PATCHES` collection passed to `qt_install_submodule`.

Add a path-specific `.gitattributes` rule with `-text` for the vendored patch. This keeps the supplied bytes stable on Windows checkouts even when `core.autocrlf=true`, so the recorded patch hash and vcpkg ABI input do not depend on the developer or runner's Git configuration.

The overlay directory will contain:

- `port.data.cmake`: the exact Qt 6.11.1 archive URLs, filename, and SHA-512 hash.
- `portfile.cmake`: the built-in recipe with the vendored patch added to `${PORT}_PATCHES`.
- `vcpkg.json`: the built-in dependency metadata with a local port revision.
- `24205cd-qquickwindow-child-window-stacking.patch`: the supplied upstream commit in patch form.

This approach follows the repository's existing vcpkg extension seam. A Qt fork would add source-hosting and synchronization work, while mutating downloaded sources in CI scripts would make CI behavior differ from ordinary local builds and would bypass vcpkg's package ABI tracking.

## Build and CI Flow

1. CMake loads a local or CI preset that inherits the hidden `vcpkg` preset.
2. The preset supplies `${sourceDir}/vcpkgOverlayPorts` through `VCPKG_OVERLAY_PORTS`.
3. vcpkg resolves `qtdeclarative` from the common overlay before the built-in registry.
4. `qt_install_submodule` downloads the pinned Qt 6.11.1 archive, verifies its existing SHA-512, and applies the vendored patch before configuration.
5. The patched package receives a distinct vcpkg ABI because the overlay recipe and patch are inputs to package hashing.
6. CI cache keys include `vcpkgOverlayPorts*/**`, so committing any overlay or patch change produces a new dependency-cache key.

No workflow-specific patch command is needed. Coverage, sanitizer, Ubuntu test/deploy, Windows test/deploy, and matching local presets all consume the same common overlay.

## Failure Behavior

Patch drift must be a hard failure. The complete overlay intentionally pins its own Qt 6.11.1 source metadata, so changing only the vcpkg baseline does not silently switch `qtdeclarative` to a different archive. When the overlay source metadata is deliberately refreshed for a Qt update, vcpkg will stop during source extraction if the patch no longer applies instead of silently building unpatched Qt. A baseline or Qt update therefore requires refreshing the copied port metadata and explicitly rechecking, rebasing, or removing the patch.

## Verification

- Confirm the vendored patch applies to the exact Qt archive named by `port.data.cmake`.
- Confirm Git attributes preserve the vendored patch byte-for-byte when `core.autocrlf=true`.
- Run vcpkg formatting and manifest/configuration checks against the overlay.
- Reconfigure a Windows development preset and confirm vcpkg selects the overlay port and applies the patch.
- Build RhythmGame with the configured preset and run the relevant test preset.
- Confirm all vcpkg-backed CI configure presets inherit the common overlay and that CI cache hashes include its files.
- Run repository whitespace checks before committing.
- Push the current branch and monitor the triggered GitHub Actions workflow to completion; diagnose and fix any patch-related CI failure before considering the change complete.

## Maintenance

The overlay intentionally pins the copied `qtdeclarative` recipe to Qt 6.11.1. When the repository's vcpkg baseline or Qt series changes, update the overlay files alongside the baseline, check whether the upstream fix is already included, and either refresh the patch or remove the overlay once the stock port contains the fix.
