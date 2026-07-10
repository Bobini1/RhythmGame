# Online Arena Phase 3 design

## Status and authority

This specification derives from the approved Online Arena umbrella design and
the accepted Phase 1 and Phase 2 designs. The user authorized autonomous
assumptions, implementation, and continuation without another question or
approval gate. If this document conflicts with the umbrella design, the
umbrella design wins; otherwise the exact contracts below control Phase 3.

Phase 2 is a prerequisite. This design extends its protocol 1.1
`rounds-v1` contract, frozen round snapshot, deterministic loader, held
`ChartRunner` start, `ArenaSession::roundRunnerStarted(roundId, runner)` seam,
normal select integration, and room state ending in Playing. It does not
redefine inventory, availability, selection, or synchronized-start behavior.

## Goal

Complete one Arena round after synchronized start:

- Sample the existing local runner at five updates per second without sending
  individual judgements.
- Publish authoritative, revisioned live standings to every room seat.
- Force the strongest other participant's latest EX score as a transient
  gameplay pacemaker without touching saved profile settings.
- Accept a deliberate gameplay-chat action without pausing the chart.
- Capture the completed local result only after the normal local save and IR
  upload path has started, then submit it to Arena independently.
- Resolve local abort, disconnect, grace expiry, leave, kick, and play deadline
  into deterministic finished or DNF terminal states.
- Finalize with EX-only competition ranking, joint winners, temporary lobby
  wins, and one ephemeral last-result snapshot.
- Expose the result through Default's ranking-position widget, LR2/Beatoraja
  result numbers 179/180, and a minimal universal result overlay.

Phase 3 is complete when a three-client round can show coalesced live
competition, survive one reconnect, finalize a `1, 1, 3` EX-score tie, update
both winners' room-lifetime wins, and display the result while each client's
ordinary local score and IR submission remain independent.

## Scope

Included:

- Protocol 1.2 negotiation and the `competition-v1` capability.
- A deterministic chart-length agreement at the Phase 2 load barrier and a
  bounded Playing deadline.
- Five-hertz client sampling, ingress limiting, server coalescing, ephemeral
  backpressure behavior, full live standings, and resume snapshots.
- Strongest-opponent target calculation and Default/LR2/Beatoraja target
  adapters.
- Reliable final-result and explicit-DNF submission, terminal idempotence,
  disconnect grace, deadline DNF, and atomic finalization.
- EX-only competition ranks, joint winners, room-lifetime lobby wins, and one
  last-result snapshot.
- Normal result navigation capture after `ChartRunner::finish()` and explicit
  structural independence from IR upload success.
- A gameplay chat action plus functional, always-on universal gameplay and
  result overlays.
- Default transient Arena result source and LR2/Beatoraja result values 179 and
  180.

Excluded except for stable interfaces consumed by Phase 4:

- F2 drag/resize, normalized placement persistence, per-profile/per-skin/per-
  layout geometry, keyboard nudging, reset placement, and viewport clamping.
- Polished Default-integrated player/chat/result layouts and full legacy-skin
  overlay visual treatment.
- Spectators, voice chat, replays of opponents, score attestation, persistent
  match history, matchmaking, alternate ranking rules, or an Arena-specific IR
  score endpoint.
- Any new field in `ChartData`, any saved Arena pacemaker in `GeneralVars`, or
  any fabricated Arena entry/provider in `OnlineRankingModel`.

Phase 3's fixed overlay is an integration milestone. Phase 4 replaces its
placement wrapper and presentation, not its models, result semantics, target,
chat action, or networking.

## Autonomous assumptions

1. Protocol major remains 1 and current minor becomes 2. Capabilities are
   canonically ordered `rooms-v1`, `rounds-v1`, `competition-v1`; each later
   capability requires every earlier one.
2. After Phase 3 deployment, create, join, and resume require
   `competition-v1`. Minor-0 and minor-1 clients may browse but show update-
   required for admission.
3. The official client first sends minor 2 and all three capabilities. An
   initial `protocol_incompatible` from an older strict server causes one
   anonymous minor-0 `rooms-v1` reconnect. That connection is browse-only and
   the fallback never loops.
4. IR Arena tickets advertise protocol minor 2. Ticket verification continues
   accepting any nonnegative safe minor; negotiated capabilities decide access.
5. Telemetry sampling and room broadcasts use a 200 ms interval. A participant
   sends no more than one newest sample per tick and the server stores only its
   newest accepted sample.
6. EX score is an integer and equals `2 * perfect + great`. Standard judgement
   detail is Perfect, Great, Good, Bad, Poor, and Empty Poor; engine-internal LN
   and mine events are not transmitted.
7. Exact chart length is reported after deterministic loading as
   `ceil(chartLengthNanoseconds / 1,000,000)`. The accepted range is 0 through
   21,600,000 ms (six hours).
8. The Playing deadline is the agreed chart length plus two minutes, clamped to
   three minutes through six hours and two minutes from scheduled server start.
9. A zero-score telemetry/final is valid. Absence of telemetry is distinct from
   a score of zero.
10. A local abort is DNF for Arena even when the existing local UI produces and
    saves a partial local score. That local score follows existing IR behavior
    but is never submitted as an Arena final.
11. The local result screen opens immediately after a normal finish and may show
    "waiting for players" until server finalization. Result is not a server room
    phase.
12. Competition ranking is `1 + number of valid final EX scores strictly
    greater`; exact ties therefore produce `1, 1, 3`.
