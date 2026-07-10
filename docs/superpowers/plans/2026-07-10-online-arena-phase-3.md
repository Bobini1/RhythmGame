# Online Arena Phase 3 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add bounded live competition, strongest-opponent targeting, independent final/DNF submission, deterministic EX standings, temporary wins, and cross-skin gameplay/result presentation to a Phase 2 Arena round.

**Architecture:** Extend protocol major 1 to negotiated minor 2 with `competition-v1`. The Bun round domain owns newest telemetry, rate state, ranks, terminal participants, finalization, wins, and last result; the gateway only schedules/coalesces effects and drops explicitly ephemeral live snapshots under backpressure. On the client, `ArenaSession` orchestrates an injected gameplay-source adapter plus value-row standings/result models, while application-owned QML overlays and narrow Default/LR2 adapters consume those models without mutating `ChartData`, `GeneralVars`, or `OnlineRankingModel`.

**Tech Stack:** Bun 1.3.14, TypeScript 5.9, Zod 4, WebSocket JSON, Qt 6 C++23, Qt WebSockets/QML, Catch2, CMake/CTest, Docker/Coolify.

## Global Constraints

- The controlling specification is `docs/superpowers/specs/2026-07-10-online-arena-phase-3-design.md`.
- Phase 2 is complete and its inventory/selection/load/start contracts remain intact.
- Protocol major is `1`; current minor is `2`; canonical capabilities are `rooms-v1`, `rounds-v1`, `competition-v1` in dependency order.
- Create, join, and resume require `competition-v1`; minor-0/minor-1 connections are browse-only.
- Client telemetry and server standings coalescing use exactly 200 ms (5 Hz).
- Client text remains capped at 65,536 UTF-8 bytes and server text at 4 MiB.
  Encoded `round_standings`, `RoundResultSnapshot`, and complete
  `round_finalized` values are additionally capped at 65,536, 262,144, and
  524,288 bytes respectively; maximum legal Phase 2 selections and identities
  must fit rather than making finalization fail.
- Qt WebSocket incoming frame/message limits stay at the 4 MiB
  `MaxServerMessageBytes`; 65,536 is not a valid transport-level receive cap
  for the one-shot Phase 3 finalization event.
- Server outgoing buffering uses a 5 MiB per-socket high-water. Ephemeral
  overflow drops; reliable overflow closes with 1013/`try_again_later`, reserves
  the seat, and is repaired by authenticated resume/authoritative snapshot.
- Telemetry/final EX equals `2 * perfect + great`; BP equals `bad + poor + emptyPoor`.
- Exact chart length is `0..21,600,000` ms; play deadline is `start + clamp(length + 120,000, 180,000, 21,720,000)` ms.
- Final ranks are competition ranks by EX only; DNF rank is null/0; every joint first wins; one-player rounds receive no lobby win.
- Lobby wins saturate at `uint32`; one final result is retained per room and no Arena state is persisted.
- Normal `ChartRunner::finish()`, local save, and profile IR submission are never bypassed, delayed, awaited, or proxied by Arena.
- No Arena provider/entry is added to `OnlineRankingModel`; no Arena target is written to `GeneralVars`; no Arena field is added to `ChartData`.
- The compact Arena gameplay/result overlays are forced on. F2 placement, drag/resize, normalized persistence, and full visual polish remain Phase 4 work.
- Telemetry, results, score GUIDs, EX values, judgements, gauge values, identities, chat bodies, credentials, hashes, and terminal queues never enter operational logs/public errors.
- Existing unrelated dirty files and accepted Phase 1/2 documents are never rewritten or staged.
- Each task receives a spec review and a code-quality review before the next task. Commit independently in the IR and RG worktrees.

---

## File map

### RhythmGame-IR / Arena server

- Existing `arena-server/src/protocol/messages.ts`, `codec.ts`, and `errors.ts`: protocol 1.2 negotiation and strict competition DTOs.
- `arena-server/src/rooms/standings.ts`: pure telemetry progression, live/final rank, ordering, and winner construction.
- `arena-server/src/rooms/telemetry-limiter.ts`: per-frozen-seat integer token bucket and bounded violation window.
- Existing Phase 2 `rooms/round-state.ts`: Playing participant state, deadlines, finalization, and last result.
- Existing room/model/directory modules: seat lifecycle, wins, waiting eligibility, and public snapshots.
- Existing `application/arena-application.ts` and `delivery.ts`: capability/binding preflight and reliable/ephemeral effect mapping.
- Existing `transport/start-server.ts`: exact deadline/coalescing scheduling and backpressure-aware ephemeral delivery.
- Existing config/main/Docker files: capacities, release defaults, health, and deployment verification.
- `tests/unit/*`, `tests/integration/*`, and `scripts/phase3-smoke.ts`: deterministic and real-WebSocket proof.

### RhythmGame client

- Existing `src/arena/ArenaTypes.h` and `ArenaProtocol.*`: protocol 1.2 values and strict text codec.
- `src/arena/ArenaStandingsModel.*`: value-row live/final standings model.
- `src/arena/ArenaResultModel.*`: pending/final metadata plus nested standings.
- `src/arena/ArenaOpponentTarget.*`: stable strongest-other transient target.
- `src/arena/ArenaGameplaySource.h`: fakeable runner sample/final-capture seam.
- `src/arena/QtArenaGameplaySource.*`: production adapter over `ChartRunner`, `BmsLiveScore`, gauges, and `BmsScore`.
- Existing `src/arena/ArenaSession.*`: telemetry, resume, terminal/finalization, models, chat state, and QML commands.
- Existing `RhythmGameQml/Rg.*`, `src/main.cpp`: production object graph.
- `RhythmGameQml/Arena/ArenaOverlayHost.qml`, `ArenaGameplayOverlay.qml`, `ArenaResultOverlay.qml`, and `ArenaGameplayChat.qml`: functional fixed overlays.
- Existing `ContentFrame.qml`: overlay host, central result capture, and result-presentation lifecycle.
- Existing Default gameplay/result QML: transient target and screen-local Arena ranking source.
- Existing LR2 wrapper/value resolver: transient target, abort/chat gate, and Arena result numbers 179/180.
- `test/arena/*`: codec, model, source, session, target, and integration coverage.

---

### Task 1: Negotiate protocol 1.2 and define the shared competition text contract

**Files:**
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/src/lib/server/auth/arena-ticket.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/src/lib/server/auth/tests/arena-ticket.test.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/messages.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/codec.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/errors.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/codec.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/fixtures/phase3-text-goldens.json`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaTypes.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaProtocol.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaProtocol.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/QtWebSocketArenaTransport.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/ArenaProtocol.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/fixtures/phase3-text-goldens.json`

