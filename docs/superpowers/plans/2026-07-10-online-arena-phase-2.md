# Online Arena Phase 2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish exact chart libraries, select one common chart with a complete immutable transformation, and synchronize every frozen participant through probe, load, and future start.

**Architecture:** Extend protocol major 1 with negotiated minor 1 and `rounds-v1`. The Bun room domain owns committed packed inventories, exact next-round common state, selection/readiness, and frozen launch transitions; connection-bound partial transfers stay in an upload manager. `ArenaSession` remains the QML-facing deep module and delegates SQLite inventory construction and deterministic runner creation to injected production/fake adapters.

**Tech Stack:** Bun 1.3.14, TypeScript 5.9, Zod 4, WebSocket JSON plus bounded binary frames, Qt 6 C++23, Qt WebSockets/Concurrent/SQL wrapper, QML, Catch2, CMake/CTest, Docker/Coolify.

## Global Constraints

- The controlling specification is `docs/superpowers/specs/2026-07-10-online-arena-phase-2-design.md`.
- Protocol major is `1`; current minor is `1`; `rooms-v1` gates browsing and `rounds-v1` gates create/join and every Phase 2 mutation.
- A 1.0 `rooms-v1` client may browse anonymously but cannot create or join a Phase 2 room.
- SHA-256 is canonical lowercase hex in JSON and raw 32-byte values in binary vectors.
- One inventory is at most 250,000 hashes and 8,000,000 raw bytes.
- Every binary frame is at most 65,536 bytes with the exact 32-byte `RGA1` header and at most 2,047 hashes.
- Client JSON remains capped at 65,536 UTF-8 bytes; server JSON remains capped at 4 MiB.
- No inventory, selection, availability, activation, ready, or round field is added to `ChartData`.
- The full selection contains realized `#RANDOM`, all 11 existing P1/P2 note-order values, four DP values, a 64-bit hex seed, and randomization version 1.
- Gauge, hi-speed, lane cover, lift/hidden, BGA, offset, target, and presentation settings remain personal and are never written by Arena.
- `ProfileList::battleActive` remains silently forced off while Arena is active; synchronized DP `battle` remains a distinct allowed transformation.
- Courses, replay, autoplay, telemetry, leaderboard, results, DNF, lobby-win mutation, result adapters, and overlay placement are not implemented in Phase 2.
- Partial transfers, file paths, tickets, passwords, individual hashes, vectors, and chat bodies never enter logs or public error strings.
- Existing unrelated dirty files and in-progress Phase 1 documents are never staged or rewritten.
- Each task is reviewed before the next task starts. Commit separately in `T:/RhythmGame-IR/.worktrees/online-arena` and `T:/RG/.worktrees/online-arena`.

---

## File map

### RhythmGame-IR / Arena server

- `arena-server/src/protocol/messages.ts`: protocol 1.1 text schemas and exact round DTOs.
- `arena-server/src/protocol/binary.ts`: `RGA1` binary header codec.
- `arena-server/src/protocol/errors.ts`: stable Phase 2 errors.
- `arena-server/src/inventory/packed-inventory.ts`: immutable packed-vector validation/intersection/delta.
- `arena-server/src/inventory/inventory-upload-manager.ts`: partial uploads, budgets, digest/order/deadline.
- `arena-server/src/inventory/availability-transfer.ts`: reset/delta planning and binary delivery construction.
- `arena-server/src/rooms/round-state.ts`: inventory basis, selection, ready, probe/load/start state transitions.
- Existing `rooms/models.ts`, `room.ts`, and `room-directory.ts`: seat/room integration and public transitions.
- Existing `application/arena-application.ts` and `delivery.ts`: text/binary orchestration.
- Existing `transport/start-server.ts`: bounded binary gateway and exact deadline scheduling.
- `arena-server/tests/unit/*` and `tests/integration/*`: deterministic domain/protocol/gateway coverage.

### RhythmGame client

- Existing `src/arena/ArenaTypes.h` and `ArenaProtocol.*`: protocol 1.1 values/text codec.
- `src/arena/ArenaBinaryProtocol.h/.cpp`: strict binary header/vector codec.
- `src/arena/ArenaAvailabilityIndex.h/.cpp`: compact atomic common-set index.
- `src/arena/ArenaInventorySource.h`: fakeable library generation/snapshot seam.
- `src/arena/SqliteArenaInventorySource.h/.cpp`: production SQLite worker adapter.
- `src/arena/ArenaRoundLoader.h`: fakeable selection/probe/load seam.
- `src/arena/QtArenaRoundLoader.h/.cpp`: production adapter over ChartLoader/profile/song DB.
- `src/resource_managers/ChartPlayConfig.h`: generic explicit chart transformation.
- Existing `src/qml_components/ChartLoader.*`: explicit-config load path.
- Existing `src/gameplay_logic/ChartRunner.*`: generic held-start seam.
- Existing `src/qml_components/RootSongFoldersConfig.*`: true queue-drained generation.
- Existing `src/arena/ArenaSession.*`: Phase 2 orchestration and QML-facing state.
- `RhythmGameQml/Arena/ArenaSelectStrip.qml`: minimal skin-independent selection/ready/loading controls.
- Existing Default/LR2/Beatoraja select paths: activation and availability presentation only.
- `test/arena/*`: protocol, index, source, loader, session, and integration tests/fakes.

---

### Task 1: Negotiate protocol 1.1 and define the shared Phase 2 text contract

**Files:**
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/src/lib/server/auth/arena-ticket.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/src/lib/server/auth/tests/arena-ticket.test.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/messages.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/codec.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/errors.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/codec.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/fixtures/phase2-text-goldens.json`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaTypes.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaProtocol.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaProtocol.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/ArenaProtocol.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/fixtures/phase2-text-goldens.json`