13. Every rank-1 player wins. Lobby wins increment only for frozen rosters of at
    least two and saturate at unsigned 32-bit maximum.
14. The universal compact gameplay overlay is forced on in Arena. Users may
    expand/collapse detail but cannot hide it in Phase 3.
15. F8 is the initial deliberate gameplay-chat toggle. Escape closes an open
    chat drawer before any gameplay-abort action. Opening chat never pauses the
    runner.
16. The fixed Phase 3 gameplay overlay uses the top-right safe area. No geometry
    is persisted until Phase 4.

## Design choices

### Coalesced snapshots, not judgement events

Three options were considered: relay every judgement, send client peer-to-peer
streams, or send coalesced score snapshots. Per-judgement traffic creates burst
load and exposes engine timing detail that standings do not need. Peer traffic
adds NAT, trust, and reconnect complexity while the server must still finalize
results. The chosen 200 ms snapshot keeps the server authoritative for room
ordering and is sufficient for a live BMS leaderboard.

### Central result capture, not a second finish path

Arena does not add an alternate `ChartRunner::finish()` or wait for IR. Both
Default and LR2/Beatoraja already call the application-owned
`ContentFrame.openResult(scores, profiles, chartData)` after `finish()` has
saved locally and started the normal profile submission. ContentFrame invokes
`ArenaSession::submitLocalResult(scores[0])` immediately before pushing the
existing result screen. Arena copies bounded values synchronously and never
owns, delays, proxies, or observes the IR network reply.

### Transient target model, not profile mutation

`ArenaOpponentTarget` is derived from authoritative standings. Default and
legacy gameplay check it before their saved-score/fraction target logic. No
`GeneralVars` assignment, temporary restoration, fake `BmsScore`, or
`ScoreReplayer` is used. Leaving Arena simply removes the transient source and
reveals the unchanged saved target.

## Module shape

### Server

`RoomDirectory` remains the application-facing deep module. Phase 3 extends
the Phase 2 internal `round-state.ts` Playing branch and adds two focused
internal modules:

- `rooms/standings.ts` validates immutable telemetry/finals, computes live and
  final competition ranks, orders rows, selects winners, and creates bounded
  public snapshots. It knows no sockets, clocks, passwords, or IR behavior.
- `rooms/telemetry-limiter.ts` owns the per-frozen-seat token bucket and bounded
  violation window.
- `round-state.ts` owns frozen identities, participant terminal state, Playing
  deadline, standings/result revisions, finalization, and last result.
- `ArenaApplication` performs connection/capability/binding preflight and maps
  domain effects to reliable or ephemeral deliveries.
- The gateway coalesces due room snapshots and may drop only ephemeral live
  standings under backpressure. Terminal acknowledgement, finalization, room
  snapshots, chat, and lifecycle messages remain reliable.

Arena never imports or calls an IR score-upload module.

### Client

`ArenaSession` remains the only QML-facing Arena networking module.

Internal seams:

- `ArenaGameplaySource` attaches to the current Phase 2 `ChartRunner`, samples
  its local P1 `BmsLiveScore`, captures a bounded final from `BmsScore`, and has
  a production Qt adapter plus deterministic fake.
- `ArenaStandingsModel` is a value-row `QAbstractListModel` used for current live
  standings and nested result standings.
- `ArenaResultModel` owns result metadata plus its standings model; Session
  creates separate last-result and currently-presented instances.
- `ArenaOpponentTarget` derives one stable strongest-other value from the live
  model.
- `ArenaOverlayHost.qml` selects the fixed gameplay/result overlay above the
  current skin and owns the deliberate chat action.

QML never samples score internals, parses competition JSON, computes ranks,
chooses a winner, correlates terminal retries, or mutates profile target state.

## Protocol evolution and admission

Constants:

```text
protocolMajor = 1
protocolMinor = 2
capabilities = rooms-v1, rounds-v1, competition-v1
```

The 1.2 server accepts hello minor 0, 1, or 2 and returns negotiated minor
`min(clientMinor, 2)`. Usable capabilities are the canonical intersection of
the client list, server list, and that negotiated minor's ceiling:

```text
minor 0: rooms-v1
minor 1: rooms-v1, rounds-v1
minor 2: rooms-v1, rounds-v1, competition-v1
```

Thus a client cannot claim a newer feature while reporting an older minor.
`competition-v1` is valid only when `rounds-v1` and `rooms-v1` are also
advertised; `rounds-v1` still requires `rooms-v1`.

Create/join without negotiated `competition-v1` returns command error
`competition_capability_required` without mutation. A hello resume without it
returns a successful hello with resume status `failed`, code
`competition_capability_required`; it does not consume or rotate the reserved
seat token. Anonymous and authenticated directory browsing remain available.

New client message variants:

```text
round_telemetry
round_result_submit
round_abandon
```

New server variants:

```text
round_standings
round_terminal_accepted
round_finalized
```

The new messages use the existing strict `{ type, requestId?, data }`
envelope. Their exact top-level data is:

```text
round_telemetry (uncorrelated)
  data: roomId, roomGeneration, connectionGeneration,
        roundId, launchAttemptId, telemetry: ArenaTelemetry

round_result_submit (correlated)
  requestId
  data: roomId, roomGeneration, connectionGeneration,
        roundId, launchAttemptId, result: ArenaFinalResult

round_abandon (correlated)
  requestId
  data: roomId, roomGeneration, connectionGeneration,
        roundId, launchAttemptId,
        reason: aborted | result_unavailable

round_standings (uncorrelated event)
  data: roomId, roomGeneration, roundId, launchAttemptId,
        standingsRevision, entries: LiveStandingEntry[]

round_terminal_accepted (correlated response)
  requestId
  data: roomId, roomGeneration, roundId, launchAttemptId,
        terminal: finished | dnf

round_finalized (uncorrelated reliable event)
  data: roomId, roomGeneration, roundId, launchAttemptId,
        result: RoundResultSnapshot,
        members: complete post-finalization Member[]
```

IDs and generations reuse the Phase 2 bounds. Arrays are unique by member ID,
frozen-roster ordered where specified below, and capped at 16. Unknown keys at
the envelope, data, nested-value, or union-branch level are invalid.

Every protocol-1.2 `RoomSnapshot` has required nullable fields
`liveStandings: LiveStandingsSnapshot | null` and
`lastRoundResult: RoundResultSnapshot | null`; absence is not interchangeable
with null. `liveStandings` is non-null only for the active Playing round.
Protocol-1.2 frozen participant rows carry immutable `PublicIdentity`, and
public `Member.lobbyWins` is bounded to `uint32`.

Protocol 1.2 also extends the Phase 2 `round_load_result(ok=true)` with
`chartLengthMs` and extends frozen participants with their immutable
`PublicIdentity`. A `competition-v1` connection must supply/accept those fields;
they are absent from minor-1 browse-only traffic.

Every competition command repeats the current room ID/generation, connection
generation, round ID, and launch-attempt ID. The server accepts it only from the
current binding of a frozen participant. A waiting member can receive standings
but cannot submit telemetry or a terminal state.

## Chart-length agreement and Playing deadline

Each successful Phase 3 load response reports:

```text
chartLengthMs: integer 0..21600000
```

The Qt loader derives it from the loaded runner, not remote metadata. After all
clients report loaded, the server requires exact equality. A mismatch cancels
the launch with `chart_length_mismatch`, clears the selection, increments
`selectionRevision`, and clears every ready flag through Phase 2's consistency-
failure cancellation branch.

When start is scheduled:

```text
playDeadlineAtServerMs = startAtServerMs
  + clamp(chartLengthMs + 120000, 180000, 21720000)
```

`round_start_scheduled`, `round_started`, active-round snapshots, and resume
state include the deadline. At `now >= playDeadlineAtServerMs`, the domain marks
every nonterminal participant DNF before processing a command stamped at that
same time, then finalizes if possible. The existing exact earliest-deadline
scheduler services it.

## Five-hertz telemetry

### Exact wire value

`round_telemetry` has no request ID and receives no per-packet acknowledgement:

```text
roomId, roomGeneration, connectionGeneration
roundId, launchAttemptId
sequence: integer 1..4294967295
exScore: integer 0..100000000
progressPermille: integer 0..1000
maxCombo: integer 0..100000000
badPoorCount: integer 0..100000000
judgements:
  perfect, great, good, bad, poor, emptyPoor
  each integer 0..100000000
gauge:
  type: fc | exhard | hard | normal | easy | aeasy
  valueMilli: integer 0..100000
playStatus: literal playing
```

The server requires:

```text
exScore == 2 * perfect + great
badPoorCount == bad + poor + emptyPoor
```

For one participant, sequence, EX, progress, max combo, bad/poor, and each
judgement count may not decrease. Gauge may increase or decrease. Duplicate or
older sequences are silently ignored and do not dirty standings. Final state
supersedes telemetry; every later telemetry packet is ignored.

### Client sampler

When the Phase 2 session emits `roundRunnerStarted`, the gameplay source stores
the expected local score GUID and P1 runner pointer. Every 200 ms while the
runner is Running, `ArenaSession` samples one owned value:

- EX from `BmsLiveScore::points`, requiring a finite exact integer.
- Progress from `Player::elapsed / Player::chartLength`, clamped and rounded to
  permille. A zero-length chart reports `0` rather than dividing by zero; it
  normally reaches finalization before another sample.
- Max combo and the six public judgement counts.
- BP as Bad + Poor + Empty Poor.
- The displayed gauge: first gauge strictly above threshold, otherwise the last
  gauge, matching current gameplay presentation; gauge value is normalized to
  its 0..100,000 milli range.

The timer reads only on the runner's QObject thread. During socket loss Session
keeps at most one newest unsent sample and continues sequence numbering. On a
successful resume of the same round it sends that newest sample; stale-round
samples are discarded. Sampling stops at runner Finished, local abandon,
profile change, leave, cancellation, or destruction.

Cadence is anchored to a monotonic `nextSampleAt`. After a delayed callback,
Session samples once, advances by whole 200 ms quanta until the next deadline
is in the future, and never emits catch-up bursts.

Normal runner Finished cancels only the sampling timer: the source association
and expected P1 GUID remain alive until the immediately following central
`openResult` capture succeeds/fails or a cleanup path wins. This avoids a
Finished-to-result race. Local abandon may detach score sampling, but Session
retains the expected GUID long enough to recognize the partial result as the
same Arena DNF presentation.

### Server ingress limit

Each frozen seat owns an integer token bucket:

```text
capacity = 10
initialTokens = 10
refill = 1 token per 200 ms
cost = 1 accepted-shape telemetry attempt
```