**Interfaces:**
- Consumes: Phase 2 minor-1 hello, binding fields, `SelectionSnapshot`, frozen round/load/start variants, and strict sensitive-error handling.
- Produces these canonical constants and mirrored TypeScript/C++ value families:

```ts
export const PROTOCOL_MAJOR = 1 as const;
export const PROTOCOL_MINOR = 2 as const;
export const ROOMS_CAPABILITY = 'rooms-v1' as const;
export const ROUNDS_CAPABILITY = 'rounds-v1' as const;
export const COMPETITION_CAPABILITY = 'competition-v1' as const;

type ArenaJudgements = Readonly<{
  perfect: number; great: number; good: number;
  bad: number; poor: number; emptyPoor: number;
}>;
type GaugeSnapshot = Readonly<{
  type: 'fc' | 'exhard' | 'hard' | 'normal' | 'easy' | 'aeasy';
  valueMilli: number;
}>;
type ArenaTelemetry = Readonly<{
  sequence: number;
  exScore: number;
  progressPermille: number;
  maxCombo: number;
  badPoorCount: number;
  judgements: ArenaJudgements;
  gauge: GaugeSnapshot;
  playStatus: 'playing';
}>;
type ArenaFinalResult = Readonly<{
  exScore: number;
  maxCombo: number;
  badPoorCount: number;
  judgements: ArenaJudgements;
  clearType: 'max' | 'perfect' | 'fc' | 'exhard' | 'hard' |
    'normal' | 'easy' | 'aeasy' | 'failed';
  finalGauge: GaugeSnapshot;
}>;

type ArenaDnfReason = 'aborted' | 'result_unavailable' | 'left' |
  'kicked' | 'grace_expired' | 'play_deadline';
type LiveStandingEntry = Readonly<{
  memberId: string;
  connectionStatus: 'connected' | 'reserved';
}> & (
  Readonly<{
    competitionState: 'loading' | 'playing';
    rank: number | null;
    telemetry: ArenaTelemetry | null;
  }> |
  Readonly<{
    competitionState: 'finished';
    rank: number;
    result: ArenaFinalResult;
  }> |
  Readonly<{
    competitionState: 'dnf';
    rank: null;
    dnfReason: ArenaDnfReason;
  }>
);
type FinalStandingEntryBase = Readonly<{
  memberId: string;
  identity: PublicIdentity;
  lobbyWinsAfter: number | null;
}>;
type FinalStandingEntry = FinalStandingEntryBase & (
  Readonly<{ competitionState: 'finished'; rank: number; result: ArenaFinalResult }> |
  Readonly<{ competitionState: 'dnf'; rank: null; dnfReason: ArenaDnfReason }>
);
type RoundResultSnapshot = Readonly<{
  resultRevision: number;
  roundId: string;
  selectionRevision: number;
  finalizedAtServerMs: number;
  participantCount: number;
  selection: SelectionSnapshot;
  winnerMemberIds: readonly string[];
  entries: readonly FinalStandingEntry[];
}>;
type LiveStandingsSnapshot = Readonly<{
  roomId: string;
  roomGeneration: number;
  roundId: string;
  launchAttemptId: string;
  standingsRevision: number;
  entries: readonly LiveStandingEntry[];
}>;
```

The C++ mirror uses `ArenaJudgements`, `GaugeSnapshot`, `TelemetrySnapshot`,
`FinalResult`, `DnfReason`, `LiveStandingEntry`, `LiveStandingsSnapshot`,
`FinalStandingEntry`, and `RoundResultSnapshot` in namespace `arena`; it uses
the existing `arena::PublicIdentity` rather than defining a second identity
DTO.

New client variants are `round_telemetry`, `round_result_submit`, and
`round_abandon`; new server variants are `round_standings`,
`round_terminal_accepted`, and `round_finalized`. Active/final standing unions,
`RoundResultSnapshot`, result revision, frozen identity, and `chartLengthMs`
match the controlling spec exactly.

Use the existing strict outer envelope. Telemetry is uncorrelated and nests
its value under `data.telemetry`; result/abandon carry outer `requestId` and
nest `result`/`reason`; standings carries the complete entries; terminal
accepted echoes outer `requestId` plus room/round/attempt and terminal kind;
finalized carries room/round/attempt, the immutable result, and the complete
post-finalization member array. Do not flatten nested values or add an
acknowledgement for each telemetry packet.

Add command errors `competition_capability_required`, `result_invalid`,
`round_already_terminal`, and `server_capacity`; add
`chart_length_mismatch` to the closed launch-cancellation reasons. Existing
`round_stale`, `connection_generation_stale`, `rate_limited`, and fatal
malformed-message behavior remain unchanged.

Extend protocol-1.2 `RoomSnapshot` with required nullable `liveStandings` and
`lastRoundResult` fields; a missing key is invalid. Extend frozen participant
rows with `PublicIdentity`, bound public `Member.lobbyWins` to `uint32`, and
keep the deadline additions on the exact Phase 2 active-round/schedule/start
branches named in the specification.

- [ ] **Step 1: Write failing negotiation, schema, and golden tests**

Cover minors 0/1/2, canonical intersection, invalid capability dependency,
minor-ceiling spoof attempts, minor-1 create/join/resume rejection without
seat-token consumption, both C++ hello encodings, every numeric/string bound,
EX/BP equation,
strict unknown fields at every nesting level, all unions, and one positive/
negative golden per new or extended variant. Add maximum-shape encoding tests:
a 16-row standing is at most 65,536 bytes, a legal result containing the
4,096-value Phase 2 random sequence is at most 262,144 bytes, and the complete
finalization event is at most 524,288 bytes. Assert the existing Phase 2
fixture bytes are unchanged. Assert the C++ decoder and WebSocket configuration
retain a 4 MiB server-message receive ceiling and accept the maximum legal
finalization shape.

- [ ] **Step 2: Run RED in both repositories**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/codec.test.ts
ctest --preset dev-rel -R ArenaProtocol --output-on-failure
```

Expected: protocol minor 2, `competition-v1`, and Phase 3 variants are not yet
defined.

- [ ] **Step 3: Implement strict negotiation and competition values**

The server accepts minor `0..2`, returns the ordered capability intersection
bounded by the negotiated-minor ceiling (`0: rooms`, `1: rooms+rounds`,
`2: rooms+rounds+competition`), and enforces
`competition -> rounds -> rooms`. Keep Zod objects strict. The C++ codec
mirrors enum/string mappings, integer limits, required keys, and safe error
redaction. `round_telemetry` has no request ID; final/abandon are request-
correlated and binding-complete. Keep `QtWebSocketArenaTransport` incoming
frame/message limits at `MaxServerMessageBytes`; per-variant decoding enforces
the smaller 64/256/512 KiB competition caps.

- [ ] **Step 4: Implement admission/resume compatibility**

Create/join without `competition-v1` returns
`competition_capability_required`. A hello resume without it returns resume
status failed with the same code and does not mutate the seat/token. Update the
C++ protocol hello modes so Task 7 can send minor 2/all capabilities or minor
0/`rooms-v1`. Task 7 owns the single-use `protocol_incompatible` reconnect and
browse-only command gates.

- [ ] **Step 5: Update the IR ticket literal and focused cryptographic test**

Change the issued claim to `protocolMinor: 2`. Keep the Arena verifier accepting
any nonnegative safe minor. Run:

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena test:server -- src/lib/server/auth/tests/arena-ticket.test.ts
```

