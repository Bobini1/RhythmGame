# Online Arena Phase 4 design

## Status and authority

This specification derives from the approved Online Arena umbrella design and
the accepted Phase 1, Phase 2, and Phase 3 designs. The user authorized
autonomous assumptions, implementation, and continuation without another
question or approval gate. If this document conflicts with the umbrella
design, the umbrella design wins; otherwise the exact contracts below control
Phase 4.

Phase 3 is a prerequisite. This design consumes its protocol 1.2
`competition-v1` contract, `ArenaSession`, standings/result models,
strongest-opponent target, fixed application-owned gameplay/result overlays,
gameplay-chat action, Default transient result source, and LR2/Beatoraja result
numbers 179/180. It changes presentation, profile-local placement, verification,
and deployment behavior. It does not redefine a room, round, score, ranking,
inventory, option-synchronization, or IR-upload rule.

## Goal

Turn the functional Phase 3 milestone into a usable and operable first public
Arena release:

- Default song select and result present Arena as native parts of the theme.
- LR2 and Beatoraja song select and result receive complete application-owned
  presentations even though those skin formats have no Arena concepts.
- The compact live leaderboard is forced on for every Arena gameplay skin and
  can be moved and resized with F2 without editing a skin.
- Gameplay and result overlay rectangles persist per active profile, resolved
  skin family, layout variant, and overlay kind.
- The same F2 gesture participates in Default's existing gameplay customize
  mode and provides an equivalent mode for LR2/Beatoraja.
- Gameplay chat remains closed by default, opens deliberately, never pauses the
  chart, and attaches to the configured leaderboard without leaving the
  viewport.
- Focus, keyboard operation, untrusted text, scaling, contrast, and translated
  copy receive a release-level pass.
- The Bun service exposes bounded, privacy-safe operational metrics, applies
  proxy-aware connection admission, survives malformed traffic, and has a
  reproducible single-replica Coolify runbook and deployment smoke.

Phase 4 is complete when representative Default, LR2, and Beatoraja select,
gameplay, and result paths are usable at common aspect ratios; overlay geometry
survives a restart under the correct profile/skin/layout key; a production
container passes the malformed/load/privacy/shutdown gates; and the documented
official WSS deployment can be checked without a player credential.

## Scope

Included:

- A versioned, profile-local Arena overlay placement store.
- Application-owned drag, resize, reset, keyboard nudge, viewport clamping,
  first-use hint, and coordinated F2 mode.
- Forced compact leaderboard visibility and session-only detail expansion.
- Safe suppression of chart input while Arena overlay customization mode is
  active, without pausing chart time.
- Native Default select and result panels using existing ArenaSession models.
- Complete LR2/Beatoraja select and result overlays.
- Gameplay chat attachment, flipping, focus, unread indication, and Escape
  priority.
- Accessibility, plain-text, English-source/Polish localization, and responsive
  layout polish.
- Proxy-aware anonymous connection admission, hello timeout, metrics, graceful
  shutdown, Docker hardening, Coolify documentation, and public smoke scripts.
- Cross-repository end-to-end, malformed-input, load, privacy, representative-
  skin, and non-Arena regression gates.

Excluded:

- A protocol 1.3, new capability, or changed wire message.
- Spectators, matchmaking, ratings, persistent match history, alternate song-
  picking rules, downloads, voice chat, item/ojama mode, or tournaments.
- A setting that hides the Arena leaderboard.
- Persisting detail expansion, open chat, room state, results, wins, or chat.
- User-authored layout files or exposing overlay placement to remote peers.
- Horizontal scaling, Redis, shared room ownership, or more than one official
  Arena replica.
- Automatic Netcup DNS, certificate, or Coolify mutation without operator
  credentials. The repository supplies the exact deployable image, settings,
  runbook, and smoke; applying them is an external operation.
- Translating the entire Default theme or advertising a new language. Phase 4
  completes the supported English-source and Polish Arena surface; the existing
  unadvertised Japanese catalog may continue to use source fallback.

## Autonomous assumptions

1. Protocol major/minor remain `1.2`, and admission still requires
   `rooms-v1`, `rounds-v1`, and `competition-v1`. No Phase 4 UI preference or
   operational metric travels over WebSocket.
2. The canonical placement descriptors created by Phase 3 remain exact:
   `placementKind` is `gameplayLeaderboard` or `resultStandings`,
   `resolvedSkinId` is the active profile's resolved theme-family key, and
   `layoutVariant` is `k5`, `k7`, `k10`, `k14`, or `result`.