**Interfaces:**
- Consumes: Phase 1 `ClientMessage`/`ServerMessage`, strict codecs, room/member/snapshot DTOs, and ticket verifier minor compatibility.
- Produces: negotiated hello, exhaustive `SelectionSnapshot`, Phase 2 client/server variants, room/member/round view types, and byte-identical golden fixture copies consumed by Tasks 2-11.

- [ ] **Step 1: Add failing negotiation and text-golden tests**

Cover client `1.0 + rooms-v1`, `1.1 + rooms-v1 + rounds-v1`, unsupported major,
duplicate capabilities, server intersection ordering, and create/join capability
gating. Add a client test for exactly one anonymous minor-0 reconnect after the
old server's initial `protocol_incompatible`, with login/create/join/resume disabled
and no fallback loop. Add one positive and one strict-invalid golden for every
new message.

The exact new variants are:

```text
Client:
inventory_upload_begin, inventory_upload_commit, inventory_upload_abort,
availability_applied, availability_resync, selection_set, ready_set,
round_probe_result, round_load_result

Server:
inventory_upload_ready, inventory_committed,
availability_transfer_begin, availability_transfer_commit,
selection_changed, selection_rejected,
round_loading_started, round_probe_requested, round_load_requested,
round_start_scheduled, round_started, round_launch_cancelled
```

Extend room phase to `selecting|loading|playing`, member views with the five
Phase 2 fields, and room snapshots with selection/availability/round state.

- [ ] **Step 2: Run RED in both repositories**

```powershell
bun --cwd arena-server test tests/unit/codec.test.ts
ctest --preset dev-rel -R ArenaProtocol --output-on-failure
```

Expected: existing codecs reject protocol minor 1 and the new fixture variants.

- [ ] **Step 3: Implement negotiation and strict schemas**

Use these canonical constants and values on both sides:

```ts
export const PROTOCOL_MAJOR = 1 as const;
export const PROTOCOL_MINOR = 1 as const;
export const ROOMS_CAPABILITY = 'rooms-v1' as const;
export const ROUNDS_CAPABILITY = 'rounds-v1' as const;
```

The hello decoder accepts client minor `0..1`, computes the ordered server
intersection, and returns negotiated minor `Math.min(clientMinor, 1)`. Do not
weaken strict nested objects, field bounds, request correlation, sensitive
error sanitation, or revision/generation positivity.

On the C++ protocol side, support encoding the normal minor-1 hello with both
capabilities and a minor-0 `rooms-v1` hello. Task 9 adds the single-use pre-auth
legacy fallback: only the initial `protocol_incompatible` may reconnect
anonymously with the latter hello; successful fallback is browse-only and
cannot issue authentication, create, join, or resume. Reset the fallback
allowance only on a new explicit Arena entry.

Define `SelectionSnapshot` exactly as the spec, with the 11 closed note-order
strings, four DP strings, positive safe-integer random sequence capped at 4096,
16-character seed, and version literal 1.

- [ ] **Step 4: Update the IR ticket claim to minor 1**

Change only `protocolMinor: 0` to `protocolMinor: 1` in the issuer and pin the
new literal in the real Better Auth cryptographic test. The server verifier
must continue accepting any non-negative safe minor.

- [ ] **Step 5: Run GREEN and fixture parity**

```powershell
bun run --cwd arena-server verify
ctest --preset dev-rel -R ArenaProtocol --output-on-failure
Compare-Object (Get-Content -Raw 'T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/fixtures/phase2-text-goldens.json') (Get-Content -Raw 'T:/RG/.worktrees/online-arena/test/arena/fixtures/phase2-text-goldens.json')
```

Expected: all suites pass and `Compare-Object` prints nothing.

- [ ] **Step 6: Commit both repositories**

```powershell
git -C T:/RhythmGame-IR/.worktrees/online-arena add src/lib/server/auth arena-server/src/protocol arena-server/tests/unit/codec.test.ts arena-server/tests/fixtures/phase2-text-goldens.json
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: define Arena round protocol"
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaTypes.h src/arena/ArenaProtocol.h src/arena/ArenaProtocol.cpp test/arena/ArenaProtocol.test.cpp test/arena/fixtures/phase2-text-goldens.json
git -C T:/RG/.worktrees/online-arena commit -m "feat: decode Arena round protocol"
```

---

### Task 2: Implement the binary frame codec and bounded upload manager

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/protocol/binary.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/inventory/inventory-upload-manager.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/delivery.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/start-server.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/config.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.env.example`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/binary-codec.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/inventory-upload-manager.test.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/websocket.test.ts`

**Interfaces:**
- Consumes: Task 1 inventory control DTOs and Phase 1 connection IDs/bindings.
- Produces:

```ts
type BinaryKind = 1 | 2 | 3 | 4;
type DecodedHashChunk = Readonly<{
  kind: BinaryKind;
  transferId: Uint8Array;
  chunkIndex: number;
  hashes: Uint8Array;
}>;

interface InventoryUploadManager {
  begin(connectionId: string, declaration: InventoryDeclaration, nowMs: number): BeginResult;
  append(connectionId: string, frame: Uint8Array, nowMs: number): AppendResult;
  commit(connectionId: string, uploadId: string, declaration: InventoryDeclaration, nowMs: number): CommitResult;
  abortConnection(connectionId: string): void;
  sweep(nowMs: number): readonly ExpiredUpload[];
}
```

`CommitResult` contains one validated immutable `PackedInventory`; no staged
byte array is exposed to rooms.

- [ ] **Step 1: Write failing byte-golden and upload lifecycle tests**

