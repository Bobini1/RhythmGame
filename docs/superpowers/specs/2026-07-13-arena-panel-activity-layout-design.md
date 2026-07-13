# Arena panel activity and responsive layout design

## Scope

This change polishes the application-owned Arena panels on select, gameplay,
and result screens. It does not change the wire protocol or persist room state.

## Shared presentation

- Details and Chat use the same quiet text-tab styling already used by the
  select panel. The selected tab has a short underline; clicking the selected
  tab cannot leave both modes inactive.
- Result content is clipped to the panel. The standings list owns the remaining
  height and scrolls; no delegate may paint beyond the border at any legal
  panel size.
- The Arena ranking position in the Default result theme uses the configured
  result-ranking font weight instead of forced bold.

## Chat activity

- Details and Chat remain distinct modes. Details shows the screen's primary
  Arena information; Chat shows the complete backlog and input.
- While Details is active, a persistent activity rail appears for unread remote
  messages. It shows the newest sender and message text plus the unread count.
- The rail remains until Chat is opened. Activating the rail opens Chat and
  marks the accumulated messages read.
- Messages authored by the local member do not create unread activity.
- Unread state belongs to `ArenaSession`, not to an individual screen, so it
  survives select/gameplay/result presentation changes within the room.
- Replacing chat with a reconnect snapshot establishes history without marking
  the snapshot backlog unread. Leaving the room clears chat activity.

## Density modes

- Select has no compact/expanded density control.
- Gameplay defaults to compact. Compact rows show aligned rank/identity,
  EX score, projected current clear type, and player status as four distinct
  fields.
- At narrow widths, gameplay rows reflow into identity/status followed by
  EX score/projected clear. Status and clear type are never conflated.
- Expanded gameplay rows additionally show all existing statistics:
  `BP/Combo`, `PG/GR/GD`, and `BD/PR/EP`.
- Result defaults to expanded. Compact result still lists every participant;
  expansion adds the complete per-player details rather than controlling
  whether standings are visible.
- The density action is labelled Expand or Compact, is hidden in Chat, and
  restores its prior state when returning to Details.

## Sizing

- Panel geometry continues to use the existing normalized theme-vars fields.
- Application-owned Arena content uses the viewport's exact uniform
  1920x1080 fit scale:

  `min(viewport width / 1920, viewport height / 1080)`

- There is no lower or upper scale clamp. A tiny viewport may produce tiny
  controls; a 4K logical viewport scales the panel metrics to 2x.
- Manual panel resizing changes available layout space only. It does not alter
  the viewport-derived metric scale; content reflows and lists scroll.
- LR2/Beatoraja non-uniform skin stretching is not applied to Arena text.
- Minimum geometry, default geometry, safe margins, and placement interaction
  metrics follow the same uniform scale so the panel does not retain an
  absolute screen-pixel floor.

## Verification

- Build the affected QML modules and Arena C++ targets.
- Run existing non-UI Arena tests that cover changed session/model behavior.
- Do not add QML tests or tests that parse QML source as strings.
- Manually inspect select, gameplay compact/expanded, result
  compact/expanded, chat activity, narrow resized panels, and a scaled
  viewport with Default plus one LR2/Beatoraja skin.