Excess valid telemetry is silently dropped. The server records at most 20
excess or semantically out-of-phase attempt timestamps in a rolling 10-second
window. The twentieth violation closes that connection with WebSocket policy
code 1008 and redacted reason `rate_limited`. Duplicate/old sequence packets do
not count as violations. Limiter state follows the frozen seat across resume
and is removed at finalization/room destruction.

Ordering is explicit: strict decode and current room/round/binding checks run
first; stale binding/round and duplicate/older sequence are ignored without a
token or violation. A current, newer telemetry attempt consumes one token. A
no-token attempt records one violation. With a token, a nonterminal Playing
participant whose semantic counters do not regress is accepted; a
pre-Playing/current-round or regressing attempt records one violation instead.
Packets already in flight after that participant became terminal are ignored
without a violation. Schema/EX/BP failures remain malformed input, not limiter
events.

## Authoritative live standings

Accepted telemetry, connection-status change, final result, or DNF increments
the active round's positive `standingsRevision` and marks the room dirty.
`round_standings` is always a complete snapshot, never a delta:

```text
roomId, roomGeneration, roundId, launchAttemptId
standingsRevision: positive safe integer
entries: frozen-roster rows, maximum 16
```

Rows contain `memberId`, connected/reserved status, competition state, nullable
rank, and one strict union:

- `loading` or `playing`: optional newest telemetry.
- `finished`: immutable final result.
- `dnf`: no competitive score and rank null.

Frozen public identities are sent once in the active-round snapshot, not five
times per second. Final results contain identity copies so departed players
remain identifiable.

Live ranks include active rows with telemetry and finished rows. A real zero
score ranks; an active row without telemetry does not. Ordering is:

1. Ranked rows by EX descending.
2. Exact-score ties in frozen roster order.
3. Active rows without telemetry in frozen roster order.
4. DNF rows in frozen roster order.

The first dirty snapshot may publish immediately. Later snapshots publish no
more often than every 200 ms; skipped revisions are legal because every event
is complete. An unchanged room emits nothing.

`Delivery` gains `send_ephemeral`. If a socket is backpressured, the gateway
drops only later ephemeral standings for it until drain. It never queues an
unbounded backlog and never silently drops finalization or control messages.

The gateway's per-socket reliable high-water becomes 5 MiB, above the 4 MiB
maximum single server message. Before a send it compares exact encoded UTF-8
bytes with current buffered bytes. Ephemeral delivery that would cross the
high-water is dropped. Reliable delivery that would cross it closes that slow
socket with code 1013 and redacted reason `try_again_later`; the application
then reserves the seat normally. Resume supplies the authoritative room
snapshot, including terminal state/last result. Thus every legal maximum event
fits on an empty writable socket, while a client that does not drain receives
bounded close-and-resume recovery rather than an unbounded queue or a claimed
delivery that never occurred. The next full snapshot or resume snapshot also
repairs a dropped live update.

## Strongest-opponent pacemaker

For the local frozen member, `ArenaOpponentTarget` considers every other row
with accepted telemetry or an immutable final, excludes DNF, and selects the
highest current EX score. Finished scores remain candidates.

At an exact tie it retains the current target if still tied; otherwise it picks
the earliest frozen-roster member. This prevents visual target flapping while
remaining deterministic. Its interface is:

```text
available
memberId, displayName
exScore
finished
```

Default uses `exScore` for both its current point target and transient final
target. LR2/Beatoraja `gameplayTargetScorePoints` and
`gameplayTargetFinalPoints` return the same transient value. If no opponent has
data, the target is explicitly unavailable/zero; Arena does not fall back to
Best Score, Last Score, Next Rank, or Fraction. Saved target/ghost settings and
score replayers continue unchanged in the background and reappear after Arena.

## Terminal submission and DNF

### Normal local/IR result path

Existing gameplay calls `ChartRunner::finish()`. `Player::finish()` saves the
`BmsScore` locally and, for a logged-in profile, starts
`Profile::submitScore(...)` exactly as it does outside Arena. No Phase 3 code is
inserted before or inside that path.

The subsequent central `openResult` call invokes:

```cpp
ArenaSession::submitLocalResult(gameplay_logic::BmsScore* score)
```

Session accepts only the expected P1 GUID from the current frozen round and
copies bounded result values immediately. If that expected score follows an
already-recorded local abandon, Session opens the DNF result presentation but
does not copy or submit the partial score as an Arena final. It neither reads
nor waits for `BmsScore::submissionState`. IR success, failure, duplication,
retry, and later synchronization cannot change the Arena terminal state; Arena
connection failure cannot prevent the local save/IR attempt.

### Final-result wire value

`round_result_submit` is request-correlated and carries the full binding plus:

```text
exScore, maxCombo, badPoorCount
judgements: perfect, great, good, bad, poor, emptyPoor
clearType: max | perfect | fc | exhard | hard | normal | easy | aeasy | failed
finalGauge: GaugeSnapshot
```

It contains no score GUID, replay, hit timing, file path, chart hash, IR upload
state, or IR response. The same EX/BP semantic equations and numeric bounds as
telemetry apply. A final may not regress below the participant's last accepted
telemetry EX, max combo, BP, or any of the six judgement counts. Progress has
no final-result counterpart; clear and final gauge are validated
independently.