3. Gameplay leaderboard placement and legacy result placement are both
   customizable. Default result uses its native panel but still has a
   `resultStandings` record available if native integration is unavailable.
4. Select UI is not given free-form geometry in the first release. Default owns
   a native panel; LR2/Beatoraja use a bounded collapsible application overlay.
5. The overlay rectangle is stored in
   `<profile-directory>/arena-overlays.json`; it is deliberately separate from
   theme settings because imported skins have frozen/foreign settings schemas.
6. Stored rectangles are normalized to the application viewport. Clamping for
   a temporary viewport/aspect change does not rewrite the saved value until
   the player moves, resizes, nudges, or resets the overlay.
7. F2 toggles one Arena customization mode during Arena gameplay/result. In
   Default gameplay it also toggles the existing `customizeMode`; in
   LR2/Beatoraja it affects the Arena overlay only.
8. Chart time never pauses for customization. The active Arena runner ignores
   chart input for the full customization mode and releases already-held lanes
   once on entry so drag/keyboard operations cannot create or hold notes.
9. Compact leaderboard visibility is mandatory. Expansion and chat are
   optional views, but collapsing them always leaves rank/name/EX/progress/
   state visible.
10. The first-use F2 hint is remembered once per profile at hint version 1 and
    is not keyed by room or skin.
11. Current supported Arena localization is English source plus Polish. Every
    new Arena string is extracted; Polish has no unfinished Phase 4 Arena
    entries. Japanese may fall back to the English source until that locale is
    advertised and reviewed separately.
12. Production remains one in-memory Arena replica. Deploy/restart intentionally
    destroys rooms; graceful shutdown informs clients and gives them a bounded
    interval to return to the Browser cleanly.
13. `/metrics` is disabled unless explicitly enabled and is protected by an
    operational bearer token. `/healthz` remains public, no-store, and
    independent of IR/JWKS.
14. Forwarded client addresses are trusted only when the direct peer matches an
    explicitly configured proxy CIDR. The default empty trust list preserves
    direct-peer behavior for local development.
15. Existing unrelated and in-progress Phase 2/3 files are user-owned. The
    Phase 4 documentation commit changes only its two new documents.

## Alternatives considered

### Theme-native UI everywhere

Implementing a complete Arena screen in every theme would give themes maximum
visual control, but LR2 and Beatoraja formats cannot declare the required
models, moderation, chat, or placement behavior. It would also duplicate
security-sensitive plain-text and input rules and make imported skins fail by
default. This approach is rejected.

### One universal overlay on every screen

A single overlay would minimize code, but it would leave the Default theme
feeling bolted on and would duplicate information already suited to Default's
ranking and select layouts. It would also make future native themes unable to
opt into a better composition. This approach is rejected as the only strategy.

### Shared content with presentation capabilities

The selected hybrid keeps all networking and authoritative state in
`ArenaSession`, shares application-owned roster/chat/standings components, and
uses three narrow presentation capabilities:

- Default select declares native Arena select support and mounts a native
  panel.
- Default result declares native Arena result support and mounts a winner/
  standings panel beside its existing Arena ranking source.
- Every gameplay family uses the same application-owned live leaderboard;
  Default additionally participates in its existing F2 customize mode.

Any screen without a declared native capability automatically receives the
legacy application overlay. Imported LR2/Beatoraja skins therefore work
without edits, while a future native theme can opt in explicitly.

## Architecture

### Presentation remains downstream of `ArenaSession`

All Phase 4 QML consumes only the Phase 3 session/models:

- `members`, `chat`, selection/readiness/phase values, and `lastResult` on
  select.
- `liveStandings`, `opponentTarget`, frozen option summary, and
  `gameplayChatOpen` during play.
- `presentedResult`, its standings, winners, DNF, and lobby wins on result.

No QML component parses a protocol object, computes a competition rank,
changes room phase, invents a winner, or writes a profile play option. The
server remains authoritative and Phase 3's normal score-save/IR-upload path is
unchanged.

### `ArenaOverlayPlacementStore`

A focused C++ QObject owns profile-local placement persistence. It observes
`ProfileList::mainProfileChanged`, loads only the active profile's file, and is
exposed as `Rg.arenaOverlayPlacements`. It neither depends on Arena transport
nor survives outside the profile directory.

Its QML-facing contract is:

```cpp
class ArenaOverlayPlacementStore final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString profileGuid READ profileGuid NOTIFY profileChanged FINAL)
    Q_PROPERTY(int currentHintVersion READ currentHintVersion CONSTANT FINAL)

  public:
    static constexpr int CurrentFileVersion = 1;
    static constexpr int CurrentHintVersion = 1;

    Q_INVOKABLE QVariantMap placement(const QString& placementKind,
                                      const QString& resolvedSkinId,
                                      const QString& layoutVariant) const;
    Q_INVOKABLE bool setPlacement(const QString& placementKind,
                                  const QString& resolvedSkinId,
                                  const QString& layoutVariant,
                                  qreal x, qreal y,
                                  qreal width, qreal height);
    Q_INVOKABLE bool resetPlacement(const QString& placementKind,
                                    const QString& resolvedSkinId,
                                    const QString& layoutVariant);
    [[nodiscard]] auto currentHintVersion() const -> int;
    Q_INVOKABLE bool shouldShowHint(int version) const;
    Q_INVOKABLE void markHintShown(int version);

  signals:
    void profileChanged();
    void placementChanged(const QString& placementKind,
                          const QString& resolvedSkinId,
                          const QString& layoutVariant);
};
```

`placement()` returns `{ stored, x, y, width, height }`; an absent/invalid
entry returns `{ stored: false }`. `setPlacement()` accepts only finite values,
closed placement kinds, nonempty skin/layout keys of at most 128 Unicode code
points, `0 <= x,y < 1`, `0.05 <= width,height <= 1`, and
`x + width <= 1`, `y + height <= 1`, allowing only a `1e-6` numeric tolerance.
Rejected writes do not mutate memory or disk.

The on-disk format is deterministic and versioned:

```json
{
  "version": 1,
  "hintVersion": 1,
  "placements": [
    {
      "kind": "gameplayLeaderboard",
      "skinId": "Default",
      "layoutVariant": "k7",
      "x": 0.653125,
      "y": 0.033333,
      "width": 0.328125,
      "height": 0.5
    }
  ]
}
```

Entries are sorted by kind, skin ID, and layout variant before an atomic
`QSaveFile` commit. A malformed entry is skipped independently. A malformed
root or unknown future version yields defaults and is retained untouched until
the player performs a successful placement write/reset. Paths, room IDs,
member IDs, and remote text never appear in this file.

### Application-owned placement frame

`ArenaOverlayPlacementFrame.qml` wraps the existing Phase 3 gameplay/result
content. It receives the three canonical descriptor strings, the placement
store, the viewport, and `customizeMode`.

Resolution rules:

1. Read the normalized stored rectangle, or compute a default.
2. Convert it into viewport pixels.
3. Enforce a 24 px safe margin where the viewport permits it.
4. Enforce a 280 x 160 px minimum, capped by the safe viewport.
5. Clamp position and size without writing the clamped value back.
6. Commit normalized geometry only after a drag/resize ends, after a keyboard
   nudge, or after reset.

Default gameplay geometry is top-right, width
`clamp(viewport.width * 0.30, 320, 420)` and height
`clamp(viewport.height * 0.44, 240, viewport.height - 48)`. Default legacy
result geometry is top-right, width
`clamp(viewport.width * 0.40, 360, 560)` and height
`clamp(viewport.height * 0.60, 260, viewport.height - 48)`.

Customization provides:

- A visible high-contrast frame and eight pointer resize handles.
- Dragging from the frame header/background.
- Arrow-key movement by 4 px; Shift+Arrow by 16 px.
- Alt+Arrow resize by 4 px; Alt+Shift+Arrow by 16 px.
- `R` and a visible Reset action for the exact descriptor key.
- Enter or F2 to commit/leave mode.
- Escape to commit/leave mode before gameplay abort handling.
- Accessible names and instructions for the frame and every control.

The frame is application-owned and cannot import Default's
`TemplateDragBorder`; legacy skins must not depend on files from the Default
theme. Pointer handlers exist only while customization is active or on real
overlay controls. Outside those regions the overlay does not consume gameplay
pointer events.

### Coordinated F2 and chart-input suppression

`ArenaOverlayHost` owns the Arena F2 shortcut whenever an Arena gameplay or
Arena result surface is current. It is the single owner during Arena; the
Default gameplay screen disables its local F2 shortcut and exposes:

```qml
function setArenaCustomizeMode(active) {
    root.customizeMode = active;
}
```

The host invokes that capability when present. Default's existing drag borders
and the Arena frame therefore appear together. A legacy wrapper does not
declare the capability, so the host shows a transparent pointer shield below
the Arena frame while customizing.

`ArenaSession` adds a presentation-policy command:

```cpp
Q_PROPERTY(bool overlayCustomizationActive
           READ overlayCustomizationActive
           NOTIFY overlayCustomizationActiveChanged FINAL)
Q_INVOKABLE void setOverlayCustomizationActive(bool active);
```

It applies only to `arenaRunner`. `ChartRunner` gains a generic internal input-
suppression state. Entering suppression releases each currently pressed mapped
lane once at the current monotonic chart offset, then ignores subsequent chart
press/release events until restored. It does not stop the runner, timers,
audio, BGA, telemetry, or chat. Leaving mode, screen destruction, round end,
profile change, session exit, and runner destruction all restore the gate
idempotently. Non-Arena runners never enter it.

### Forced visibility

For an active Arena runner, the compact leaderboard is always instantiated,
visible, at least 280 x 160 px when the viewport permits, and clamped on screen.
There is no visibility property in the placement file and no Hide action.
Expand/collapse affects only detail rows; the compact view always retains:

- Competition rank or dash.
- Player name.
- EX score or no-data marker.
- Progress.
- Connected/playing/finished/DNF state.

The same rule applies to the compact winner/rank summary on a legacy Arena
result. Default result always retains its native winner banner and Arena rank
even when detailed standings are collapsed.

### First-use hint

On the first active Arena gameplay for a profile whose `hintVersion < 1`, the
host shows `Press F2 to move Arena standings` for six seconds. Activating F2 or
dismissing the hint marks version 1 immediately. The hint is plain text,
localized, nonmodal, and never reappears for that profile unless a later build
increments `CurrentHintVersion`.

## Screen presentation

### Capability-based native integration

ContentFrame does not special-case a path or parse a skin file. Loaded screens
may declare these optional capabilities:

```qml
readonly property bool arenaNativeSelectPresentation: true
readonly property bool arenaNativeResultPresentation: true
function setArenaCustomizeMode(active) { ... }
```

Default declares all relevant capabilities. A screen without a native select
or result capability receives the application-owned legacy overlay. This is
the path for LR2, Beatoraja, and third-party themes by default.

### Default song select

Default mounts `scripts/select/ArenaSelectPanel.qml` inside its authored
1920 x 1080 layout. It uses the left information area and preserves the song
list, search, sort, option, and table navigation paths. The panel contains:

- Room name and leave action.
- Current selection, selecting member, frozen random/DP summary, and sync/
  loading state.
- Ready/Unready action and why readiness is unavailable.
- Roster with owner marker, connected/reserved/waiting/loading state, ready,
  inventory sync, lobby wins, and owner-only kick.
- Last winner summary and previous lobby wins from `lastResult`.
- Chat history, unread count, plain-text input, and send action.

The roster remains visible; chat/detail can switch within the panel. The
existing StageFile remains as a bounded visual header rather than being
removed. No profile switch, local battle, course, replay, autoplay, or ranking-
play action is reintroduced while seated. Select activation and availability
remain the Phase 2 paths.

### LR2 and Beatoraja song select

`ArenaLegacySelectOverlay.qml` replaces the Phase 2 minimal strip for a screen
without native select presentation. Its compact header is always visible and
shows room, current selection/sync phase, ready state, and connected/reserved
count. Expanding it exposes:

- Full roster/status/wins and owner kick.
- Frozen option summary.
- Ready/Unready and leave.
- Chat/backlog/input.
- Last winner.

It is bounded to 420 px wide and the viewport minus 48 px, scrolls up to 16
members, and defaults to top-right. It may collapse to the compact header but
cannot remove readiness/room state. It is application-owned and uses the same
data components for LR2 and Beatoraja; the existing Phase 2 chart decorators
remain different (`(arena unavailable)` for LR2 and unavailable types for
Beatoraja).

### Gameplay

Every skin uses the placement frame plus `ArenaGameplayOverlay`. Default
supplies its normal typography/customize interaction around the shared
content; LR2/Beatoraja receive the same data in a neutral high-contrast panel.
The content preserves Phase 3 compact/expanded fields and strongest-opponent
target. Remote names are plain text and elided with a tooltip/accessibility
name rather than interpreted as markup.

The panel's stacking order is above the skin and below application modal
dialogs. Its z-order cannot be controlled by imported skin CSV/JSON. An
expanded 16-player list scrolls inside the stored rectangle rather than growing
outside it.

### Default result

Default result declares native presentation and adds
`scripts/result/ArenaResultPanel.qml`:

- A winner banner is visible first and lists every joint winner.
- Pending local result shows waiting-for-players without blocking normal IR
  ranking queries.