- [ ] **Step 6: Verify byte parity and commit separately**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
ctest --preset dev-rel -R ArenaProtocol --output-on-failure
Compare-Object (Get-Content -Raw 'T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/fixtures/phase3-text-goldens.json') (Get-Content -Raw 'T:/RG/.worktrees/online-arena/test/arena/fixtures/phase3-text-goldens.json')
git -C T:/RhythmGame-IR/.worktrees/online-arena add src/lib/server/auth/arena-ticket.ts src/lib/server/auth/tests/arena-ticket.test.ts arena-server/src/protocol arena-server/tests/unit/codec.test.ts arena-server/tests/fixtures/phase3-text-goldens.json
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: define Arena competition protocol"
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaTypes.h src/arena/ArenaProtocol.h src/arena/ArenaProtocol.cpp src/arena/QtWebSocketArenaTransport.cpp test/arena/ArenaProtocol.test.cpp test/arena/fixtures/phase3-text-goldens.json
git -C T:/RG/.worktrees/online-arena commit -m "feat: decode Arena competition protocol"
```

Expected: all commands pass; `Compare-Object` prints nothing.

---

### Task 2: Implement pure standings and the bounded telemetry limiter

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/standings.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/telemetry-limiter.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/standings.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/telemetry-limiter.test.ts`

**Interfaces:**
- Consumes: Task 1 telemetry/final/public standing types and Phase 2 frozen roster order.
- Produces:

```ts
export function validateTelemetryProgression(
  previous: ArenaTelemetry | undefined,
  next: ArenaTelemetry
): boolean;

export function buildLiveStandings(
  participants: readonly CompetitionParticipant[]
): readonly LiveStandingEntry[];

export function buildFinalStandings(
  participants: readonly CompetitionParticipant[]
): Readonly<{
  entries: readonly RankedTerminalEntry[];
  winnerMemberIds: readonly string[];
}>;

export class TelemetryLimiter {
  attempt(nowMs: number): 'allow' | 'drop' | 'close';
  violation(nowMs: number): 'drop' | 'close';
  clear(): void;
}
```

`CompetitionParticipant` contains frozen index/member ID, connection state,
optional newest telemetry, and immutable final-or-DNF terminal state. These
modules return values and have no socket, room, clock creation, logging, or
side effects.

```ts
type CompetitionParticipant = Readonly<{
  frozenIndex: number;
  memberId: string;
  identity: PublicIdentity;
  connectionStatus: 'connected' | 'reserved';
  telemetry?: ArenaTelemetry;
  terminal?:
    | Readonly<{ kind: 'finished'; result: ArenaFinalResult }>
    | Readonly<{ kind: 'dnf'; reason: ArenaDnfReason }>;
}>;
type RankedTerminalEntry = Omit<FinalStandingEntryBase, 'lobbyWinsAfter'> & (
  Readonly<{ competitionState: 'finished'; rank: number; result: ArenaFinalResult }> |
  Readonly<{ competitionState: 'dnf'; rank: null; dnfReason: ArenaDnfReason }>
);
```

- [ ] **Step 1: Write failing rank/progression/limit tests**

Cover zero score versus no data; descending EX; frozen-order ties; `1,1,3`;
active/no-data/DNF order; finished precedence; all ties; all DNF; winner lists;
each nondecreasing field; gauge changes; EX/BP mismatch; limiter initial burst
10, one-token/200 ms refill, rolling 10 seconds, bounded 20 timestamps, exact
twentieth close, stale/old/terminal packet bypass, pre-Playing/regression
violation, and clear.

- [ ] **Step 2: Run RED**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/standings.test.ts tests/unit/telemetry-limiter.test.ts
```

- [ ] **Step 3: Implement immutable standings construction**

Use `rank = 1 + strictlyGreaterFinalOrLiveScores`. Never treat missing
telemetry as zero. Final rows use final-only values; no late telemetry can
override them. Validate into new values and return frozen arrays.

- [ ] **Step 4: Implement integer limiter state**

Use capacity 10, initial 10, refill quanta `floor((now-lastRefill)/200)`, and
cap tokens at 10. On no token, append/prune the fixed 10-second violation
window; return `close` at 20. `violation()` uses the same bounded window for an
allowed-shape packet that is out of phase or fails semantic progression.
Duplicate/old sequence filtering occurs before this module and therefore
consumes no attempt. Stale binding/round and already-terminal in-flight packets
also bypass it; a current newer packet consumes exactly one token before phase/
progression validation.

- [ ] **Step 5: Verify and commit**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/standings.test.ts tests/unit/telemetry-limiter.test.ts
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/rooms/standings.ts arena-server/src/rooms/telemetry-limiter.ts arena-server/tests/unit/standings.test.ts arena-server/tests/unit/telemetry-limiter.test.ts
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: rank bounded Arena telemetry"
```

---

### Task 3: Extend the room domain through Playing, terminal states, and finalization

**Files:**
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/round-state.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/models.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room-directory.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/round-loading.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/round-playing.test.ts`

**Interfaces:**
- Consumes: Phase 2 frozen/loading state and Tasks 1-2 values/functions.
- Produces these additions to `RoomDirectory`:

```ts
reportTelemetry(actor, input, nowMs): DomainResult<TelemetryMutation>;
submitRoundResult(actor, input, nowMs): DomainResult<TerminalMutation>;
abandonRound(actor, input, nowMs): DomainResult<TerminalMutation>;
flushDueStandings(nowMs): readonly RoomTransition[];
nextDeadlineMs(): number | undefined;
```

`TerminalMutation` distinguishes accepted from identical retry and may contain
one atomic `RoundResultSnapshot`; a conflict is the domain error
`round_already_terminal`. `TelemetryMutation` represents every non-error
ingress outcome without carrying a socket delivery.

```ts
type TelemetryMutation =
  | Readonly<{
      status: 'accepted';
      standingsRevision: number;
      nextFlushAtMs: number;
    }>
  | Readonly<{ status: 'ignored' }>
  | Readonly<{ status: 'dropped' }>
  | Readonly<{
      status: 'close';
      closeCode: 1008;
      reason: 'rate_limited';
    }>;