Test exact offsets/magic/version/reserved zero, all four kinds, 0/1/2047 hash
payloads, 65,536/65,537 boundaries, sequence and cross-chunk ordering,
duplicates, digest/count/byte/chunk mismatch, empty inventory, supersede,
abort, connection cleanup, exact 60-second expiry, six-per-minute identity
limit hook, and 128/512 MiB budgets.

- [ ] **Step 2: Run RED**

```powershell
bun --cwd arena-server test tests/unit/binary-codec.test.ts tests/unit/inventory-upload-manager.test.ts
```

Expected: missing `binary.ts` and upload manager.

- [ ] **Step 3: Implement the exact 32-byte codec**

Use constants rather than implicit offsets:

```ts
export const BINARY_MAGIC = new Uint8Array([0x52, 0x47, 0x41, 0x31]);
export const BINARY_HEADER_BYTES = 32;
export const MAX_HASHES_PER_CHUNK = 2_047;
export const MAX_BINARY_FRAME_BYTES = 65_536;
```

Decode into new owned bounded bytes. Never retain the Bun frame object or put
its bytes into a thrown error.

- [ ] **Step 4: Implement upload state and memory accounting**

Store chunks incrementally, validate every addition before accounting, and
coalesce only at commit. Use constant-time digest comparison, release every
buffer on terminal paths, and keep committed inventory outside this manager.
Add validated config defaults:

```text
INVENTORY_UPLOAD_TIMEOUT_MS=60000
MAX_PENDING_INVENTORY_BYTES=134217728
MAX_COMMITTED_INVENTORY_BYTES=536870912
```

- [ ] **Step 5: Extend delivery/gateway for binary without weakening ordering**

Add `Delivery { kind: 'send_binary', connectionIds, bytes }`. Incoming string
frames still use `decodeClientMessage`; incoming binary frames go through a
new serialized `ArenaApplication.receiveBinary` call in the same per-socket
promise tail. Set `maxPayloadLength` to 65,536 and retain the queue/backpressure
policy. Binary outside an expected room upload closes with a redacted fatal
error.

- [ ] **Step 6: Verify and commit**

```powershell
bun run --cwd arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/protocol/binary.ts arena-server/src/inventory/inventory-upload-manager.ts arena-server/src/application/delivery.ts arena-server/src/transport/start-server.ts arena-server/src/config.ts arena-server/.env.example arena-server/tests
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: receive bounded Arena inventories"
```

---

### Task 3: Store exact inventories and publish atomic common availability

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/inventory/packed-inventory.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/inventory/availability-transfer.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/models.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room-directory.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/arena-application.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/main.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/packed-inventory.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/room-inventory.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/availability-transfer.test.ts`

**Interfaces:**
- Consumes: validated `PackedInventory` from Task 2.
- Produces these additions to `RoomDirectory`:

```ts
replaceInventory(actor, input, inventory, nowMs): DomainResult<InventoryCommit>;
markInventorySyncing(actor, libraryGeneration, nowMs): DomainResult<UploadAdmission>;
abortInventorySync(actor, libraryGeneration, nowMs): DomainResult<void>;
ackAvailability(actor, revision, nowMs): DomainResult<void>;
requestAvailabilityReset(actor, nowMs): DomainResult<AvailabilitySnapshot>;
```

`PackedInventory` exposes count/raw bytes, `contains(hash)`,
`intersect(other)`, and `deltaTo(next)` without string arrays.

- [ ] **Step 1: Write failing packed-vector and room-basis tests**

Cover identical/disjoint/partial vectors; add/remove/replace member; reserved
seat inclusion; waiting join during active round; revision zero; strictly
increasing seat library generation; authoritative replacement; committed-byte
budget release on leave/kick/grace/destruction; and selection/ready reset hooks
as observable placeholders in the domain result (no selection logic yet).

- [ ] **Step 2: Run RED**

```powershell
bun --cwd arena-server test tests/unit/packed-inventory.test.ts tests/unit/room-inventory.test.ts tests/unit/availability-transfer.test.ts
```

- [ ] **Step 3: Implement packed set operations and room state**

Extend each seat with `libraryGeneration`, `inventoryRevision`,
`inventoryState`, optional committed vector, and applied availability revision.
Extend each room with the current compact common vector, sorted basis, and
availability revision. Recompute synchronously through the pure packed module;
room/application code must not implement its own intersection loop.

- [ ] **Step 4: Implement reset/delta planning and acknowledgement**

Choose reset after join/resume/resync/gap or when delta records are not smaller
than reset. Encode one transfer declaration, ordered binary chunks, and commit
event as one ordered delivery batch. Until `availability_applied`, the seat is
not ready-capable. A newer common revision sends reset to an unacknowledged
seat rather than chaining a delta from an unknown base.

- [ ] **Step 5: Wire application upload begin/commit/abort/resync/ack**

`ArenaApplication` validates the current binding and `rounds-v1`, delegates
partial bytes to Task 2, commits only the returned packed value, and maps room
effects to text/binary deliveries. Resume discards partial transfers and sends
a targeted full reset after the authoritative room snapshot.

- [ ] **Step 6: Verify, record performance, and commit**

```powershell
bun run --cwd arena-server verify
bun --cwd arena-server test tests/unit/packed-inventory.test.ts --timeout 30000
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/inventory arena-server/src/rooms arena-server/src/application/arena-application.ts arena-server/src/main.ts arena-server/tests/unit
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: compute Arena common charts"
```

Record 88,000- and 250,000-hash timings in the task report; do not make a
machine-dependent wall-clock assertion.

---

### Task 4: Add immutable selection and all-ready freeze to the server domain

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/round-state.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/models.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room-directory.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/arena-application.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/selection-ready.test.ts`

**Interfaces:**
- Consumes: Task 3 authoritative common set/basis and Task 1 selection DTO.
- Produces:

```ts
select(actor, snapshot, revisions, nowMs): DomainResult<SelectionCommit>;
setReady(actor, ready, revisions, nowMs): DomainResult<ReadyCommit>;
```

`ReadyCommit` may contain a newly frozen `RoundLoadingState` with generated
round/attempt IDs and per-participant probe effects.

- [ ] **Step 1: Write failing selection/ready state-machine tests**

Cover all 11 note-order/four DP values and bounds, any-member selection,
last-accepted ordering, non-common rejection with missing member IDs, stale
inventory/availability/selection, no mutation on rejection, all ready-reset
triggers, disconnected eligible seat, one-player freeze, atomic frozen basis,
and joining after freeze as waiting.

- [ ] **Step 2: Run RED**

```powershell
bun --cwd arena-server test tests/unit/selection-ready.test.ts
```

- [ ] **Step 3: Implement `round-state.ts` as a pure transition helper**

Keep room/sockets/password/chat outside this file. It owns only selection,
ready, frozen roster, revisions, and round stages. Generate 128-bit round and
launch IDs through the existing injected entropy seam. An accepted selection
clears ready; a rejected selection changes nothing.

- [ ] **Step 4: Integrate RoomDirectory and application mapping**

Add `selection_set` and `ready_set` handlers after the standard binding
preflight. Extend room/member/snapshot copies and directory phase. Emit one
ordered transition where the final ready change and `round_loading_started`
cannot be interleaved with another selection.

- [ ] **Step 5: Verify and commit**

```powershell
bun run --cwd arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/rooms arena-server/src/application/arena-application.ts arena-server/tests/unit/selection-ready.test.ts
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: coordinate Arena selection and ready"
```

---

### Task 5: Implement probe, deterministic load barrier, waiting joins, and scheduled start

**Files:**
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/round-state.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/rooms/room-directory.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/arena-application.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/transport/start-server.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/src/application/delivery.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/unit/round-loading.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/round-start.test.ts`

**Interfaces:**
- Consumes: Task 4 frozen round and existing heartbeat nonce timing.
- Produces:

```ts
reportProbe(actor, result, nowMs): DomainResult<void>;
reportLoaded(actor, result, nowMs): DomainResult<void>;
sweep(nowMs): readonly RoomTransition[];
nextDeadlineMs(): number | undefined;
```

Effects include targeted probe/load/start schedule, launch cancellation, and
the server-side Playing transition.

- [ ] **Step 1: Write failing fake-time launch tests**

Cover unique nonces, exact binding/round/attempt/revision checks, duplicate and
conflicting replies, 15,000/60,000 ms boundaries, every cancellation reason,
selection retain/clear policy, frozen participant leave/kick, waiting member
leave/kick, resume at each stage, recent-eight minimum RTT samples, 2,000-
5,000 ms lead, connection-specific delay, cancellation during lead, and exact
Playing transition.

- [ ] **Step 2: Run RED**

```powershell
bun --cwd arena-server test tests/unit/round-loading.test.ts
```

- [ ] **Step 3: Implement stages and cancellation in the domain**

Use `probing -> loading -> scheduled -> playing`. Keep per-participant stage,
nonce, deadline, and immutable inventory revision. Loading/Playing next-round
inventory changes update only next-round state. Return complete authoritative
selection/revision state in every cancellation effect.

- [ ] **Step 4: Add exact deadline scheduling to the gateway**

Expose `ArenaApplication.nextDeadlineMs()`. Maintain one rescheduled one-shot
timer for the earliest room deadline in addition to the coarse heartbeat/
grace sweep. Reschedule after receive, binary receive, disconnect, and sweep.
Shutdown cancels both timers.

- [ ] **Step 5: Add real WebSocket round integration**

Drive two authenticated clients through inventory, common ack, selection,
ready, probe, load, targeted start, and server Playing. Add a third waiting
join and one cancellation case. Keep telemetry/result out of the harness.

- [ ] **Step 6: Verify and commit**

```powershell
bun run --cwd arena-server verify
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/src/rooms arena-server/src/application arena-server/src/transport/start-server.ts arena-server/tests
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "feat: synchronize Arena round start"
```

---

### Task 6: Decode binary transfers and implement `ArenaAvailabilityIndex`

**Files:**
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaBinaryProtocol.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaBinaryProtocol.cpp`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaAvailabilityIndex.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaAvailabilityIndex.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaMemberListModel.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaMemberListModel.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaRoomListModel.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaRoomListModel.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaBinaryProtocol.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaAvailabilityIndex.test.cpp`

**Interfaces:**
- Consumes: Task 1 C++ text values and Task 2 binary format.
- Produces:

```cpp
class ArenaAvailabilityIndex final : public QObject {
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY changed FINAL)
    Q_PROPERTY(qint64 revision READ revision NOTIFY changed FINAL)
  public:
    enum class State { NotApplicable, Syncing, Ready };
    enum class Availability { NotApplicable, Syncing, AvailableToAll, UnavailableToSome };
    Q_ENUM(State)
    Q_ENUM(Availability)
    [[nodiscard]] auto availability(QStringView sha256Hex) const -> Availability;
    [[nodiscard]] auto applyReset(qint64 targetRevision, QByteArray packed) -> bool;
    [[nodiscard]] auto applyDelta(qint64 baseRevision, qint64 targetRevision,
                                  QByteArray added, QByteArray removed) -> bool;
    void setSyncing();
    void clear();
  signals:
    void changed();
};
```

- [ ] **Step 1: Write failing binary/index/model-role tests**

Mirror every server byte golden and malformed case. Test reset/delta
atomically, stale base, duplicate/disjoint/sorted validation, lookup, state,
one notification, previous-state retention on failure, and 250,000 hashes.
Extend exact room phase and member role tests for ready/inventory/availability/
round state.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R "ArenaBinaryProtocol|ArenaAvailability|ArenaModels" --output-on-failure
```