- Final standings show competition ranks, DNF, EX, BP/combo/judgements/gauge,
  and `lobbyWinsAfter`.
- The local row is marked without relying on color alone.
- Detailed standings may collapse, but winner, local Arena rank/count, and
  pending/final state remain visible.

The existing RankingPosition behavior from Phase 3 remains authoritative:
Arena is selected on every Arena result, normal providers can be cycled, and
the saved provider is untouched.

### LR2 and Beatoraja result

Screens without native result presentation receive the movable
`ArenaResultOverlay` inside a `resultStandings` placement frame. Its compact
summary always exposes pending/final state, every winner, and local rank/count
or DNF. Expanded mode shows all standings and wins.

LR2/Beatoraja number 179/180 overrides remain unchanged and continue to work
when a skin renders them. The overlay is still required because a skin may not
render either field. It defaults to top-right and is independently stored from
gameplay geometry under layout variant `result`.

## Chat behavior

### Select and result

Select chat is immediately available in the native/legacy room panel. Result
does not open a new chat editor by default; the room panel becomes available
again after returning to select. Chat remains the Phase 1 bounded transient
backlog and is never saved locally.

### Gameplay drawer

Gameplay chat remains closed on entry. F8 or the visible Chat action opens it;
incoming messages never auto-open it. While closed, rows inserted after the
current round began increment a session-local unread badge. Opening clears the
badge. Leaving the round/session clears both open and unread state.

The drawer has a 320-420 px target width and at most the safe viewport height.
It attaches in this order:

1. To the right when the configured leaderboard has enough safe space.
2. Otherwise to the left.
3. Otherwise below when vertical safe space is larger there.
4. Otherwise above.
5. On a viewport too small for every adjacent arrangement, occupy the largest
   safe rectangle that does not overlap the leaderboard, with an internal
   scroll view.

It recomputes after placement, resize, viewport, and aspect changes. The drawer
position is not persisted independently.

Enter sends, Shift+Enter follows the existing chat-schema multiline rule, and
Escape closes the drawer. Focused text input consumes keyboard play events by
the Phase 3 input route but does not stop controller play or pause the runner.
Shortcut priority is:

1. Escape closes gameplay chat.
2. Escape leaves Arena customization mode and restores chart input.
3. A later Escape invokes the existing Arena abandon path.

F2 closes an open chat drawer before entering customization. Opening chat while
customizing first exits customization and restores chart input.

## Accessibility and localization

All application-owned Arena UI must:

- Give buttons, fields, lists, rows, status indicators, and drag handles
  meaningful `Accessible.name`, role, and description values.
- Provide a deterministic Tab/Backtab order and visible focus indication.
- Support keyboard create/join/select/ready/chat/kick/leave, overlay nudge,
  reset, expand/collapse, and result inspection.
- Render every remote room name, display name, chart metadata, and chat body
  with `Text.PlainText`; rich-text auto detection is forbidden.
- Pair color with text/icon/state. Ready, unavailable, disconnected, winner,
  local row, and DNF cannot be color-only.
- Meet at least 4.5:1 text contrast and 3:1 focus/essential-control contrast in
  the application-owned panels.
- Preserve usable clipping/scrolling at 100%, 150%, and 200% UI/text scale,
  1280x720 through 3840x2160, and 4:3, 16:9, and 21:9 viewports.
- Keep pointer targets at least 32 x 32 logical pixels; primary touch-like
  actions use 40 x 40 where layout permits.
- Announce reconnect, selection invalidation, loading cancellation, winner,
  and DNF changes through one bounded accessible status region rather than
  repeatedly focusing new elements.

All Phase 4 strings use `qsTr`. Translation extraction includes every
application Arena QML and new Default Arena component. The English source and
Polish catalog are complete for Arena contexts. Accelerator instructions use
translated key names only where Qt does not render the shortcut itself.

## Operational hardening

### Protocol stability

Phase 4 does not change protocol schemas or capabilities. The official client
still sends 1.2/all three capabilities and uses the existing single legacy
browse fallback. A server/client wire fixture digest change in Phase 4 is a
release failure unless it fixes a previously documented Phase 1-3 parity bug
and updates the controlling earlier specification.

### Proxy-aware connection admission

The gateway gains a focused `ClientAddressResolver` and `ConnectionAdmission`
module. Defaults:

```text
TRUSTED_PROXY_CIDRS=
UPGRADE_ATTEMPTS_PER_ADDRESS_PER_MINUTE=120
MAX_CONNECTIONS_PER_ADDRESS=20
CLIENT_HELLO_TIMEOUT_MS=10000
MAX_TRACKED_ADDRESSES=20000
```

