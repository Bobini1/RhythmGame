# Arena Select Panel Polish Specification

## Outcome

Arena song selection must preserve each skin's useful song-select layout while presenting one consistent Arena room panel. The Default theme and LR2/Beatoraja fallback use the same panel content and interactions.

## Visual composition

- Default select keeps the normal `640 x 480` StageFile image. Arena must not crop it to a short banner.
- Default select keeps `stageFileFrame.png` visible because that composite asset supplies the translucent backing behind the song wheel.
- The Default panel starts in the visual gap between the StageFile and the song-wheel rows. Its authored default is based on `x = 728`, `y = 120`, `width = 520`, and `height = 480` in the scaled `1920 x 1080` design coordinate system, then clamped to the real viewport.
- LR2/Beatoraja starts with a bounded panel near the top-right safe area. It must never dim or intercept the whole select screen.
- Normal, non-Arena song selection is unchanged.

## Shared panel UX

- Both native Default and legacy fallback surfaces use the same room panel component.
- The panel is always expanded. There is no `+`, collapse button, or hidden second layer of room controls.
- Details and Chat are exclusive tabs backed by one authoritative selection state. Exactly one tab is always selected, including after clicking or keyboard-activating the active tab again.
- Details shows the roster and synchronized selection summary. Chat shows the same roster beside chat.
- Details and Chat share the same body origin at a given panel rectangle. The roster and summary/chat content start immediately below the header; Details leaves unused vertical space below its summary instead of centering the body row.
- Ready, readiness reason, and moderation remain available in either tab.
- The panel has no Leave button. Escape leaves the room and returns to the Arena browser through the select screen's existing room-exit path.
- Return and keypad Enter perform the same select-screen activation. In Arena, either key selects the focused chart just like clicking it.

## Placement and direct manipulation

- Placement is stored only in the existing per-profile, per-select-theme variable map as:
  - `arenaOverlaySelectXNormalized`
  - `arenaOverlaySelectYNormalized`
  - `arenaOverlaySelectWidthNormalized`
  - `arenaOverlaySelectHeightNormalized`
- No standalone placement class, file, or lobby-state persistence is introduced.
- A compact, full-width header is the only drag handle during ordinary Arena selection. It exposes a move cursor and a subtle grip cue so the draggable area is apparent; buttons, roster actions, and chat keep their normal pointer behavior.
- The room title keeps priority in the header. Details and Chat use compact, adjacent tab widths instead of allowing the tab bar to expand and ellipsize the title while unused header space remains.
- Eight edge/corner resize hit areas are active during ordinary Arena selection. Corner targets remain compact, while each side target spans the usable edge between the corners. Their visual handle chrome is always hidden; resize cursors provide hover feedback across the full target.
- Moving and resizing clamp the panel to a 24-pixel safe viewport margin and persist normalized geometry when the gesture ends.
- Select placement does not require or consume F2. Existing gameplay/result F2 customization remains unchanged.
- The panel minimum is `520 x 320` where the viewport permits it; smaller viewports clamp safely rather than placing controls off-screen.

## Legacy Loader correction

- A fullscreen `Loader` may size a transparent fullscreen host, but it must not directly size the bounded visual panel.
- The legacy fallback's dark rectangle belongs only to the placed panel. The surrounding host stays transparent and passes non-panel visual composition through.

## Regression coverage

- Theme-var seeding, per-theme independence, persistence, invalid placement fallback, safe clamping, full-header drag, and invisible direct resize from every side and corner are tested.
- Default and legacy tests cover initial exclusivity, reactivating each selected tab by mouse and Space, switching tabs, and common action routing.
- Shared-panel tests compare Details and Chat at identical dimensions and require the roster and loaded body content to keep the same top origin. Header tests require a usable title width and adjacent compact tabs at the minimum panel width.
- Select-input tests require Return and keypad Enter to activate the same chart-selection path, while Escape leaves the room without an in-panel Leave control.
- Legacy tests mount the real fullscreen-Loader shape and assert that the visual panel remains bounded.
- Default composition tests assert the full StageFile, restored song-wheel backing, scene-space placement host, and gap-based default rectangle.
