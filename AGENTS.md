# RhythmGame agent guidance

## Scope and authoritative documentation

This file applies to the entire repository. A more deeply nested `AGENTS.md` or
`AGENTS.override.md` may refine these instructions for its subtree.

Read the relevant project documentation before changing an unfamiliar area:

- `DEV_ENGINE.md` for configuration, builds, tests, developer targets, and dependencies.
- `DEV_THEME.md` for theme contracts, screen inputs, reusable behavior, and scaling.
- `DEV_LANG.md` for translations and localization.
- `CONTRIBUTING.md` for documentation and contribution entry points.

## Working agreements

- Preserve unrelated tracked and untracked work. This checkout commonly contains local skins, profiler traces,
  databases, generated dependency trees, and diagnostic artifacts. Never clean, reset, move, or include them unless the
  task explicitly targets them.
- Inspect the current source, branch, and build state before acting. Keep diffs focused on the requested behavior and
  avoid opportunistic rewrites.
- Search exact relevant paths first. Broad recursive searches in this repository are noisy and can hide the useful
  result through output truncation.
- Prefer the existing architectural seam and the narrowest correct layer. Do not move a theme-only behavior into shared
  C++ merely because C++ is easier to test.
- Report only validation actually completed. Distinguish source inspection, formatting, lint, build, automated tests,
  profiler evidence, and live gameplay verification.

## Skills and tools

When available, use the project-relevant skill whose trigger matches the task. In particular, prefer the Qt QML
coding/review/profiler skills for QML work, Qt C++ review for review tasks, Qt UI design for cross-screen UX work, and
the GitHub Actions efficiency or hardening skills for CI audits. Skills supplement these repository rules; they do not
replace the validation below.

## Configure, build, and test

When the ignored, user-local `CMakeUserPresets.json` is available, use `dev-rel` for routine Release validation in this
checkout unless the task requires another configuration. Otherwise, follow the tracked preset workflow documented in
`DEV_ENGINE.md`.

```powershell
cmake --preset=dev-rel
cmake --build build/dev-rel --config Release --target RhythmGame_exe --parallel 4
cmake --build build/dev-rel --config Release --target RhythmGame_test --parallel 4
ctest --test-dir build/dev-rel -C Release --output-on-failure -j 4
```

The commands below likewise assume that local `dev-rel` build tree; adapt the preset and build-directory names together
when using a tracked preset.

- Configure only when required; do not discard or regenerate a healthy build tree without a concrete reason.
- For a focused C++ test, use
  `build/dev-rel/test/bin/RhythmGame_test.exe`, not
  `build/dev-rel/bin/RhythmGame_test.exe`.
- If CTest names a test absent from current `HEAD`, a focused result contradicts the source, or an incremental
  `SongAssetStore` binary crashes suspiciously, rebuild the
  `RhythmGame_test` target before editing source. Regenerate test discovery through that build, rerun the focused case,
  then run the full suite if appropriate.
- If an MSVC build command times out, inspect active `MSBuild`, `cl`, and linker processes before launching another
  build; the original build may still be progressing.
- Run build targets that generate shared QML metadata sequentially. In particular, do not build `all_qmllint` and
  `RhythmGame_test` concurrently.

Useful validation targets include:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_qml_qmllint --parallel 2
cmake --build build/dev-rel --config Release --target RhythmGame_lr2_qml_qmllint --parallel 2
cmake --build build/dev-rel --config Release --target all_qmllint --parallel 2
cmake --build build/dev-rel --config Release --target format-check --parallel 2
cmake --build build/dev-rel --config Release --target docs --parallel 2
git diff --check
```

Choose validation proportionate to the change. Do not run the full build and suite for a documentation-only edit. After
code changes, always run `git diff --check` and inspect the final diff.

## QML and theme work

- Treat `DEV_THEME.md` and the live skin implementations as the contract. Keep reusable skin behavior optional, with
  small interfaces and explicit override points.
- Application-owned navigation and scene flow belong at the `ContentFrame` boundary. Shared or theme code should call
  semantic operations such as
  `globalRoot.returnToPreviousScreen()` instead of reaching into `sceneStack` directly.
- Gameplay presentation should consume injected play state such as
  `columnState.pressed`; reserve direct `Input` handling for actual input actions described by the theme contract.
- Disable screen-owned sounds and `Shortcut` objects when a screen is inactive; inactive
  `ContentFrame` input suppression does not disable shortcuts automatically.
- Construct Windows local-file URLs in C++ with `QUrl::fromLocalFile()` or through the existing `FileQuery` seam. Never
  hand-build `file:///` URLs; `#` and `?` in paths must not become fragments or queries.
- When changing layouts or themes, test meaningful window resizes and relevant aspect ratios.

QML lint may exit successfully while retaining known unresolved runtime-type or unqualified-access warnings. Report that
as "lint completed with existing warnings," not as warning-free lint. Standalone `qmltestrunner` may not load the
app-linked static
`RhythmGameQml` plugin; use project CMake wiring or a narrowly scoped probe and state the boundary.

## Translations and localization

- Do not create, infer, or update translations with AI unless the user explicitly requests translation work.
- When a change introduces or modifies user-facing source text without such a request, leave translation catalogs
  untouched and report that translation work remains.
- For explicitly requested translation work, follow `DEV_LANG.md` and preserve placeholders, formatting, accelerator
  markers, and the intended locale.

## Compatibility and reference implementations

For LR2 or beatoraja compatibility work, use Beatoraja and OpenLR2 source code from Github.

Trace the exact reference behavior, option gates, timers, CSV layout, and lifecycle before changing RhythmGame. Do not
infer compatibility from names or screenshots alone. Preserve useful information and interaction behavior when adapting
legacy skins rather than deleting content merely to make a layout fit.

## Runtime bugs and performance

- When a reported fix is ineffective, reproduce or trace the actual live path before treating unit tests, source
  inspection, or lint as proof. Prefer user-driven reproduction plus targeted logs; do not control the desktop unless
  the user explicitly requests it.
- Keep diagnostic logging low-overhead and remove temporary instrumentation before final verification.
- For QML or frame-time problems, use a Release or RelWithDebInfo build and correlate QML profiler results with
  frame/presentation evidence where possible. Profiler traces can perturb event counts and timing.
- A successful build or automated test does not prove a gameplay, focus, timing, audio, or visual bug is fixed. Request
  or report the remaining live retest explicitly.