type TerminalMutation = Readonly<{
  status: 'accepted' | 'identical_retry';
  terminal: 'finished' | 'dnf';
  standingsRevision: number;
  finalized?: RoundResultSnapshot;
}>;
```

- [ ] **Step 1: Add failing chart-length and deadline tests**

Cover load lengths 0/21,600,000, exact agreement, mismatch cancellation that
clears selection/increments selection revision/clears ready, deadline clamp,
deadline in schedule/start/snapshot, and the invariant that a terminal command
at `now == deadline` loses to deadline DNF.

- [ ] **Step 2: Add failing Playing/terminal/finalization tests**

Cover current binding/frozen participant/waiting rejection, sequence and
limiter behavior, connection-status revision, first terminal, identical retry,
conflicting retry, final EX/combo/BP/each-judgement regression rejection, eight
terminal attempts/minute with an eight-timestamp bound retained through resume,
all DNF causes, resume before grace, exact grace expiry, leave/kick before and
after finish, deadline, and finalization once.

Final assertions include EX-only ties, all-DNF/no-winner, zero-score winner,
sole-player no win, sole valid finisher, joint wins, reserved winner, removed
winner, uint32 saturation, member snapshot atomicity, immediate Selecting,
waiting eligibility, selection Phase 2 revalidation, last-result replacement,
and room destruction cleanup.

- [ ] **Step 3: Run RED**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/unit/round-loading.test.ts tests/unit/round-playing.test.ts
```

- [ ] **Step 4: Extend loading and freeze public identity**

On Phase 3 successful load store exact `chartLengthMs`; reject disagreement
through Phase 2's clear-selection consistency cancellation, incrementing
selection revision and clearing ready. Copy each participant's
`PublicIdentity` into the frozen round before Playing so later seat removal
cannot erase result identity.

- [ ] **Step 5: Implement Playing mutation and deadlines**

Store newest telemetry and limiter per frozen participant. Return `ignored` for
stale/duplicate/terminal in-flight packets, `dropped` for bounded token/semantic
violations, and `close` on the twentieth violation. Only `accepted` increments
`standingsRevision` and marks `nextStandingsFlushMs`; no-data, zero, connection
state, finished, and DNF remain distinct. Process due play deadline before
same-time commands.

- [ ] **Step 6: Implement idempotent terminal acceptance and finalization**

Compare identical final values/abandon reason structurally. Finalization first
computes ranks/winners, then applies saturating wins to extant connected/
reserved winner seats, and only then builds the immutable result so every
`lobbyWinsAfter` is post-award. It clears active competition/ready state,
returns Selecting, re-evaluates selection against Phase 2 common availability,
and retains exactly one last result. Reject a final below any comparable newest
telemetry counter with `result_invalid`; progress, clear, and final gauge are
not regression counters.

- [ ] **Step 7: Verify and commit**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/rooms arena-server/tests/unit/round-loading.test.ts arena-server/tests/unit/round-playing.test.ts
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: finalize Arena competition rounds"
```

---

### Task 4: Wire competition commands, coalesced delivery, resume, and backpressure

**Files:**
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/delivery.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/arena-application.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/start-server.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/config.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/main.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.env.example`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/arena-application.test.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/websocket.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/phase3-round.test.ts`

**Interfaces:**
- Consumes: Task 3 directory methods/effects and existing serialized per-socket receive tail.
- Produces:

```ts
type Delivery = ExistingReliableDelivery |
  Readonly<{
    kind: 'send_ephemeral';
    connectionIds: readonly string[];
    message: Extract<ServerMessage, { type: 'round_standings' }>;
  }>;
```

`ArenaApplication.nextDeadlineMs()` includes heartbeat/grace, Phase 2 launch,
play deadline, and next dirty standings flush. Room snapshots expose active
standings through required nullable `liveStandings` and retain the required
nullable `lastRoundResult`.

- [ ] **Step 1: Write failing application and real-WebSocket scenarios**

Drive three authenticated clients through load-length agreement, start,
telemetry/no-data/zero, full standings, waiting receive/reject-submit, final,
DNF, finalization, member wins, last-result resume, and next round. Add fake
backpressure/drain assertions proving ephemeral drop and reliable terminal/
finalization delivery. Prove a 524,288-byte finalization and a legal near-4-MiB
room snapshot fit from an empty socket; preloaded buffered bytes that would
cross 5 MiB close with 1013, reserve the seat, and recover the result on resume.

- [ ] **Step 2: Run RED**

```powershell
bun --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server test tests/integration/arena-application.test.ts tests/integration/websocket.test.ts tests/integration/phase3-round.test.ts
```

- [ ] **Step 3: Wire capability/binding preflight and effects**

Only negotiated `competition-v1` room-bound frozen participants reach domain
mutation. Stale/old telemetry is ignored without response. Final/abandon map to
correlated accepted/error effects. Telemetry `ignored`/`dropped` produces no
delivery; `close` maps to the existing redacted WebSocket policy-close effect.
A finalizing terminal command sends targeted accepted, reliable complete
`round_finalized`, member/room/directory effects in domain order.

- [ ] **Step 4: Implement 200 ms coalescing and ephemeral send**

After every receive/disconnect/sweep, schedule the earliest application
deadline. `flushDueStandings` creates one complete event per dirty room. The
gateway sends immediately when writable; on first backpressure it marks that
socket ephemeral-blocked and drops later ephemeral deliveries until drain.
Measure encoded UTF-8 bytes and current `getBufferedAmount()` before each send.
Set the Bun backpressure limit to 5 MiB. Drop ephemeral sends that would cross
it; for reliable sends, explicitly close with 1013/`try_again_later` and let
normal disconnect/grace reserve the seat. A single legal server message up to
4 MiB always fits from an empty buffer. Resume must replace state from the room
snapshot, including `lastRoundResult`; do not claim the overflowed frame was
delivered.

- [ ] **Step 5: Add capacities and redacted observability**

Validate defaults:

```text
MAX_ROOMS=1000
MAX_CONNECTIONS=5000
TELEMETRY_INTERVAL_MS=200
```

Room creation beyond capacity returns `server_capacity`; WebSocket upgrade over
the connection cap receives HTTP 503 before application state allocation. Logs
contain only opaque room/round ID plus participant/finished/DNF/winner counts
and duration; tests inject sentinel telemetry/identity/result values and assert
absence.

- [ ] **Step 6: Verify and commit**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/application arena-server/src/transport/start-server.ts arena-server/src/config.ts arena-server/src/main.ts arena-server/.env.example arena-server/tests/integration
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: relay Arena live standings"
```

---

### Task 5: Add client standings/result models and strongest-opponent target