Configuration rejects `0.0.0.0/0`, `::/0`, malformed CIDRs, and duplicate
CIDRs. The resolver accepts at most 512 bytes and eight comma-separated entries from
`X-Forwarded-For`. It uses a forwarded chain only when the direct peer matches
`TRUSTED_PROXY_CIDRS`; otherwise the direct peer is the client address. It walks
the chain from right to left across trusted proxies and selects the first
untrusted canonical IPv4/IPv6 address. Invalid or ambiguous input falls back to
the direct peer. `X-Real-IP`, `Forwarded`, hostnames, ports, zone IDs, and
client-supplied forwarding headers from an untrusted peer are ignored.

Rate state uses an HMAC-SHA-256 address key with a process-random salt. Raw
addresses and forwarding headers are never emitted in logs or metrics. The
upgrade limit is checked before WebSocket application state. Concurrent
address count is released on every close/error path. A socket that does not
complete `client_hello` inside 10 seconds closes with policy code 1008 and a
redacted `hello_timeout` reason. Each address retains at most 120 attempt
timestamps and 20 active lease IDs. Address entries with no active lease and no
attempt in the rolling minute are evicted. If 20,000 live rate entries remain
after an expiry sweep, a new address receives HTTP 503 rather than allocating
unbounded state.

Identity/seat rate limits from Phases 1-3 remain independently enforced.
Production Coolify must configure only the actual Traefik/container-network
CIDR; an empty trust list remains valid direct-peer mode, while a broad public
CIDR is rejected at startup and by the deployment checklist.

### Metrics

No third-party metrics dependency is required. `OperationalMetrics` stores
bounded integer counters/gauges and fixed-bucket histograms. Exact names are:

```text
arena_connections_current
arena_connections_total
arena_rooms_current
arena_reserved_seats_current
arena_rounds_active
arena_rounds_started_total
arena_rounds_finalized_total
arena_rounds_cancelled_total{reason}
arena_auth_failures_total{reason}
arena_command_rejections_total{code}
arena_inventory_committed_bytes
arena_inventory_upload_seconds_bucket{le}
arena_standings_dropped_total
arena_websocket_closes_total{class}
```

Label values are closed enums already present in the protocol/domain. No room,
round, connection, member, user, IP, chart, score, or error text becomes a
label. Histograms use fixed buckets `0.1, 0.25, 0.5, 1, 2.5, 5, 10, 30, 60`
seconds and maintain count/sum.

Configuration:

```text
METRICS_ENABLED=false
METRICS_BEARER_TOKEN=
```

When enabled, a nonempty token of at least 32 bytes is mandatory. Exact
`GET /metrics` requires `Authorization: Bearer <token>`, compares in constant
time, returns Prometheus text with `Cache-Control: no-store`, and is never
linked from public health. Disabled metrics return 404. Authorization headers
and tokens are excluded from logs. Health remains credential-free and excludes
operational counts.

### Graceful deployment

SIGTERM/SIGINT follows one idempotent sequence:

1. Mark the gateway shutting down and reject new upgrades with 503.
2. Emit the existing reliable `server_going_away` event to writable sockets.
3. Stop new room mutations and stop accepting new inventory/telemetry work.
4. Wait up to `SHUTDOWN_DRAIN_MS=8000` for queued reliable sends.
5. Close remaining sockets with 1012, clear timers/transfers/rooms, and exit.

The Docker image declares `STOPSIGNAL SIGTERM`; Coolify stop grace is at least
15 seconds. `/healthz` remains 200 while the process is live so shutdown does
not trigger a restart race; new upgrades still receive 503. There is no attempt
to migrate rooms to a new process.

### Container and Coolify release contract

- Pinned Bun image and frozen production dependencies.
- Non-root runtime user, read-only-root-filesystem compatibility, writable
  `/tmp` only when Bun requires it, no volume, and no credentials in image
  layers.
- OCI source/revision/version labels supplied at build time.
- Exact HTTP/WS port 3001, public TLS/WSS at the proxy, five-minute-or-longer
  idle timeout, and one replica.
- Health path `/healthz`; metrics path is private/token-protected when enabled.
- Explicit CPU/memory/connection limits in Coolify rather than unbounded host
  defaults.
- No `DATABASE_URL`, Better Auth secret, signing key, IR bearer token, cookie
  secret, or certificate in the Arena resource.
- A production environment example containing no secret values, an operator
  runbook, and a credential-free post-deploy anonymous WSS smoke.

