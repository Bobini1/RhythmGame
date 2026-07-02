# Settings UI Layout Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Give all settings pages a consistent Qt Quick workbench layout while preserving existing functionality.

**Architecture:** Keep the existing Fluent WinUI3 Qt Quick Controls style and current settings models. Add only small shared QML layout helpers, then reshape each page around three patterns: list/detail manager, form workbench, and table-like data workbench.

**Tech Stack:** Qt Quick/QML, Qt Quick Controls, Qt Quick Layouts, existing `RhythmGameQml` objects and settings components.

---

### File Map

- Modify `share/RhythmGame/themes/Default/scripts/settings/PlayerSettings.qml`: profile list/detail manager with identity, online account, and replay import sections.
- Modify `share/RhythmGame/themes/Default/scripts/settings/SongFolderSettings.qml`: folder source list plus scan activity panel with useful empty state.
- Modify `share/RhythmGame/themes/Default/scripts/settings/TableSettings.qml`: denser installed/browse table layout and aligned list rows.
- Modify `share/RhythmGame/themes/Default/scripts/settings/ThemeSettings.qml`: reduce nested panels and make screen/theme editing clearer.
- Modify `share/RhythmGame/themes/Default/scripts/settings/GeneralSettings.qml`: split settings into more balanced groups.
- Modify `share/RhythmGame/themes/Default/scripts/settings/KeySettings.qml`: convert repeated binding rows into table-like player panels.
- Add small QML helpers under `share/RhythmGame/themes/Default/scripts/settings/` only if the same structure is needed by at least two pages.

### Tasks

- [ ] Add or adjust shared row/page helpers without changing behavior.
- [ ] Refactor Player settings into compact profile list plus detail sections.
- [ ] Refactor Song folders into source list plus scan activity with empty state.
- [ ] Refactor Tables into installed-source and browse-source workbench with aligned rows.
- [ ] Refactor Themes to reduce nested frames and clarify screen/theme hierarchy.
- [ ] Refactor General settings into balanced grouped panels.
- [ ] Refactor Key config into binding tables for player 1 and player 2.
- [ ] Run `git diff --check`.
- [ ] Run `cmake --build --preset=dev --target RhythmGame_exe`; accept only the known `zlibd.lib` link blocker after QML resources compile.
- [ ] Commit focused changes on `codex/settings-ui-redesign`.