If local capture is impossible, Session sends `round_abandon` with
`result_unavailable`. A deliberate local gameplay abort sends it with
`aborted`. Both are correlated, binding-complete commands.

Public DNF reasons are:

```text
aborted | result_unavailable | left | kicked | grace_expired | play_deadline
```

### First-terminal-wins and retry

The first accepted final/DNF state is immutable.

- An identical final retry or identical abandon reason is acknowledged
  idempotently with `round_terminal_accepted`.
- A conflicting final/abandon returns `round_already_terminal`.
- Before terminal acceptance, final/abandon attempts are limited to eight per
  frozen seat per rolling minute.
- The ninth otherwise well-formed preterminal attempt returns correlated
  `rate_limited` without mutation. Schema-invalid attempts follow the normal
  malformed-message policy; identical retries after terminal acceptance are
  not charged to this preterminal limiter.
- A queued local final survives socket reconnect in process memory only.
  It retains the same request ID and byte-equivalent terminal result/reason;
  resend rewrites only the current connection-generation binding. Resume state
  decides whether to resend it; if the snapshot already shows the member
  terminal or the round finalized, the queue is dropped.

Disconnect alone marks the live row reserved/disconnected and starts existing
seat grace; it is not DNF. Resume before grace continues the same participant.
Grace expiry marks DNF. Explicit leave or kick marks an unfinished frozen
participant DNF before removing the seat. A terminal finished result is never
rewritten by later leave/kick. Waiting-member lifecycle never affects the
frozen result.

## Finalization, ranks, winners, and wins

The server finalizes exactly once when every frozen participant is Finished or
DNF. The play deadline first converts every unresolved participant to DNF.

Final competition rank is:

```text
rank = 1 + count(finished results with EX strictly greater)
```

Only final EX matters. Gauge, clear, combo, BP, judgements, connection state,
and selected options are detail. Finished rows sort by EX descending with
frozen order for exact ties. DNF follows with `rank: null`. All-DNF rounds have
no winner. Frozen roster size, including DNF, is participant count.

Every rank-1 member is a winner. When participant count is at least two, each
winner whose seat still exists (connected or reserved) receives one saturating
`uint32` lobby win. A finished player removed before finalization remains a
winner in the result but has no seat on which to retain wins. A sole valid
finisher in a multi-player round wins even when every opponent DNFed. A
one-player round never increments lobby wins.

`RoundResultSnapshot` contains:

```text
resultRevision: positive room-lifetime safe integer
roundId, selectionRevision, finalizedAtServerMs, participantCount
selection: frozen SelectionSnapshot
winnerMemberIds: ordered unique IDs
entries, each with frozen PublicIdentity and:
  finished result + numeric rank, or DNF reason + null rank
  lobbyWinsAfter: uint32 or null when no seat remains
```

Finalization atomically:

1. Computes ranked terminal rows and ordered winner IDs without publishing.
2. Awards saturating lobby wins to extant winning seats.
3. Builds the immutable result from the post-award seat state, including
   `lobbyWinsAfter`, and increments `resultRevision` once.
4. Clears active telemetry, limiter state, and ready flags.
5. Returns the room from Playing to Selecting.
6. Makes waiting seats next-round eligible and uses Phase 2 availability rules.
7. Retains the selection only if still common; otherwise clears it through the
   Phase 2 revision rule.
8. Retains the new `lastRoundResult` until a later round finalizes.
9. When a client result/abandon command triggered finalization, emits its
   targeted terminal acknowledgement first. It then emits reliable
   `round_finalized` with the complete post-finalization public member array,
   followed by the directory phase update in one ordered effect batch.

Room snapshots contain the required nullable `liveStandings` and
`lastRoundResult` fields defined above. The previous last result remains during
a later round and is replaced only on that round's finalization. Room
destruction deletes it and all lobby wins.

## Client presentation models and session orchestration

### `ArenaStandingsModel`

The model stores value rows, never per-row QObjects. Roles are:

```text
memberId, displayName, avatarUrl
connected, competitionState
rank (0 means unranked), hasScore
exScore, progressPermille, maxCombo, badPoorCount
perfect, great, good, bad, poor, emptyPoor
gaugeType, gaugeValueMilli, clearType
lobbyWinsAfter, dnfReason
```

For a live snapshot, identity comes from the frozen participant cache. A final
snapshot carries identity directly. Full replacement validates every row and
installs it with one model reset; stale round/revision snapshots do not mutate
the model. Wire-null rank maps to integer `0`; wire-null `lobbyWinsAfter` maps
to `-1` so QML can distinguish a removed seat from zero wins.

### Result models

`ArenaResultModel` owns:

```text
valid, finalized, roundId, resultRevision
participantCount, winnerMemberIds, winnerNames
localRank, localDnf, localWinner
selectionTitle, selectionOptionsSummary
ArenaStandingsModel* standings
```

`selectionOptionsSummary` is a client-local, plain-text rendering of the
frozen P1/P2 note-order and DP mode values; it never relays lane seed or the
realized random sequence to QML. Session exposes the same summary for the
active round so gameplay and result overlays consume one formatting rule.