## Error and recovery behavior

- Missing/corrupt placement file: use defaults; do not block Arena or overwrite
  the file until a user placement action succeeds.
- Invalid stored rectangle: skip only that entry and use the descriptor default.
- Viewport too small: shrink to the safe minimum that fits, keep the compact
  content scrollable, and retain the original normalized record.
- Active profile change: Phase 1 leaves the room; placement store switches to
  the new profile and the old profile's file remains untouched.
- Skin/layout change between rounds: resolve a different descriptor key and use
  its stored/default rectangle.
- Screen/round destroyed during customization: restore runner input before
  clearing the frame/session state.
- Chat opened while customizing: commit/leave customization first, then open.
- Server metrics disabled/misconfigured: disabled is valid; enabled with a
  missing/short token fails startup rather than exposing a public endpoint.
- Untrusted forwarded headers: ignore them and rate-limit by direct peer.
- Address/connection limit: reject upgrade with 429/503 before allocating room
  state; existing sockets continue.
- Hello timeout: close only that socket and release all counters.
- Metrics serialization failure: return 500 without counter bodies in logs;
  gameplay and room state continue.
- Deployment shutdown: clients receive going-away when possible and return to
  Browser; all room state is intentionally lost.
- Malformed/flood traffic: existing strict schema/size/limiter rules close or
  reject it without changing a valid room or growing unbounded buffers.

## Security and privacy

- Placement files contain only normalized geometry, descriptor keys, version,
  and hint version. They contain no remote/session data.
- Remote text remains plain text at every Default/legacy surface.
- Overlay pointer/focus handlers cannot invoke protocol commands except the
  explicit existing ready/chat/kick/leave actions.
- Customization cannot change chart transformation, gauge, target preference,
  saved score, or room state.
- Raw IP/forwarding headers, metrics token, Authorization header, room/member/
  user IDs, telemetry/results, chat, inventory hashes, file paths, tickets,
  resume tokens, and passwords stay out of logs and metrics.
- Metrics use only closed low-cardinality labels. Unknown error text is counted
  under `other`, not turned into a label.
- `/metrics` uses a constant-time token comparison and no-store response.
- The official service remains WSS-only outside explicit loopback tests.
- The Arena container still receives public JWKS configuration only and has no
  IR database/score-upload authority.

## Testing strategy

### Client unit tests

- Placement key validation, deterministic JSON ordering, atomic write, active-
  profile isolation, corrupted entry/root/future version, reset, hint version,
  finite/bounds checks, and no remote data in the file.
- Normalized-to-pixel/default/clamp behavior at 4:3, 16:9, 21:9, 1280x720,
  1920x1080, 2560x1080, and 3840x2160.
- Drag/resize end commit, eight handles, minimum size, safe margin, keyboard
  nudge/resize/reset, Escape/F2/Enter, and no write on passive viewport clamp.
- Descriptor separation across profile, skin ID, k5/k7/k10/k14/result, and
  gameplay/result kind.
- Forced compact visibility and absence of a hide path.
- F2 ownership: Default local shortcut disabled in Arena, coordinated Default
  customize mode, legacy pointer shield, and restoration on every cleanup.
- Runner suppression releases held lanes once, ignores input without pausing,
  restores idempotently, and leaves non-Arena runners unchanged.
- Chat attachment direction, unread behavior, focus/Escape priority, no pause,
  and all content inside the viewport.
- Native capability selection: Default native select/result; LR2/Beatoraja
  fallback overlays; unknown third-party theme fallback.
- Plain-text and accessibility contracts for remote labels and controls.
- Existing Arena session/result/pacemaker and non-Arena gameplay tests remain
  green.

### Server unit/integration tests

- Trusted/untrusted direct peer, IPv4/IPv6/CIDR matching, multiple proxy chain,
  eight-entry/512-byte limit, malformed header fallback, and no raw address in
  diagnostics.
- Upgrade attempts/minute, concurrent-address count, exact hello timeout,
  release on upgrade failure/normal close/policy close/shutdown, and global
  capacity interaction.
- Metrics exact names/types, fixed labels/buckets, counter/gauge transitions,
  no high-cardinality values, disabled 404, missing/wrong/right bearer, no-
  store, constant-time helper, and startup failure for weak configuration.
- Existing chat/password/inventory/telemetry/result limits under mixed traffic.
- Malformed JSON/binary, maximum legal messages, slow reader/backpressure,
  invalid capability/order/revision, and no mutation after rejection.
