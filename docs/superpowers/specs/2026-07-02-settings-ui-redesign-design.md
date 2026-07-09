# Settings UI redesign design

## Context

The current settings UI is functional but hard to read at a glance. It is implemented in QML under `share/RhythmGame/themes/Default/scripts/settings/` and uses Qt Quick Controls with the application style set in C++:

- `src/main.cpp` calls `QQuickStyle::setStyle("FluentWinUI3")`.
- The settings screen is a themed QML page with six top-level areas: player settings, song directories, tables, themes, general settings, and key config.
- Generated theme settings use reusable property editors in `settingsProperties/`: boolean, range, choice, string, file, color, font, and group.
- Existing functionality must be preserved. This is a redesign of presentation, layout, reusable controls, and interaction states.

## Problems to avoid

- Weak visual hierarchy: panels, rows, buttons, labels, and section headings currently have similar weight.
- Inconsistent geometry: different pages use noticeably different widths, panel layouts, and alignment rules.
- Gray-on-gray palette: active, disabled, secondary, and destructive states are too similar.
- Hidden explanations: generated setting descriptions are mostly tooltip-only, which makes dense settings unfriendly.
- Button noise: reset, remove, reload, scan, add, and configure actions often compete visually.
- Awkward empty space: large displays leave huge unused regions while important controls remain narrow.
- Hard-to-scan rows: repeated lists need clearer columns, row states, and stable value areas.
- Weak navigation orientation: the top tabs work, but the active state and current page context are too subtle.
- Theme-dependent contrast: FluentWinUI3 follows the system light/dark mode, so custom surfaces must work in both modes.

## Chosen direction

Use the **Friendly workbench** direction as the shared design language.

The settings screen should feel like a polished desktop settings workspace: calm, readable, and consistent. Keep the current top-level tab structure, but make each tab feel like a clear work area with:

- A stronger top navigation state.
- A page header with title and short contextual subtitle.
- Consistent content margins and max widths.
- Workbench panels with clear headings and optional summary/status text.
- Reusable list rows with fixed action zones.
- More deliberate primary, secondary, destructive, and reset actions.
- Better hover, pressed, focus, disabled, selected, loading, empty, and error states.

Ideas from the other explored directions should be used where they fit the task:

- Use the compact operator-panel approach for tables and key bindings, where dense repeated rows are useful.
- Use the settings-library approach for theme/general settings, where groups, descriptions, and search-like scanning matter more than raw density.

## Qt and FluentWinUI3 constraints

The redesign should cooperate with Qt Quick Controls and FluentWinUI3 rather than fighting them.

- Keep runtime style selection through `QQuickStyle::setStyle("FluentWinUI3")`.
- Continue importing `QtQuick.Controls` for normal controls that should inherit the active runtime style.
- Prefer Qt Quick Controls (`Button`, `ToolButton`, `TabBar`, `TabButton`, `ComboBox`, `CheckBox`, `Slider`, `SpinBox`, `TextField`, `ScrollView`, `SplitView`, `Frame`, `GroupBox`, `ItemDelegate`) over rebuilding equivalent controls from primitives.
- Create small reusable QML components for app-specific structure: page shell, panel, list row, setting row, action buttons, tag chips, empty/error states.
- Override control `background`, `contentItem`, `indicator`, `handle`, or delegates only where FluentWinUI3 does not provide enough hierarchy or density.
- Do not create a full custom Qt Quick Controls style unless repeated one-off customizations become harder to maintain than a local component set.
- Avoid switching settings pages or shared settings components to `QtQuick.Controls.Basic`; keep the runtime FluentWinUI3 style path through ordinary `QtQuick.Controls` imports.
- Avoid mixing compile-time style imports with the existing runtime style path in settings UI code.
- Preserve built-in accessibility from Qt Quick Controls where possible. Custom interactive items must define focus behavior and accessibility names.
- Derive custom colors from the active Qt palette where practical (`palette.window`, `palette.base`, `palette.alternateBase`, `palette.mid`, `palette.button`, `palette.highlight`, `palette.text`, `palette.placeholderText`) instead of hard-coding a dark theme.
- Treat light mode and dark mode as first-class. Any custom panel, row, chip, button variant, or focus ring must be checked in both modes.
- Use Qt layouts idiomatically: no mixing `anchors` and `Layout.*` on the same layout-managed item, no hard-coded width math where layouts can express the relationship, and stable preferred/minimum sizes for repeated rows.
- Keep user-visible strings translatable with `qsTr()`.

## Shared design system

### Shell

`Settings.qml` should become the consistent shell:

- Full-screen `Page` using the existing settings fonts.
- Header row with a clearer back `ToolButton` and a centered/filled `TabBar`.
- Active tab state should be visible beyond a tiny underline: selected background, stronger text, and a subtle accent marker.
- Page content should use a shared workbench container with responsive horizontal margins.
- The content area should own the page title/subtitle, not each tab inventing its own geometry.

### Surfaces

Use an adaptive palette tuned for Fluent controls in both system light and dark modes:

- App background should come from the active window/background palette, with enough contrast from panels in light and dark mode.
- Panel backgrounds should be one step above or below the app background, using palette roles or helper functions instead of fixed charcoal/white values.
- Row hover/selected fills should be subtle accent-tinted fills that remain visible in light mode and do not glow too strongly in dark mode.
- Borders should be visible separators in light mode and low-glare separators in dark mode.
- Accent should use `palette.highlight` or a restrained derived accent, not saturated red except for errors/destructive feedback.
- Destructive actions should use a danger treatment that is legible on both light and dark backgrounds.
- Disabled controls should reduce emphasis but keep labels legible in both modes.
- Custom chips, status badges, and selection strips should define both foreground and background contrast, not rely on a single fill color.