- [ ] **Step 3: Implement the codec and compact sorted index**

Use one `QByteArray` containing `count * 32` bytes and binary search/merge over
fixed-width records. Validate into local values and swap only after commit.
Errors are closed enums without raw bytes. Do not use `QSet<QString>` or create
one QObject per hash.

- [ ] **Step 4: Extend value-row models tightly**

Add exact roles from the spec. Snapshot replacement installs all context in one
reset; incremental updates emit only changed roles. Inventory bytes, user IDs,
transfer IDs, nonces, and generations remain absent.

- [ ] **Step 5: Verify and commit**

```powershell
ctest --preset dev-rel -R "ArenaBinaryProtocol|ArenaAvailability|ArenaModels" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaBinaryProtocol.* src/arena/ArenaAvailabilityIndex.* src/arena/ArenaMemberListModel.* src/arena/ArenaRoomListModel.* CMakeLists.txt test/CMakeLists.txt test/arena
git -C T:/RG/.worktrees/online-arena commit -m "feat: track Arena common charts"
```

---

### Task 7: Add true library generations and the SQLite inventory source

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/src/qml_components/RootSongFoldersConfig.h`
- Modify: `T:/RG/.worktrees/online-arena/src/qml_components/RootSongFoldersConfig.cpp`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaInventorySource.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/SqliteArenaInventorySource.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/SqliteArenaInventorySource.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/FakeArenaInventorySource.h`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaInventorySource.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: song DB path and committed mutation signals.
- Produces:

```cpp
struct ArenaInventorySnapshot {
    qint64 libraryGeneration{};
    QByteArray packedSha256;
};

class ArenaInventorySource : public QObject {
    Q_OBJECT
  public:
    [[nodiscard]] virtual auto generation() const -> qint64 = 0;
    virtual void requestSnapshot(quint64 requestId) = 0;
    virtual void cancel(quint64 requestId) = 0;
  signals:
    void generationChanged(qint64 generation);
    void snapshotReady(quint64 requestId, arena::ArenaInventorySnapshot snapshot);
    void snapshotFailed(quint64 requestId, arena::ArenaInventoryFailure failure);
};
```

- [ ] **Step 1: Write failing queue-drain/generation/build-race tests**

Test two queued root scans produce one generation only after final removal,
stop/cancel reaches idle once, root removal increments after DB cleanup,
snapshot sort/distinct/decode, invalid hash, 250,000/250,001, generation change
during worker query, cancellation, and newest-generation coalescing.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaInventorySource --output-on-failure
```

- [ ] **Step 3: Add explicit mutation signals**

`ScanningQueue` emits `queueDrained` only on the nonempty-to-empty transition
after the final watcher completion and no `performTask()` call. `RootSongFolders`
emits `chartSetMutationCommitted` after synchronous removal cleanup. A small
process-lifetime generation owner initializes to `1` after the database is
available and increments once per signal. Generation zero is never published.

- [ ] **Step 4: Implement the SQLite worker adapter**

Open a separate connection from the injected song DB path, execute the exact
ordered `DISTINCT lower(sha256)` query with `LIMIT 250001`, reject when the
sentinel row exists, validate/decode at most 250,000 rows into packed bytes,
and post the owned result to the adapter thread. Compare captured/current
generation before emitting. At most one query and one newest pending rebuild
exist.

- [ ] **Step 5: Verify scanner regressions and commit**

```powershell
ctest --preset dev-rel -R "ArenaInventorySource|SongDbScanner" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/qml_components/RootSongFoldersConfig.* src/arena/ArenaInventorySource.h src/arena/SqliteArenaInventorySource.* test/arena/FakeArenaInventorySource.h test/arena/ArenaInventorySource.test.cpp CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: publish Arena library generations"
```

---

### Task 8: Build selections and load an explicit immutable play configuration

**Files:**
- Create: `T:/RG/.worktrees/online-arena/src/resource_managers/ChartPlayConfig.h`
- Modify: `T:/RG/.worktrees/online-arena/src/qml_components/ChartLoader.h`
- Modify: `T:/RG/.worktrees/online-arena/src/qml_components/ChartLoader.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/gameplay_logic/ChartRunner.h`
- Modify: `T:/RG/.worktrees/online-arena/src/gameplay_logic/ChartRunner.cpp`
- Create: `T:/RG/.worktrees/online-arena/src/arena/ArenaRoundLoader.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/QtArenaRoundLoader.h`
- Create: `T:/RG/.worktrees/online-arena/src/arena/QtArenaRoundLoader.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/FakeArenaRoundLoader.h`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaRoundLoader.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: active profile, selected `ChartData`, song DB SHA-256 path lookup,
  Task 1 `SelectionSnapshot`, and existing ChartData/ChartFactory pipeline.
- Produces:

```cpp
struct ChartPlayConfig {
    QList<qint64> randomSequence;
    NoteOrderAlgorithm noteOrderP1{ NoteOrderAlgorithm::Normal };
    NoteOrderAlgorithm noteOrderP2{ NoteOrderAlgorithm::Normal };
    DpOptions dpMode{ DpOptions::Off };
    quint64 laneSeed{};
    int randomizationVersion{ 1 };
};

enum class ArenaSelectionBuildFailure {
    InvalidChart,
    InvalidSha256,
    InvalidRandomSequence,
    UnsupportedConfig,
};

using ArenaSelectionBuildResult =
  std::expected<arena::SelectionSnapshot, ArenaSelectionBuildFailure>;

enum class ArenaProbeFailure {
    None,
    MissingFile,
    HashMismatch,
    ReadFailed,
    Cancelled,
};