**Files:**
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaStandingsModel.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaStandingsModel.cpp`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaResultModel.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaResultModel.cpp`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaOpponentTarget.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaOpponentTarget.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaCompetitionModels.test.cpp`

**Interfaces:**
- Consumes: Task 1 C++ `LiveStandingEntry`, frozen identity, and
  `RoundResultSnapshot` values.
- Produces:

```cpp
class ArenaStandingsModel final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString roundId READ roundId NOTIFY snapshotChanged FINAL)
    Q_PROPERTY(qint64 revision READ revision NOTIFY snapshotChanged FINAL)
  public:
    bool replace(const arena::LiveStandingsSnapshot& snapshot,
                 const QHash<QString, arena::PublicIdentity>& identities);
    bool replaceFinal(const arena::RoundResultSnapshot& snapshot);
    void clear();
};

class ArenaResultModel final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
    Q_PROPERTY(bool finalized READ finalized NOTIFY changed FINAL)
    Q_PROPERTY(QString roundId READ roundId NOTIFY changed FINAL)
    Q_PROPERTY(qint64 resultRevision READ resultRevision NOTIFY changed FINAL)
    Q_PROPERTY(int participantCount READ participantCount NOTIFY changed FINAL)
    Q_PROPERTY(QStringList winnerMemberIds READ winnerMemberIds NOTIFY changed FINAL)
    Q_PROPERTY(QStringList winnerNames READ winnerNames NOTIFY changed FINAL)
    Q_PROPERTY(int localRank READ localRank NOTIFY changed FINAL)
    Q_PROPERTY(bool localDnf READ localDnf NOTIFY changed FINAL)
    Q_PROPERTY(bool localWinner READ localWinner NOTIFY changed FINAL)
    Q_PROPERTY(QString selectionTitle READ selectionTitle NOTIFY changed FINAL)
    Q_PROPERTY(QString selectionOptionsSummary READ selectionOptionsSummary NOTIFY changed FINAL)
    Q_PROPERTY(arena::ArenaStandingsModel* standings READ standings CONSTANT FINAL)
};

class ArenaOpponentTarget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed FINAL)
    Q_PROPERTY(QString memberId READ memberId NOTIFY changed FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed FINAL)
    Q_PROPERTY(qint64 exScore READ exScore NOTIFY changed FINAL)
    Q_PROPERTY(bool finished READ finished NOTIFY changed FINAL)
  public:
    void update(const arena::LiveStandingsSnapshot& snapshot,
                QStringView selfMemberId,
                const QHash<QString, arena::PublicIdentity>& identities);
    void clear();
};
```

Standing roles exactly match the spec: identity, connection/competition state,
rank/hasScore, EX/progress/combo/BP/judgements/gauge/clear, lobbyWinsAfter, and
DNF reason. Nullable rank maps to `0`; nullable `lobbyWinsAfter` maps to `-1`.

- [ ] **Step 1: Write failing model-contract tests**

Cover role names/types, identity cache, one reset per complete snapshot,
stale/other-round rejection, rank 0, zero-score `hasScore`, final replacement,
pending/final result metadata, local DNF/winner, plain frozen-option summary,
clear, and no per-row QObject.

- [ ] **Step 2: Write failing target tests**

Cover local exclusion, strongest current EX, target change, exact-tie retention,
frozen-order fallback, finished target persistence, DNF/no-data exclusion,
zero-score availability, identity lookup, and clear.

- [ ] **Step 3: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaCompetitionModels --output-on-failure
```

- [ ] **Step 4: Implement value-row replacement and target derivation**

Validate all rows into owned vectors before `beginResetModel`; reject without
mutation. Use integer roles and nullable-wire rank mapped to zero. Target tie
logic retains current ID only while it remains among strongest candidates.

- [ ] **Step 5: Verify and commit**

```powershell
ctest --preset dev-rel -R ArenaCompetitionModels --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaStandingsModel.* src/arena/ArenaResultModel.* src/arena/ArenaOpponentTarget.* test/arena/ArenaCompetitionModels.test.cpp CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: model Arena competition state"
```

---

### Task 6: Sample the existing runner and capture bounded local finals

**Files:**
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaGameplaySource.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/QtArenaGameplaySource.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/QtArenaGameplaySource.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/FakeArenaGameplaySource.h`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaGameplaySource.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: existing `ChartRunner`, P1 `Player`, `BmsLiveScore`, gauges,
  `BmsScore/BmsResult`, and Task 1 C++ telemetry/final values.
- Produces:

```cpp
enum class ArenaGameplayCaptureFailure {
    NoRunner,
    WrongScore,
    InvalidNumber,
    UnsupportedGauge,
    InvalidResult,
};

class ArenaGameplaySource : public QObject {
    Q_OBJECT
  public:
    virtual auto attach(gameplay_logic::ChartRunner* runner)
      -> std::expected<QString, ArenaGameplayCaptureFailure> = 0;
    virtual void detach() = 0;
    virtual auto sample(quint32 sequence) const
      -> std::expected<arena::TelemetrySnapshot,
                       ArenaGameplayCaptureFailure> = 0;
    virtual auto captureFinal(gameplay_logic::BmsScore* score) const
      -> std::expected<arena::FinalResult,
                       ArenaGameplayCaptureFailure> = 0;
};
```

`attach` returns the expected local P1 score GUID; it does not take ownership.
`sample`/`captureFinal` copy values on the QObject thread and never create a
network request or inspect `submissionState`.

- [ ] **Step 1: Write failing sample/capture tests**

Build a deterministic runner/score fixture. Assert EX/sequence, elapsed-length
progress rounding/clamp and zero-length behavior, max combo, six judgement
indices, BP equation, every six gauge-name mapping, first-above-threshold/last
fallback, positive gauge maximum/normalization, zero score, wrong GUID, NaN/
noninteger/overflow rejection, final clear mapping, detach, and QPointer
destruction safety.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaGameplaySource --output-on-failure
```

- [ ] **Step 3: Implement the production adapter without core-score edits**

Read `points`, elapsed/length, combo, judgement list, and gauge getters. Require
finite exact integers and protocol bounds. Normalize gauge with
`round(clamp(gauge/gaugeMax,0,1)*100000)`. Map only the six standard gauge names
and nine result clear names. Do not modify `ChartRunner`, `BmsLiveScore`,
`BmsScore`, `Profile`, or `GeneralVars` in this task.

- [ ] **Step 4: Prove IR-state independence**

For otherwise identical final fixtures, set `BmsScore::submissionState` to
Submitting, Submitted, Failed, Duplicate, and NotSubmitting. Assert identical
Arena final values and no signal/network dependency.

- [ ] **Step 5: Verify and commit**

```powershell
ctest --preset dev-rel -R ArenaGameplaySource --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaGameplaySource.h src/arena/QtArenaGameplaySource.* test/arena/FakeArenaGameplaySource.h test/arena/ArenaGameplaySource.test.cpp CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: sample Arena gameplay results"
```