- Repeated SIGTERM, upgrade rejection, going-away ordering, bounded drain,
  1012 close, timer/room/buffer release, and health behavior.

### QML and representative-skin tests

A small Qt Quick Test target loads application-owned components with fake
session/models. It covers placement math, focus, keyboard controls, drawer
attachment, native-capability fallback, forced visibility, and 16-row clipping.
Static/lint gates reject `Text.AutoText` on remote fields and missing accessible
names on interactive Phase 4 controls.

Manual/visual matrix:

- Default select/gameplay/result at k5/k7/k10/k14.
- One representative LR2 and one representative Beatoraja select/gameplay/
  result configuration.
- 4:3, 16:9, and 21:9; 100%, 150%, and 200% scale.
- F2 move/resize/reset, restart persistence, profile isolation, skin/layout
  isolation, viewport clamp, first-use hint, compact/expanded, and 16 seats.
- Chat F8/Enter/Shift+Enter/Escape, drawer flip, unread badge, controller play,
  customization input suppression, and no pause.
- Pending result, joint winners, DNF, lobby wins, Default ranking-source cycle,
  legacy 179/180 present/absent, and ordinary non-Arena result.

### Cross-repository release verification

- Protocol 1.2 text/binary golden digests remain identical across C++/TS.
- The Phase 3 three-client smoke remains green.
- A Phase 4 malformed corpus drives HTTP, JSON, and binary boundaries without
  room mutation, secret output, crash, or unbounded allocation.
- A bounded soak drives 200 authenticated sockets in 25 eight-seat rooms for
  30 seconds at five-hertz telemetry, selection/chat churn below limits, one
  reconnect per room, and finalization. It records event-loop delay, RSS,
  buffered bytes, dropped ephemeral standings, and cleanup to baseline; release
  gating is invariant/resource-bound rather than a platform-fragile timing
  number.
- Docker runs as non-root with a read-only root filesystem, answers health,
  protects metrics, handles WSS-proxy headers only from configured CIDR,
  survives SIGTERM, and contains no test/private files.
- Public post-deploy smoke checks HTTPS health, WSS anonymous directory, query-
  string rejection, certificate/hostname, and graceful-going-away behavior
  without accepting a credential argument.

## Delivery and traceability

Phase 4 produces one verification report in each repository. Every exit
criterion maps to an automated command, measured load result, visual/manual
observation, or explicit environmental blocker. A missing Docker daemon,
Netcup access, DNS, certificate, or Coolify credential is recorded rather than
reported as success or worked around by weakening authentication.

## Exit criteria

- Default select exposes roster, owner moderation, selection/options, ready,
  last winner/wins, and chat in a native panel without restoring forbidden
  profile/local-battle/replay/autoplay paths.
- LR2 and Beatoraja select expose the same functional room surface through an
  application-owned bounded overlay while retaining their Phase 2 unavailable-
  chart behavior.
- Every active Arena gameplay shows a forced compact live leaderboard; no hide
  setting or off-screen persisted state can remove it.
- F2 moves/resizes/resets the leaderboard in Default, LR2, and Beatoraja;
  Default's existing customize mode participates; keyboard nudging and visible
  handles work.
- Placement persists per profile, resolved skin, layout variant, and overlay
  kind, clamps safely after viewport changes, and stores no room data.
- Arena customization suppresses chart input without pausing and restores it on
  every exit/cleanup path.
- Gameplay chat stays closed by default, deliberately opens without pause,
  attaches/flips inside the viewport, reports unread messages, and closes before
  customization/abort handling.
- Default result shows every winner first, Arena rank/count by default, complete
  standings/wins, and normal internet provider cycling without saved-provider
  mutation.
- LR2/Beatoraja result always has a compact winner/rank overlay, expandable
  standings, F2 placement, and unchanged 179/180 compatibility.
- Remote text is plain, keyboard/focus/accessibility rules pass, and supported
  English-source/Polish Arena strings are complete.
- Proxy-aware admission trusts forwarded addresses only from configured CIDRs,
  enforces address/hello bounds, and logs no raw address.
- Metrics are bounded/low-cardinality, disabled by default, token-protected when
  enabled, and contain no player/session data.
- The pinned non-root single-replica container passes health, malformed/load/
  privacy/read-only-root/SIGTERM gates and has an exact Coolify runbook and
  anonymous WSS smoke.
- Phase 1-3 Arena, local score/IR upload, Default/LR2/Beatoraja non-Arena, local
  battle, and ordinary target/ranking regression suites remain green or report
  a concrete environmental blocker without a false success claim.