struct ArenaProbeResult {
    ArenaProbeFailure failure{ ArenaProbeFailure::None };
    QByteArray observedSha256;
};

enum class ArenaLoadFailure {
    MissingFile,
    HashMismatch,
    ParseFailed,
    UnsupportedConfig,
    ResourceFailed,
    Cancelled,
};

struct ArenaRoundLoadRequest {
    QByteArray sha256;
    ChartPlayConfig playConfig;
};

class ArenaRoundLoader : public QObject {
    Q_OBJECT
  public:
    virtual auto buildSelection(gameplay_logic::ChartData* chart)
      -> ArenaSelectionBuildResult = 0;
    virtual void probe(quint64 requestId, const QByteArray& sha256) = 0;
    virtual void load(quint64 requestId,
                      const ArenaRoundLoadRequest& request) = 0;
    virtual void cancel(quint64 requestId) = 0;
  signals:
    void probeFinished(quint64 requestId, arena::ArenaProbeResult result);
    void loadFinished(quint64 requestId, gameplay_logic::ChartRunner* runner);
    void loadFailed(quint64 requestId, arena::ArenaLoadFailure failure);
};
```

- [ ] **Step 1: Write failing exhaustive transform and held-start tests**

For one deterministic chart fixture, cover all 11 P1/P2 enum mappings, four DP
modes, `#RANDOM`, exact 64-bit seed, version rejection, SHA path rehash,
profile-variable immutability, runner Ready only after resources, cancel, and:

```cpp
runner.holdStart();
runner.start();
CHECK(runner.getStatus() == ChartRunner::Ready);
runner.releaseStart();
CHECK(runner.getStatus() == ChartRunner::Running);
```

Also prove an ordinary non-held runner retains existing behavior.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaRoundLoader --output-on-failure
```

- [ ] **Step 3: Refactor ChartLoader around explicit `ChartPlayConfig`**

Keep normal and replay QML methods behavior-identical by deriving a config as
they do today. Add a non-QML C++ load path that accepts the config directly,
passes its values into `ChartFactory::PlayerSpecificData`, and never edits
GeneralVars. Reject unsupported version before file parsing.

- [ ] **Step 4: Implement selection/probe/load adapter**

Capture only SHA/display metadata, current chart random sequence, P1/P2 note
order, DP mode, new 64-bit seed, and version. Resolve probe/load paths by local
database SHA-256 only. Hash file bytes on a worker. Retain a prepared runner
until accepted/cancelled, watch `ChartRunner::Ready`, and never log a path or
raw hash on failure.

- [ ] **Step 5: Implement generic held start**

`holdStart()` is valid only before Running; `start()` while held latches one
request; `releaseStart()` is idempotent and starts only when latched and Ready.
Destruction/cancel leaves no timer or callback. Do not add Arena knowledge to
`ChartRunner`.

- [ ] **Step 6: Verify and commit**

```powershell
ctest --preset dev-rel -R "ArenaRoundLoader|GeneratePermutation|ChartLoader" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/resource_managers/ChartPlayConfig.h src/qml_components/ChartLoader.* src/gameplay_logic/ChartRunner.* src/arena/ArenaRoundLoader.h src/arena/QtArenaRoundLoader.* test/arena/FakeArenaRoundLoader.h test/arena/ArenaRoundLoader.test.cpp CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: load immutable Arena play config"
```

---

### Task 9: Orchestrate inventory, selection, readiness, and launch in `ArenaSession`

**Files:**
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/ArenaSession.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/QtWebSocketArenaTransport.h`
- Modify: `T:/RG/.worktrees/online-arena/src/arena/QtWebSocketArenaTransport.cpp`
- Modify: `T:/RG/.worktrees/online-arena/src/main.cpp`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Rg.h`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Rg.cpp`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/FakeArenaTransport.h`
- Modify: `T:/RG/.worktrees/online-arena/test/arena/ArenaSession.test.cpp`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaSessionRounds.test.cpp`
- Modify: `T:/RG/.worktrees/online-arena/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/test/CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 6-8 adapters/index and Phase 1 session lifecycle.
- Produces the additional QML-facing surface:

```cpp
Q_PROPERTY(arena::RoomPhase roomPhase READ roomPhase NOTIFY roundChanged FINAL)
Q_PROPERTY(arena::ArenaAvailabilityIndex* availability READ availability CONSTANT FINAL)
Q_PROPERTY(bool canSelect READ canSelect NOTIFY selectionChanged FINAL)
Q_PROPERTY(bool canReady READ canReady NOTIFY readyChanged FINAL)
Q_PROPERTY(bool ready READ ready NOTIFY readyChanged FINAL)
Q_PROPERTY(bool availabilitySyncing READ availabilitySyncing NOTIFY availabilityChanged FINAL)
Q_PROPERTY(bool roundsAvailable READ roundsAvailable NOTIFY capabilitiesChanged FINAL)
Q_PROPERTY(QString selectedTitle READ selectedTitle NOTIFY selectionChanged FINAL)
Q_PROPERTY(QString selectedByMemberId READ selectedByMemberId NOTIFY selectionChanged FINAL)
Q_PROPERTY(qint64 selectionRevision READ selectionRevision NOTIFY selectionChanged FINAL)
Q_PROPERTY(QString currentRoundId READ currentRoundId NOTIFY roundChanged FINAL)

Q_INVOKABLE void selectChart(gameplay_logic::ChartData* chart);
Q_INVOKABLE void setReady(bool ready);

signals:
void capabilitiesChanged();
void preparedGameplayChanged(gameplay_logic::ChartRunner* runner);
void roundRunnerStarted(const QString& roundId,
                        gameplay_logic::ChartRunner* runner);