Do not make everything a card. Use panels for primary work areas, rows for repeated records, and simple section blocks for generated settings.

### Typography

- Keep the app's theme font mechanism.
- Use a modest but clearer scale: page title, panel title, row primary text, secondary metadata, helper text.
- Use tabular numeric alignment for counts, percentages, and key states where practical.
- Avoid large display typography inside dense controls.

### Actions

Define reusable button variants:

- Primary: add, import, sync, scan all, save/apply-like actions.
- Secondary: open folder, reload, scan one item, configure.
- Tertiary/reset: reset buttons should be visually quieter and consistently aligned.
- Destructive: remove/delete/logout should be secondary or danger-styled, never visually equal to primary actions.

### Rows

Repeated rows should have stable columns:

- Leading identity/content.
- Optional metadata/count/status.
- Fixed trailing actions.
- Selected/active state that is visible without relying only on color.
- Hover state for mouse users.
- Keyboard focus state for Qt focus navigation.

### Generated setting rows

The generated settings property components should share one row component:

- Label column.
- Optional inline description below the label, visible when present instead of tooltip-only.
- Control/value column.
- Reset column.
- Responsive behavior for narrow widths: stack label/description above control while keeping reset reachable.

## Per-page design

### Player settings

Keep the two-panel workbench:

- Left panel: profile list with selected row, score count, and quieter remove action.
- Right panel: selected profile details.
- Avatar/name/open-folder form should sit in a compact profile summary block.
- Online login and replay import should be separate sections with clear status text and progress/error states.
- Add profile should be a primary action near the profile list header or bottom action bar.
- Delete confirmation remains modal, but the delete trigger should read as destructive.

### Song directories

Keep the two-panel workbench:

- Left panel: configured song directories.
- Right panel: scanning queue and current scanning status.
- Folder rows should show path, scan action, remove action, and scanning state if relevant.
- Bottom action bars should remain: add folder and scan all.
- Empty states should explain what to do when no folders or no queue entries exist.

### Tables

Use the compact operator-panel treatment:

- Keep the left/right split: installed tables and browsed tables.
- Installed tables need stable columns for drag handle, URL/name, status, reload, remove.
- Browsed tables need title, URL/comment, tag chips, and add action.
- Filter/search bar should be stronger and easier to scan: recommended toggle, type/category filters, search, reload, and source URL configuration.
- The source URL editor should feel like an expandable advanced panel.
- Loading/error/empty states should be inline and centered in the browse pane.
- Drag handles should be visually clear and have a tooltip.

### Themes

Use the settings-library treatment inside the workbench:

- Keep the left screen list because it maps well to theme screens.
- Improve the selected screen area with a header showing selected screen, selected theme, and available screen settings.
- Place the theme picker near the header, not as a small orphaned control.
- Generated groups should use clearer section containers and inline descriptions.
- If a screen has no settings, show an empty state instead of a blank area.

### General settings

Use grouped setting sections:

- System settings.
- Gameplay timing.
- Lane cover/lift/hidden.
- BGA, score graph, ghost/replay.
- Score target.
- Note order and speed.
- DP and gauge options.

Each section should use the shared generated setting row style. Reset remains aligned but visually quiet.

### Key config

Use compact, readable key binding panels:

- Player 1 and Player 2 should be matched panels.
- Each row should have stable columns: button name, binding text, live state, configure, reset.
- Live state should be a small status chip with stable width.
- Configure should remain checkable and clearly indicate the listening/configuring state.
- Analog axis panels should remain responsive, but should visually belong to the same workbench instead of floating beside it.

## Functional requirements

Preserve all existing behavior:

- Back button and Escape pop the settings screen.
- Initial tab index still works.
- Profile selection, creation, deletion, avatar selection, profile renaming, folder opening, login/logout, score sync, replay import, import progress, and import errors.
- Adding/removing/scanning song folders and queue cancellation.
- Table add/remove/reload/reorder, browser fetch, filters, recommended toggle, search, source URL edit/reset, and error/loading states.
- Theme selection per screen and generated/overridden screen settings.
- General settings bindings to `generalVars`, `Window.window`, `Rg.audioEngine`, and `Rg.inputTranslator`.
- Key binding configure/reset behavior and live input state display.

## Implementation boundaries

- Stay within the existing QML theme settings code unless a small C++ exposure is strictly necessary.
- Favor shared QML components over copy-pasted layout styling.
- Avoid touching gameplay screens except for shared components that are already used there.
- Do not migrate away from Qt Quick Controls or replace the global style as part of this redesign.
- Keep edits reviewable: create shared primitives first, then migrate each settings page.

## Verification

Before considering the redesign complete:

- Run the relevant build/QML validation target available in the repo.
- Launch the app and manually inspect all six settings tabs.
- Verify at desktop-wide, medium, and narrow window widths.
- Check both system light mode and system dark mode with FluentWinUI3.
- Specifically verify that custom panels, selected rows, hover states, reset/destructive buttons, tag chips, key state chips, and focus rings remain readable in both modes.
- Verify all listed functional requirements still work.
- Confirm no text clipping in Polish or English labels where practical.
- Confirm keyboard focus remains visible for controls and custom row actions.
- Confirm `.superpowers/` remains ignored and visual companion artifacts are not staged.