---

### Task 7: Orchestrate telemetry, targets, terminal retries, and results in `ArenaSession`

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/main.cpp`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Rg.h`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Rg.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/FakeArenaTransport.h`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/ArenaSessionRounds.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaSessionCompetition.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Phase 2 runner/loading/session lifecycle and Tasks 1, 5, and 6.
- Produces the QML-facing additions:

```cpp
Q_PROPERTY(arena::ArenaStandingsModel* liveStandings READ liveStandings CONSTANT FINAL)
Q_PROPERTY(arena::ArenaResultModel* lastResult READ lastResult CONSTANT FINAL)
Q_PROPERTY(arena::ArenaResultModel* presentedResult READ presentedResult CONSTANT FINAL)
Q_PROPERTY(arena::ArenaOpponentTarget* opponentTarget READ opponentTarget CONSTANT FINAL)
Q_PROPERTY(bool competitionAvailable READ competitionAvailable NOTIFY capabilitiesChanged FINAL)
Q_PROPERTY(gameplay_logic::ChartRunner* arenaRunner READ arenaRunner NOTIFY competitionChanged FINAL)
Q_PROPERTY(bool arenaGameplayActive READ arenaGameplayActive NOTIFY competitionChanged FINAL)
Q_PROPERTY(bool resultPresentationActive READ resultPresentationActive NOTIFY competitionChanged FINAL)
Q_PROPERTY(bool gameplayChatOpen READ gameplayChatOpen NOTIFY gameplayChatOpenChanged FINAL)
Q_PROPERTY(QString arenaOptionsSummary READ arenaOptionsSummary NOTIFY competitionChanged FINAL)

Q_INVOKABLE bool submitLocalResult(gameplay_logic::BmsScore* score);
Q_INVOKABLE void abandonCurrentRound();
Q_INVOKABLE void setGameplayChatOpen(bool open);
Q_INVOKABLE void toggleGameplayChat();
Q_INVOKABLE void endResultPresentation(const QString& roundId);
```

Constructor injection adds non-owning `ArenaGameplaySource*`. Session owns the
three models/target, sampling tasks, frozen identity cache, expected GUID,
sequence, newest pending telemetry, one pending terminal, presentation ID, and
one formatter for the frozen P1/P2 note-order plus DP-mode summary.

- [ ] **Step 1: Write failing deterministic session scenarios**

Use fake transport, scheduler, loader, and gameplay source. Cover agreed/mismatched
load length, deadline decode, exact first/next 200 ms tick, sequence, invalid
sample skip, disconnected newest-only queue, resume resend, final stopping
telemetry without detaching before `openResult`, standings/model/target update,
stale snapshot, backpressure gaps,
terminal submission/capture failure/abandon, identical retry, authoritative
resume with the same request ID/value and refreshed connection generation,
finalization, pending/presented/last
result, profile/leave/kick/exit
cleanup, chat state, the one-shot minor-0 browse fallback with competition
commands disabled, and absence of sensitive diagnostics.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaSessionCompetition --output-on-failure
```

- [ ] **Step 3: Extend load/start orchestration**

On each explicit Arena entry permit one legacy retry. Send minor 2/all
capabilities first; only an initial pre-auth `protocol_incompatible` reconnects
as anonymous minor 0/`rooms-v1`. Mark that successful connection browse-only
and reject login/create/join/resume locally.

On Phase 2 `loadFinished`, compute
`ceil(player1.chartLength / 1,000,000)` with checked arithmetic and include it
in the Phase 3 load result. Cache frozen identities/deadline from snapshots.
Attach the gameplay source before emitting `roundRunnerStarted`; reject wrong
source/round without exposing the runner.

- [ ] **Step 4: Implement telemetry scheduling and standings replacement**

Schedule one-shot 200 ms tasks recursively through injected scheduler. Increment
sequence only for a successfully captured value. Anchor to a monotonic next-
sample deadline; after a delayed callback, skip missed quanta and sample once
rather than bursting. If transport is unavailable, replace one pending sample;
after same-round resume send it once. Complete standings snapshots atomically
replace model and recompute target.

- [ ] **Step 5: Implement result capture, DNF, and retry**

`submitLocalResult` requires the current expected GUID. It copies the final
immediately when not abandoned, stops sampling, creates pending
presentation, and sends or stores one correlated terminal. If the expected
score follows local abandon, it creates the DNF presentation but does not call
`captureFinal`. `abandonCurrentRound` is idempotent, marks local DNF pending,
stops sampling, and sends/stores `aborted`. Capture failure uses
`result_unavailable`. Runner Finished cancels the timer but retains the source/
GUID until this capture or cleanup; abandon retains just the GUID needed to
recognize the partial score. Snapshot/ack/finalization resolves queues without
calling finish or IR. A reconnect retry reuses the stored request ID and exact
terminal value, rewrites only the current connection generation, and never
reconstructs the payload from mutable score state.

- [ ] **Step 6: Implement model/presentation/cleanup state**

Room snapshot replaces active standings and last result; round finalization
updates members/wins, live final rows, last result, and matching presented
result before room phase changes. Every profile/room/session terminal path
cancels timer, detaches source, clears target/chat/queues/cache, and ends stale
presentation.

- [ ] **Step 7: Construct production source and verify**

```powershell
ctest --preset dev-rel -R "ArenaSession|ArenaGameplaySource|ArenaCompetitionModels" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaSession.* src/main.cpp RhythmGameQml/Rg.* test/arena/FakeArenaTransport.h test/arena/ArenaSessionRounds.test.cpp test/arena/ArenaSessionCompetition.test.cpp CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: orchestrate Arena competition"
```

---