void roundLaunchCancelled();
```

- [ ] **Step 1: Write failing deterministic session scenarios**

Use fake transport, identity, scheduler, inventory source, round loader, and
binary frames. Cover join-triggered upload, generation coalescing, commit,
availability reset/delta/ack/resync, stale transfer and transport generation,
selection local/server rejection, ready prerequisites, full room snapshot,
waiting join, probe/load correlation, schedule/held start, cancellation,
resume reset, the one-shot pre-auth legacy browse fallback, profile/logout/
leave/exit cleanup, and no secret/path leakage.

- [ ] **Step 2: Run RED**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaSession --output-on-failure
```

- [ ] **Step 3: Extend the constructor and production object graph**

Inject non-owning `ArenaInventorySource*` and `ArenaRoundLoader*`; the session
owns its availability index and transfer staging. Construct production
adapters after Phase 1 dependencies and before `ArenaSession`. Configure
QWebSocket incoming frame/message limits to 65,536 for both text and binary.
Implement the one-shot legacy handshake described in Task 1 and derive
`roundsAvailable` only from the negotiated minor/capability intersection.

- [ ] **Step 4: Implement inventory/availability orchestration**

On admission request current generation, encode begin/chunks/commit only after
server ready, retain one generation/request, and rebuild if generation changed.
Decode common transfers into staging and apply/ack only on commit. Resume and
revision gaps force a reset; stale generations are ignored.

- [ ] **Step 5: Implement selection/ready/round orchestration**

`selectChart` locally requires room Selecting, Ready availability, and common
SHA, then asks the loader to build the immutable snapshot and sends one
selection command. `setReady(true)` sends current exact revisions only when all
local prerequisites hold. Probe/load/schedule callbacks carry local request
generations plus server round/attempt IDs. Profile/leave/cancel invalidates all
callbacks before destroying a prepared runner.

- [ ] **Step 6: Implement synchronized runner exposure**

When `round_start_scheduled` arrives, expose the held Ready runner, latch
`start()`, and schedule release with `ArenaScheduler`. On release emit
`roundRunnerStarted`; on cancellation cancel the task and emit
`roundLaunchCancelled` before destroying/popping the runner.

- [ ] **Step 7: Verify and commit**

```powershell
ctest --preset dev-rel -R "ArenaSession|ArenaBinaryProtocol|ArenaAvailability|ArenaInventorySource|ArenaRoundLoader" --output-on-failure
git -C T:/RG/.worktrees/online-arena add src/arena/ArenaSession.* src/arena/QtWebSocketArenaTransport.* src/main.cpp RhythmGameQml/Rg.* test/arena CMakeLists.txt test/CMakeLists.txt
git -C T:/RG/.worktrees/online-arena commit -m "feat: orchestrate Arena round loading"
```

---

### Task 10: Reuse normal select with Arena activation, availability, ready, and profile gates

**Required sub-skill:** Use `qt-qml` before editing QML.

**Files:**
- Create: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaSelectStrip.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaBrowser.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaLoginPanel.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Arena/ArenaRoom.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/CMakeLists.txt`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/List.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/ChartEntry.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/ReplayAutoplay.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/options/Options.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/scripts/select/options/Login.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SelectItemModel.h`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SelectItemModel.cpp`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SelectContext.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinValueResolver.qml`
- Modify: `T:/RG/.worktrees/online-arena/RhythmGameQml/Lr2/Lr2SkinScreenWrapper.qml`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_en.ts`
- Modify: `T:/RG/.worktrees/online-arena/share/RhythmGame/themes/Default/translations/Default_pl.ts`
- Create: `T:/RG/.worktrees/online-arena/test/arena/ArenaSelectIntegration.test.cpp`

**Interfaces:**
- Consumes: Task 9 QML surface and existing normal select APIs.
- Produces: one skin-independent strip and transient availability decoration;
  no theme networking or ChartData mutation.

- [ ] **Step 1: Add failing C++ adapter/activation policy tests**

Test exact LR2 availability role, revision-driven data changes, table-missing
versus Arena-unavailable distinction, course/replay/autoplay policy, profile
switch policy, legacy browse showing update-required with login/create/join
disabled, and Default/LR2 activation calling `selectChart` rather than
`openChart` when seated.

- [ ] **Step 2: Make ContentFrame open normal select after admission**

Keep the Browser beneath/available for return. Replace the Phase 1 holding room
with configured normal select plus `ArenaSelectStrip`. ArenaSession still owns
state; ContentFrame owns push/pop. Pre-start prepared gameplay is pushed during
the scheduled lead and popped on launch cancellation.

- [ ] **Step 3: Intercept Default activation and reuse red presentation**

In `List.openPlayable`, branch before local battle/replay handling:

```qml
if (Rg.arenaSession.state === ArenaSession.InRoom) {
    Rg.arenaSession.selectChart(item);
    return;
}
```

Bind chart color/enabled state to availability state/revision. Keep existing
non-ChartData table missing red behavior unchanged. Disable course,
ReplayAutoplay, ranking-play, and profile Login panel only while seated.

- [ ] **Step 4: Decorate LR2 and Beatoraja select**

Pass the session availability index into `Lr2SelectItemModel` and expose one
Arena availability role using the existing SHA role. LR2 prefixes translated
`(arena unavailable) ` only for a local uncommon chart. Beatoraja returns its
existing unavailable body/title types and no LR2 prefix. Activation routes to
`selectChart`; local missing entries keep current behavior.

- [ ] **Step 5: Implement the minimal universal select strip**

Show room/selection, inventory/common sync, self ready state, ready/unready
button, loading stage, and stable translated error. Render untrusted names as
plain text. The strip has no gameplay leaderboard, result standings, F2
customization, or persistent layout setting.

- [ ] **Step 6: Verify QML, C++ tests, and manual profile/battle behavior**

```powershell
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "ArenaSelect|ArenaSession|ArenaModels" --output-on-failure
```

Manual matrix: Default, LR2, and Beatoraja local/common/uncommon/table-missing;
any-player select; option change without reselection; ready; hidden profile
switch; attempted local battle remains off; DP Battle remains selectable;
external profile change returns Browser.

- [ ] **Step 7: Commit**

```powershell
git -C T:/RG/.worktrees/online-arena add RhythmGameQml/Arena RhythmGameQml/ContentFrame.qml RhythmGameQml/CMakeLists.txt RhythmGameQml/Lr2 share/RhythmGame/themes/Default/scripts/select share/RhythmGame/themes/Default/translations test/arena/ArenaSelectIntegration.test.cpp
git -C T:/RG/.worktrees/online-arena commit -m "feat: integrate Arena song select"
```

---

### Task 11: Cross-repository Phase 2 integration, limits, and release gate

**Files:**
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/tests/integration/phase2-two-client.test.ts`
- Create: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/scripts/phase2-smoke.ts`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/package.json`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/README.md`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/Dockerfile`
- Modify: `T:/RhythmGame-IR/.worktrees/online-arena/arena-server/.dockerignore`
- Create: `T:/RG/.worktrees/online-arena/docs/arena/phase2-verification.md`
- Modify only if parity failures require it: files owned by Tasks 1-10.