Session owns `lastResult` (server room state) and `presentedResult` (the local
result screen's round). A normal local finish creates a pending presented
result immediately; `round_finalized` completes both when round IDs match.
Leaving the result screen ends presentation but does not clear last result.

### `ArenaSession` behavior

Session exposes the live model, two result models, opponent target, active
gameplay/result flags, negotiated `competitionAvailable`, and gameplay-chat
open state. It:

- Attaches the gameplay source before emitting Phase 2
  `roundRunnerStarted`.
- Schedules/cancels 200 ms sampling through `ArenaScheduler`.
- Correlates standings and terminal messages by transport generation, room,
  round, attempt, revision, and request.
- Queues at most one final or abandon while reconnecting.
- Replaces state from authoritative resume/room snapshots.
- Clears source, timers, target, chat drawer, queues, and presented context on
  profile change, leave, kick, room loss, or exit.

## Gameplay chat and local abort

`ArenaOverlayHost` installs an application-level F8 shortcut and visible Chat
button only while the local Arena runner is active. `ArenaSession` owns
`gameplayChatOpen`; opening it exposes the existing bounded `ArenaChatModel` and
`sendChat()` path. It never changes room/runner phase or pauses a timer.

The fixed drawer accepts plain text only. Enter sends, Shift+Enter inserts a
line break only if the existing chat schema permits it, and Escape closes the
drawer. While its editor has focus, keyboard events are accepted by the drawer
instead of gameplay; non-keyboard play input remains live. Default and legacy
gameplay abort handlers are disabled while the drawer is open.

Every new QML label that can contain a remote display name or chat body sets
`textFormat: Text.PlainText` explicitly; using a `Text` item with its default
auto-detection is not sufficient.

Both gameplay families call `ArenaSession::abandonCurrentRound()` before their
existing Escape/stop behavior. Session immediately marks the local terminal as
pending DNF, stops telemetry, and sends or queues `round_abandon(aborted)`.
Any partial local `BmsScore` produced afterward is ignored for Arena but retains
normal local/IR behavior.

## Minimal universal gameplay overlay

`ArenaGameplayOverlay.qml` is application-owned, above the configured gameplay
skin, and always visible for an active Arena runner.

Compact mode shows rank, player name, EX, progress, and connected/playing/
finished/DNF state for every frozen participant. Expanded mode adds BP, max
combo, six judgements, gauge, and the frozen random/DP option summary. It also
contains Expand and Chat actions. Untrusted names/chat render as plain text.

Phase 3 anchors a bounded panel to the top-right safe margin and prevents it
from capturing gameplay pointer input outside its controls. It exposes
`placementKind`, `resolvedSkinId`, and `layoutVariant` properties to a wrapper,
but does not read/write placement settings. Phase 4 wraps the same content in
drag/resize/persistence behavior.

The panel is at most 420 px wide and at most the viewport height minus two
24 px safe margins. Participant/detail and chat bodies use clipped, scrollable
views so a 16-seat room cannot extend off-screen.

## Pacemaker integration

- Default `Gameplay.qml` checks `ArenaOpponentTarget.available` before local
  battle, saved-score, Next Rank, or Fraction target paths. It uses the Arena EX
  for current/final target and leaves `generalVars.scoreTarget` untouched.
- LR2/Beatoraja `gameplayTargetScorePoints` and
  `gameplayTargetFinalPoints` perform the same first check in
  `Lr2SkinScreenWrapper.qml`. Existing value-resolver formulas then work
  without a fake replay or ranking entry.
- No-opponent state is zero plus an explicit availability property for the
  universal overlay. Leaving/round cleanup clears it synchronously.

## Result navigation and universal overlay

The existing normal result screen opens immediately. The application overlay
uses `presentedResult` and initially shows pending status; when finalized it
shows winner names, competition ranks, complete standings, DNF, and current
lobby wins. It remains functional even when the skin renders no Arena-specific
field.

The result overlay is fixed and always on for an Arena result in Phase 3. It
exposes the same placement descriptor interface as gameplay, but Phase 4 owns
customization and full visual integration.

Exiting result pops to the existing Arena select below it. The server may
already be Selecting, but the member is unready; no next round can freeze until
every eligible member explicitly readies again or leaves.

## Default transient Arena ranking source

Arena is not added to `OnlineRankingModel::Provider`. Default result owns a
screen-local `effectiveResultSource` with this cycle:

```text
arena -> RhythmGame -> Tachi -> LR2IR -> arena
```

Every Arena result initializes it to Arena. Cycling in either direction changes
only the local result screen. It never writes `generalVars.rankingProvider`; a
non-Arena result keeps the existing saved-provider behavior.

While Arena is selected:

- RankingPosition reads `presentedResult.localRank` and participant count.
- Pending result shows loading; DNF/final unranked renders a dash.
- The old-position field and delta arrow are hidden.
- There is no external URL and IR submission failure does not dim Arena rank.
- The existing RhythmGame/Tachi/LR2IR query objects remain alive so their
  upload/query refresh proceeds normally and is immediately visible when
  selected.

The next Arena result starts on Arena again regardless of the prior result's
last transient source.

## LR2 and Beatoraja result values

During an active Arena result presentation:

```text
number 179 = finalized local competition rank, or 0 while pending/DNF
number 180 = frozen result participant count, including DNF
```

The override lives in `Lr2SkinValueResolver.qml` before its existing
`Lr2RankingState` values. Outside Arena result presentation, both numbers keep
their current internet-ranking semantics. No Arena row is inserted into
`OnlineRankingModel`.

## Error and recovery behavior

- Invalid local telemetry sample: skip the sample, keep gameplay running, and
  retain a bounded local diagnostic with no score payload.
- Dropped live standings: retain the previous model; the next complete event or
  resume snapshot repairs it.
- Socket loss while Playing: local gameplay and sampling continue; keep one
  newest unsent telemetry and one terminal payload, then resume within grace.
- Resume after server already recorded terminal/finalization: trust the
  snapshot and discard the local retry.
- Local final capture failure: submit `result_unavailable` DNF; never block the
  local result screen or IR submission.
- IR upload failure: display existing IR state only on IR sources; Arena result
  and server finalization remain valid.
- Arena terminal rejection as stale: request/use authoritative room snapshot;
  never re-finish or re-upload the local score.
- Local abort: send/queue DNF and preserve the rest of the room round.
- Grace expiry, explicit leave/kick, or deadline: server resolves DNF and
  finalizes other participants normally.
- Server loss beyond grace: normal local score remains saved/submittable; Arena
  returns to Browser under Phase 1 recovery and does not fabricate a winner.
- Late events are ignored by connection generation, room generation, round,
  attempt, standings/result revision, sequence, and request as applicable.

## Security, privacy, and resource limits

- Official transport remains WSS. No telemetry/result data appears in a URL.
- Arena is authoritative coordination, not score attestation. A modified
  authenticated client can lie; IR independently applies its own score rules.
- Client JSON stays capped at 65,536 UTF-8 bytes and general server JSON at
  4 MiB. Encoded `round_standings` is additionally capped at 65,536 bytes;
  encoded `RoundResultSnapshot` at 262,144 bytes; and the complete
  `round_finalized` event at 524,288 bytes. The larger one-shot bounds are
  required because a legal Phase 2 selection may retain 4,096 realized
  `#RANDOM` values and finalization also carries bounded frozen identities and
  the post-finalization member array. No valid maximum-shape value may fail
  finalization because of these caps.
- Qt's WebSocket incoming frame and message limits remain the 4 MiB
  `MaxServerMessageBytes`, not the 65,536-byte client-outbound/binary-transfer
  limit. The stricter per-variant decoder caps run after transport delivery.
- Maximums remain 16 seats per room, with at most 16 newest telemetry records
  per active round and one retained final result per room.
- Process defaults add 1,000 rooms and 5,000 WebSocket connections; admission
  over the room limit returns `server_capacity` without disclosing counts. A
  WebSocket upgrade over the connection limit receives HTTP 503 before Arena
  protocol state is allocated.
- Outgoing buffered data has an exact 5 MiB per-socket high-water. Ephemeral
  events drop before it; reliable overflow closes/reserves the seat for resume.
- Telemetry uses the exact token bucket/20-timestamp violation bound above.
  Terminal attempts use at most eight timestamps per frozen seat in a rolling
  minute, follow that seat across resume, and clear at finalization/destruction.
- No telemetry/result body, EX score, judgement, gauge value, display name,
  avatar, user/member ID, score GUID, IR status, or queued terminal appears in
  operational logs or public errors.
- Permitted finalization log fields are opaque room/round IDs, participant/
  finished/DNF/winner counts, duration, and stable error code.
- Frozen identities, telemetry, limiter state, result, and wins remain process-
  local and are released at the specified round/seat/room terminal path.
- UI renders names/chat with explicit `Text.PlainText`. Result metadata never
  supplies rich text.
- Arena server receives no IR database credential or score-upload bearer token
  and never calls the IR score endpoint.

New stable errors/reasons include:

```text
competition_capability_required
result_invalid
round_already_terminal
server_capacity
chart_length_mismatch
```

Malformed schemas remain fatal. Stale/out-of-order telemetry is silently
ignored; rate abuse follows the explicit policy close. Correlated result/DNF
errors leave the socket and room alive.

## Testing strategy

### Server unit and integration tests

- Protocol 0/1/2 negotiation, capability dependency/order, minor-0 legacy
  browse, minor-1 admission/resume rejection, and ticket minor 2.
- Phase 3 positive/strict-invalid text golden parity with C++; Phase 2 fixtures
  remain unchanged.
- Maximum-shape encoding proves a 16-row live snapshot fits 65,536 bytes, a
  legal 4,096-value selection/frozen-identity result fits 262,144 bytes, and
  its complete finalization event fits 524,288 bytes.
- Chart length 0/max, exact agreement/mismatch, deadline clamp, and exact
  deadline-before-command ordering.
- Telemetry bounds, EX/BP equations, monotonic fields, zero score, no-data
  distinction, sequence duplicate/old, final precedence, and stale binding.
- Token bucket burst/refill, rolling violation cap, twentieth policy close, and
  limiter retention through resume.
- Live competition ranks/ties/frozen order, no-data/DNF ordering, complete
  revision snapshots, exact 200 ms coalescing, and no unchanged event.
- Ephemeral drop during backpressure/drain while reliable terminal/finalization
  remains ordered.
- First-terminal-wins, identical retry, conflicting retry, eight/minute limit,
  result regression, all public DNF causes, and waiting-member rejection.
- Disconnect/resume before grace, exact grace expiry, leave/kick before/after
  final, and play deadline.
- Final ranks `1,1,3`, all ties, zero-score winner, all DNF, sole player no win,
  sole finisher among DNF, reserved winner, removed winner, and win saturation.
- Finalization exactly once; member/win snapshot atomicity; immediate Selecting;
  waiting eligibility; selection retain/clear; last-result retain/replace/
  destroy; frozen identity after seat removal.
- Three real WebSocket clients through tie, joint wins, reconnect, DNF, and next
  round; waiting client receives but cannot submit standings.

### Client tests

- Strict codec coverage and byte-identical Phase 3 fixture.
- Gameplay source sample mapping from runner/live score, active-gauge rule,
  finite/integer validation, progress clamp, final mapping, expected GUID, and
  no ownership/profile mutation.
- Exact 200 ms scheduling, one newest pending sample, sequence, stop paths,
  reconnect resend, and stale-round discard using fakes.
- Standings/result model atomic reset, roles, rank zero, identity cache, stale
  revision rejection, and complete final replacement.
- Strongest-other selection, local exclusion, stable tie, target change,
  finished retention, DNF removal, no-data unavailable, and cleanup.
- Terminal queue/idempotence, local result capture in every IR submission state,
  capture failure DNF, abort DNF, authoritative resume, and finalization.
- Normal `ChartRunner::finish()` still saves/submits before ContentFrame's Arena
  capture; no Arena code waits for the reply.
- Default transient source starts Arena, cycles both ways without writing
  `rankingProvider`, hides delta, shows pending/DNF, and keeps online queries.
- LR2/Beatoraja 179/180 Arena override and unchanged non-Arena behavior.
- F8/chat open-close-send, Escape priority, no runner pause, plain text, and
  gameplay abort hook in Default and legacy paths.
- Universal compact/expanded overlay and result pending/final/DNF behavior.
- Normal offline/local battle gameplay/result/targets/ranking regression.

### Cross-repository and deployment tests

- TypeScript/C++ Phase 3 text fixture bytes match; existing Phase 2 binary
  fixtures remain unchanged.
- Scripted three-client round sends approximately 5 Hz telemetry, observes
  the candidate-row changes consumed by client target tests, finalizes
  `1,1,3`, awards two wins, and retains one last result.
- Second run reconnects one participant, abandons another, reaches deadline DNF,
  and finalizes without blocking the valid finisher.
- Docker/Linux smoke verifies coalescing, policy limits, ephemeral backpressure,
  reliable finalization, SIGTERM cleanup, health, non-root runtime, and absence
  of IR/database secrets. Its authenticated container run uses Docker host
  networking plus a host-loopback, in-memory ephemeral Ed25519/JWKS fixture;
  no non-loopback HTTP exception is enabled, no signing key enters the image or
  filesystem, and health is checked before the first JWKS fetch.
- Manual two-app Default/LR2/Beatoraja matrix verifies forced target, chat,
  abort, pending result, winner/ties, Default cycling, 179/180, IR failure
  independence, Arena failure independence, and non-Arena regressions.

## Phase 4 interfaces, not implementations

Phase 3 deliberately preserves:

- `ArenaStandingsModel`, `ArenaResultModel`, and `ArenaOpponentTarget` as the
  only data sources for polished overlays.
- `ArenaGameplayOverlay` and `ArenaResultOverlay` content surfaces with
  `placementKind`, `resolvedSkinId`, and `layoutVariant`, independent of stored
  geometry.
- `gameplayChatOpen`, existing chat model, and one deliberate action so Phase 4
  can restyle/reposition without changing behavior.
- Default's `effectiveResultSource` and legacy 179/180 adapter so visual polish
  does not change ranking semantics.
- No F2 placement preference, normalized rectangle, customize frame, drag/
  resize handler, keyboard nudge, or new profile variable in Phase 3.

The placement descriptors are already canonical: `placementKind` is
`gameplayLeaderboard` or `resultStandings`; `resolvedSkinId` is the resolved
theme-family key from the active profile's existing `themeConfig` lookup; and
`layoutVariant` is the existing resolved screen key (`k5`, `k7`, `k10`,
`k14`, or `result`). They are opaque lookup keys, never user-facing labels.
Phase 4 may version the stored placement record but does not reinterpret these
Phase 3 values.

## Exit criteria

- Protocol 1.2 negotiates `competition-v1`; older clients browse but cannot
  enter a Phase 3 room.
- Every loaded participant agrees on chart length and receives a bounded Playing
  deadline.
- Client telemetry is sampled at 200 ms, bounded/coalesced, and never sends
  per-judgement events.
- Server standings are authoritative full snapshots with live competition ties,
  no-data distinction, final precedence, and bounded backpressure.
- Gameplay always targets the strongest other live/final EX score without
  changing saved target settings; no-opponent state does not fall back.
- Deliberate F8 chat opens without pausing and Escape closes it before abort.
- Normal finish saves and starts ordinary IR upload before Arena copies its
  independent final; either network can fail without blocking the other.
- Disconnect/resume, local abort, leave, kick, grace expiry, and deadline reach
  deterministic terminal behavior without cancelling other players.
- Final EX ranks use `1,1,3`; every joint first wins; DNF has no numeric rank;
  one-player rounds do not increment wins.
- Finalization atomically returns the room to Selecting, updates lobby wins,
  exposes one last result, and deletes all ephemeral state with the room.
- Default starts each Arena result on a transient Arena ranking source and can
  cycle normal providers without changing the saved provider.
- LR2/Beatoraja numbers 179/180 expose Arena rank/count only for the active Arena
  result; universal overlays always show gameplay/result state.
- Focused Bun, Catch2, QML/manual, three-client, Docker/Linux, privacy, and
  non-Arena regression gates pass or report a concrete environmental blocker
  without a false success claim.