### Task 8: Add the universal overlay host, gameplay chat action, and abort gates

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayOverlay.qml`
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaGameplayChat.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaBrowser.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaLoginPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_en.ts`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_pl.ts`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaOverlayPolicy.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 7 QML surface, existing chat model/send method, current StackView screen, and Phase 2 frozen selection summary.
- Produces fixed content surfaces with:

```qml
required property var session
required property string placementKind
required property string resolvedSkinId
required property string layoutVariant
property bool expanded: false
```

Use `gameplayLeaderboard`/`resultStandings` for `placementKind`, the active
profile's resolved `themeConfig[screenKey]` family key for `resolvedSkinId`,
and the resolved `k5`/`k7`/`k10`/`k14`/`result` screen key for
`layoutVariant`. No component reads/writes geometry settings.
`ArenaOverlayHost` is a sibling above `sceneStack` and identifies gameplay only
when the current item owns the session's runner.

- [ ] **Step 1: Add failing policy/session tests**

Test overlay active only for the current Arena runner, forced compact visibility,
expand state, F8 toggle, Escape-close priority, plain chat, send path, state
cleanup, one abort command, partial local result ignored after abandon, and
minor-0/minor-1 browser update-required with login/create/join/resume disabled.

- [ ] **Step 2: Implement the ContentFrame overlay host**

Anchor the fixed panel to top-right with a 24 px safe margin, maximum width 420,
maximum height `parent.height - 48`, and high z above every skin. Use clipped,
scrollable participant/detail/chat views for all 16 seats. Pointer handlers
exist only on panel controls. Expose placement descriptor strings but add no
Settings/Profile fields.

- [ ] **Step 3: Implement compact/expanded rows and chat drawer**

Compact rows show rank/name/EX/progress/state. Expanded rows add BP/combo/six
judgements/gauge plus frozen option summary. F8 and Chat toggle the existing
session chat model. Enter sends, Escape closes, and the runner is never paused.
Render every remote name/chat label with `Text { textFormat: Text.PlainText }`;
the default `AutoText` mode is forbidden for untrusted strings.

- [ ] **Step 4: Gate both gameplay abort paths**

In Default's Escape handler and `Lr2SkinScreenWrapper.handleGameplayEscape`,
close Arena chat first. Otherwise call `abandonCurrentRound()` before existing
pop/finish/result behavior. Do not change non-Arena escape semantics.

- [ ] **Step 5: Verify QML and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "ArenaOverlayPolicy|ArenaSessionCompetition" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena RhythmGameQml/ContentFrame.qml RhythmGameQml/CMakeLists.txt RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml share/RhythmGame/themes/Default/translations test/arena/ArenaOverlayPolicy.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: show Arena gameplay standings"
```

Manual check at this task: F8 opens/closes during Default/LR2/Beatoraja play;
game clock continues; keyboard editor does not hit lanes; controller input
continues; Escape closes chat before a second Escape abandons.

---

### Task 9: Force the transient strongest-opponent pacemaker in every gameplay family

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinValueResolver.qml`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaPacemakerIntegration.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: `ArenaSession.opponentTarget` from Tasks 5/7.
- Produces one first-priority effective target for existing Default and legacy
  target/ghost calculations; no saved state or replay object.

- [ ] **Step 1: Add failing effective-target policy tests**

Cover Arena available/unavailable, local saved target set to each provider,
opponent change, finished target, tie stability, no opponent, leave cleanup,
and byte-for-byte unchanged serialized `GeneralVars` before/after play.

- [ ] **Step 2: Intercept Default target calculations**

Before local battle/saved/fraction branches, return Arena opponent EX for both
`targetPoints1` and `targetFinalPoints1`. When Arena is active but unavailable,
return zero/no target rather than falling through. Do not alter score replayers
or target settings.

- [ ] **Step 3: Intercept LR2/Beatoraja target calculations**

In `gameplayTargetScorePoints` and `gameplayTargetFinalPoints`, perform the same
Arena-first check. Existing resolver differences/grade/ghost formulas consume
those functions. Add only a boolean helper for explicit target availability;
do not fabricate an `OnlineRankingModel` or `BmsScore` entry.

- [ ] **Step 4: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "ArenaPacemaker|ArenaCompetitionModels" --output-on-failure
git -C T:/RG/.worktrees/online-arena add share/RhythmGame/themes/Default/scripts/gameplay/Gameplay.qml RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml RhythmGameQml/Lr2/Lr2SkinValueResolver.qml test/arena/ArenaPacemakerIntegration.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: force Arena opponent target"
```

Manual matrix: Default/LR2/Beatoraja with saved Best/Last/NextRank/Fraction;
other player starts at zero, overtakes, ties, finishes, DNF; leaving Arena
reveals the unchanged saved target.

---

### Task 10: Capture normal results and expose Arena rank/winner across result skins

**Required sub-skill:** Read and apply `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaResultOverlay.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaOverlayHost.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/Result.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/Side.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/ScoreColumn.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/result/RankingPosition.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinValueResolver.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_en.ts`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_pl.ts`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaResultIntegration.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Task 7 `submitLocalResult`, `presentedResult`, `lastResult`, and the
  existing central `openResult(scores, profiles, chartData)` path.
- Produces `property string arenaRoundId: ""` on Default Result and the legacy
  result wrapper, plus a screen-local source state:

```text
Arena -> RhythmGame -> Tachi -> LR2IR -> Arena
```

Arena source exists only in Default result QML and is never written to
`OnlineRankingModel::Provider` or `generalVars.rankingProvider`.

- [ ] **Step 1: Add failing central-capture/result tests**

Assert `openResult` submits only expected Arena P1 score, after `finish()` has
already set local save/IR state; pending result opens immediately; every Arena
result starts Arena source; forward/reverse cycles do not write saved provider;
normal results retain saved-provider behavior; IR query/submission changes do
not affect Arena pending/final; and Arena failure does not affect local score.

- [ ] **Step 2: Capture result and presentation lifecycle in ContentFrame**

Call `submitLocalResult(scores[0])` immediately before pushing the normal result
component. When it returns true, pass `presentedResult.roundId` as the declared
`arenaRoundId` property. On result destruction/pop, call
`endResultPresentation(arenaRoundId)`. Do not capture
course, replay, non-Arena, wrong-GUID, or locally abandoned score as a final;
an expected abandoned score may still open the DNF presentation.

- [ ] **Step 3: Implement Default transient source and Arena ranking mode**

Refactor `Side.qml` to separate saved provider from screen-local effective
source. Keep all three `RankingQuery` instances active. Arena mode reads local
rank/count/pending from `presentedResult`, has no URL, ignores IR submission
failure, and never writes saved provider. Extend `RankingPosition.qml` with an
Arena display that hides old position/delta arrow and renders `#rank /count`,
loading, or dash for finalized DNF.

- [ ] **Step 4: Override LR2/Beatoraja 179/180 narrowly**

At the start of result-number resolution, when Session has active presentation:

```qml
let arenaResultMatches = root.arenaRoundId !== ""
    && root.arenaRoundId === Rg.arenaSession.presentedResult.roundId;
case 179: return arenaResultMatches
    ? (Rg.arenaSession.presentedResult.finalized
        ? Rg.arenaSession.presentedResult.localRank : 0)
    : rankingState.playerRank();
case 180: return arenaResultMatches
    ? Rg.arenaSession.presentedResult.participantCount
    : rankingState.currentPlayerCount;
```

Outside that condition, retain `rankingState.playerRank()` and
`currentPlayerCount` unchanged. Do not touch `OnlineRankingModel`.

- [ ] **Step 5: Implement the universal pending/final result overlay**