**Interfaces:**
- Consumes: the complete Phase 2 server/client implementation.
- Produces: one reproducible two-client contract smoke, Docker/Linux binary
  verification, resource/privacy evidence, and exit-criteria traceability.

- [ ] **Step 1: Freeze both heads and verify golden parity**

Record both SHAs in the task report. Compare text fixture bytes and add a binary
fixture generator that writes the same header/vector corpus for Bun and Catch2.
Any mismatch is fixed in the owning codec, never normalized in the test.

- [ ] **Step 2: Implement the exact scripted smoke**

Drive:

```text
1. anonymous 1.0 browse
2. two 1.1 authenticated seats join one password room
3. inventories {A,B,C} and {B,C,D} commit
4. both clients apply/ack common {B,C}
5. A selection is rejected and previous selection stays null
6. B then C selections are accepted; C is authoritative
7. both ready; frozen roster excludes a third loading-time join
8. exact probes and deterministic loads succeed
9. targeted future schedules arrive and room becomes Playing
10. a separate run covers hash mismatch cancellation and return to Selecting
```

Assert no telemetry/result/leaderboard event exists.

- [ ] **Step 3: Run all automated verification**

```powershell
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server verify
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame RhythmGame_test -j 2
ctest --preset dev-rel -R "Arena|GeneratePermutation|SongDbScanner" --output-on-failure
```

Run the IR Better Auth focused ticket test through its database-independent
config and the normal root suite when Docker/Postgres is available. Report any
baseline blocker without claiming it passed.

- [ ] **Step 4: Verify Docker/Linux binary behavior**

```powershell
docker build -t rhythmgame-arena:phase2 T:/RhythmGame-IR/.worktrees/online-arena/arena-server
docker run --rm -d --name rhythmgame-arena-phase2 -p 3001:3001 rhythmgame-arena:phase2
bun run --cwd T:/RhythmGame-IR/.worktrees/online-arena/arena-server smoke:phase2 -- ws://127.0.0.1:3001/ws
docker stop rhythmgame-arena-phase2
```

Confirm binary 65,536 succeeds, 65,537 closes, no private IR/DB/signing secret
is present, health never calls JWKS, non-root user remains, SIGTERM clears
partial uploads, and no cache/test/fixture secret is copied into the image.

- [ ] **Step 5: Perform privacy/resource review**

Search logs, errors, task reports, and crash strings for sentinel ticket,
password, path, hash, vector digest, and chat values. Exercise 128/512 MiB
budgets and prove release on abort/disconnect/leave/kick/grace/destruction.
Record 88,000/250,000 intersection and client index timings as measurements.

- [ ] **Step 6: Complete manual representative-skin verification**

Use two local app instances and Default/LR2/Beatoraja select. Verify common red
styling, existing table-missing styling, selector replacement, P1/P2/DP option
summary, profile menu absence, local battle lock, ready/loading/cancel/start,
waiting join, and external profile change. Confirm normal offline/local battle
still works after exiting Arena.

- [ ] **Step 7: Write traceability and commit final integration changes**

`phase2-verification.md` maps every spec exit criterion to an automated command
or manual observation and records environmental limitations. Then commit only
the integration-owned files and any narrowly reviewed parity fixes:

```powershell
git -C T:/RhythmGame-IR/.worktrees/online-arena add arena-server/tests/integration/phase2-two-client.test.ts arena-server/scripts/phase2-smoke.ts arena-server/package.json arena-server/README.md arena-server/Dockerfile arena-server/.dockerignore
git -C T:/RhythmGame-IR/.worktrees/online-arena commit -m "test: verify Arena phase 2 integration"
git -C T:/RG/.worktrees/online-arena add docs/arena/phase2-verification.md
git -C T:/RG/.worktrees/online-arena commit -m "docs: verify Arena phase 2"
```

---

## Final review gate

After Task 11, run independent reviews for:

1. Protocol/spec parity and no Phase 3/4 scope leakage.
2. Packed-inventory bounds, memory release, binary parsing, and privacy.
3. Room selection/ready/loading invariants and async race safety.
4. Qt model/index contracts, QObject lifetimes, thread affinity, and held-start
   behavior.
5. QML binding/event ownership, Default/LR2/Beatoraja availability semantics,
   and profile/battle gates.
6. Whole-branch tests and Docker/Linux deployment behavior.

Critical or Important findings are fixed and re-reviewed before Phase 3 begins.