Use the existing fixed host. Pending shows waiting/connection state and local
terminal status. Final shows ordered standings, every winner, DNF, selection
option summary, and `lobbyWinsAfter`. Render plain text, force visible, expose
placement descriptor, and add no Phase 4 geometry behavior. Every remote-name
label explicitly uses `Text.PlainText`, not `AutoText`.

- [ ] **Step 6: Verify normal IR independence and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "ArenaResult|ArenaSessionCompetition|ArenaGameplaySource" --output-on-failure
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena/ArenaResultOverlay.qml RhythmGameQml/Arena/ArenaOverlayHost.qml RhythmGameQml/ContentFrame.qml RhythmGameQml/CMakeLists.txt RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml RhythmGameQml/Lr2/Lr2SkinValueResolver.qml share/RhythmGame/themes/Default/scripts/result share/RhythmGame/themes/Default/translations test/arena/ArenaResultIntegration.test.cpp test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: show Arena final standings"
```

Manual failure matrix: IR upload succeeds/fails while Arena finalizes; Arena
socket drops while local score saves/submits; pending result later finalizes;
joint winner and DNF; Default cycles all sources; LR2/Beatoraja number fields
present/absent; non-Arena result persists the selected online provider normally.

---

### Task 11: Cross-repository Phase 3 integration, Docker, privacy, and release gate

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/scripts/phase3-smoke.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/package.json`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/README.md`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/Dockerfile`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.dockerignore`
- Create: `T:/RG/.worktrees/online-arena/docs/arena/phase3-verification.md`

**Interfaces:**
- Consumes: the complete Phase 3 implementation in both repositories.
- Produces one reproducible three-client smoke, Docker/Linux competition proof,
  privacy/resource evidence, normal-IR independence evidence, and exit-criteria
  traceability.

- [ ] **Step 1: Freeze heads and verify contract parity**

Record both SHAs in the verification report. Compare Phase 3 fixture bytes and
confirm Phase 2 text/binary fixture digests are unchanged. A mismatch is fixed
in the owning codec rather than normalized by the integration script.

- [ ] **Step 2: Implement the exact three-client smoke**

The script has two modes sharing one assertion driver. With no arguments it
starts the application in process with an injected local verifier. With
`--docker-image IMAGE`, it generates an ephemeral Ed25519 key only in memory,
starts a bounded loopback JWKS fixture on an OS-assigned port, launches the
image with Docker host networking and
`IR_JWKS_URL=http://127.0.0.1:<port>/jwks`, and signs the three short-lived test
tickets. Literal loopback HTTP is already accepted by production config, so no
insecure-host gate is added. The script owns and stops the container/JWKS
fixture in `finally`, including on assertion failure. No private key or fixture
is written to disk or copied into the image.

Drive:

```text
1. minor-0 and minor-1 browse; both admission attempts are update-required
2. three minor-2 seats load one exact chart length and start
3. telemetry A=100, B=100, C=90 yields live ranks 1,1,3
4. no-data and zero-score remain distinct; strongest-opponent candidate rows change
5. one ephemeral standings event is dropped and the next full snapshot repairs it
6. finals 100,100,90 finalize ranks 1,1,3 and increment A/B wins
7. room returns Selecting and retains one last result
8. a second round reconnects A, B abandons, C expires at deadline
9. valid finisher finalizes; waiting seat observes but cannot submit
10. room destruction removes result, telemetry, identities, and wins
```

Assert no telemetry/result/leaderboard message contains an IR score GUID or
upload state and no Arena server request targets the IR score endpoint.

- [ ] **Step 3: Run complete automated verification**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase3
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena test:server -- src/lib/server/auth/tests/arena-ticket.test.ts
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "Arena|GeneratePermutation|ChartLoader" --output-on-failure
```

Report database/Docker environmental blockers explicitly; do not claim an
unrun root test passed.

- [ ] **Step 4: Verify Docker/Linux competition behavior**

```powershell
docker build -t rhythmgame-arena:phase3 T:/RhythmGame-IR/.worktrees/online-arena/arena-server
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase3 -- --docker-image rhythmgame-arena:phase3
```

The Docker mode chooses free host ports and passes `--network host`; lack of
Docker host-network support is an explicit environment blocker, not a reason
to weaken authentication or permit non-loopback HTTP. Query health before the
first authenticated request and assert the fixture saw no JWKS fetch. Confirm
200 ms coalescing, 20th telemetry-abuse policy close, deadline timer, ephemeral
backpressure recovery, reliable-send overflow close/resume, finalization,
capacities, non-root runtime, SIGTERM cleanup, and absence of IR database/
signing/score credentials.

- [ ] **Step 5: Perform privacy and bounded-state review**

Inject sentinel ticket/password/chat/hash/path/GUID/identity/EX/judgement/gauge
values and search logs, errors, crash strings, and reports. Prove at most 16
newest telemetry records, 20 limiter timestamps per seat, one pending terminal
per client, and one last result per room. Prove release on finalization,
leave/kick/grace, disconnect failure, shutdown, and room destruction.

- [ ] **Step 6: Perform representative-skin and independence matrix**

Use at least two app instances and Default/LR2/Beatoraja gameplay/result. Verify
compact/expanded standings, strongest-opponent/no-opponent, F8 chat/no pause,
abort/reconnect/deadline, pending/final ties, winners/wins, Default source cycle,
179/180, IR failure with Arena success, Arena failure with local/IR success,
and ordinary offline/local-battle target/result regressions.

- [ ] **Step 7: Write traceability and commit integration-owned files**

`phase3-verification.md` maps every spec exit criterion to exact automated
output or manual observation and records environmental limitations.

```powershell
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/scripts/phase3-smoke.ts arena-server/package.json arena-server/README.md arena-server/Dockerfile arena-server/.dockerignore
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "test: verify Arena phase 3 integration"
git -C T:/RG/.worktrees/online-arena add docs/arena/phase3-verification.md
git -C T:/RG/.worktrees/online-arena commit -m "docs: verify Arena phase 3"
```

---

## Final review gate

After Task 11, run independent reviews for:

1. Protocol/spec parity, capability downgrade safety, strict schemas, and no
   Phase 4 scope leakage.
2. Telemetry arithmetic/progression, limiter boundaries, coalescing,
   backpressure, memory release, and privacy.
3. Playing deadline, terminal idempotence, DNF lifecycle, finalization once,
   tie ranks, wins, and last-result invariants.
4. Qt model contracts, QObject lifetimes/thread affinity, sampling/timer races,
   reconnect queues, and result GUID correlation.
5. QML event/Shortcut ownership, no-pausing chat, abort priority, transient
   targets, Default provider persistence, LR2/Beatoraja 179/180, and overlay
   input capture.
6. Normal local save/IR submission independence, non-Arena regressions, whole-
   branch tests, and Docker/Linux behavior.

Critical or Important findings are fixed and re-reviewed before Phase 4 begins.
