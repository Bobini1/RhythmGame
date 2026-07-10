# Online Arena Phase 2 design

## Status and authority

This specification derives from the approved Online Arena umbrella design and
the implemented Phase 1 seams. The user authorized autonomous assumptions and
implementation without another question or approval gate. If this document
conflicts with the umbrella design, the umbrella design wins; otherwise the
exact contracts below control Phase 2.

Phase 1 is a prerequisite. In particular, this design extends the existing
purpose-restricted IR ticket, Bun `ArenaApplication`/`RoomDirectory`, strict
protocol codec, binary-rejecting gateway, C++ `ArenaProtocol`, value-row
models, `ArenaSession`, transport/identity/scheduler seams, and application-
owned Arena navigation. It does not replace those modules.

## Goal

Deliver the complete pre-game Arena path:

- A seated client publishes one exact, bounded SHA-256 library inventory and
  republishes it after every committed library mutation.
- The server maintains a revisioned common chart set for every next-round
  eligible seat and transfers it atomically to clients.
- Normal Default, LR2, and Beatoraja song select remain visible and mark, but
  do not hide, charts that are not common.
- Any connected eligible member can replace the room selection with a common
  chart and a complete immutable note-transformation snapshot.
- Ready freezes the roster and selection atomically, then runs an exact local
  file probe, deterministic load barrier, and synchronized future start.
- Local battle remains disabled, the profile-switch UI is absent from Arena
  song select, and an external active-profile change still leaves the room.

Phase 2 is complete when two clients with different libraries can reach
Playing with byte-identical source charts, the same realized BMS control flow,
the same lane transformations, and unchanged personal gauge/presentation
settings.

## Scope

Included:

- Protocol 1.1 negotiation and the `rounds-v1` capability.
- Exact packed inventory uploads and room-relative common-set reset/delta
  transfers.
- Inventory, availability, selection, and round revisions.
- Queue-drained song-library generation and scan coalescing.
- `ArenaAvailabilityIndex` and select presentation adapters without fields on
  `ChartData`.
- Full option synchronization: P1 and P2 note order, Random/S-Random/Mirror
  and every existing algorithm, DP Off/Flip/LR2 Flip/Battle, lane seed, and
  realized `#RANDOM` choices.
- Ready, waiting joins, frozen roster, exact probe, deterministic loading,
  launch cancellation, and synchronized start.
- A generic held-start seam in `ChartRunner` so every skin starts on the
  Arena deadline instead of its own decide/gameplay timer.
- Normal song-select activation interception, local-battle policy, and
  profile-menu suppression.

Excluded except for narrow interfaces consumed by later phases:

- Gameplay telemetry, live leaderboard, opponent pacemaker, DNF, final
  results, EX ranking, lobby-win updates, and normal IR-upload wiring.
- Result and gameplay overlays, F2 placement, result-number adapters, and final
  visual polish.
- Courses, replay playback, autoplay, spectators, downloads, or alternate
  selection modes in Arena.
- Persistent inventories, room state, results, or chat.

Phase 2 intentionally leaves a room in Playing after the synchronized start;
Phase 3 owns completion/DNF/finalization and the transition back to Selecting.
The Phase 2 branch is therefore an integration milestone, not a separately
released multiplayer mode.

## Autonomous assumptions

1. Protocol major remains 1 and the current minor becomes 1. `rooms-v1`
   remains the base browsing capability; one new `rounds-v1` capability gates
   create/join and every Phase 2 command.
2. A 1.0 client can continue anonymous directory browsing when it advertises
   only `rooms-v1`, but it cannot create or join a playable room. A 1.1 client
   rejected by an older strict 1.0 server reconnects once in explicit legacy-
   browse mode with minor 0 and only `rooms-v1`; it may browse but presents an
   update-required state for create/join. The fallback is never used after an
   authentication or room mutation attempt and never loops.
3. IR Arena tickets advertise protocol minor 1. The verifier continues to
   accept any non-negative ticket minor; wire capability negotiation, not the
   ticket claim alone, decides feature availability.
4. One inventory contains at most 250,000 distinct SHA-256 values and exactly
   `count * 32` raw bytes, with a maximum of 8,000,000 bytes.
5. Every binary WebSocket frame remains at most 65,536 bytes. A fixed 32-byte
   header leaves room for at most 2,047 SHA-256 values per frame.
6. One connection can own one inbound inventory upload and one inbound common-
   availability transfer at a time. Starting a newer valid transfer discards
   the older incomplete transfer without changing committed state.
7. Inventory upload and exact-probe deadlines are 60 and 15 seconds;
   deterministic chart loading has a 60-second deadline.
8. A scheduled start has at least 2,000 ms lead. The server may extend it to
   at most 5,000 ms using recent heartbeat RTT estimates.
9. A one-player room may start. Phase 3 awards no lobby win unless the frozen
   roster has at least two participants.
10. Arena supports individual charts only. Course, replay, and autoplay entry
    points are disabled while the client occupies a room.
11. The initial server computes sorted intersections synchronously in a pure
    packed-inventory module. The module is isolated so a worker adapter can
    replace the implementation if measured production latency requires it.
12. Existing unrelated and in-progress Phase 1 files are user-owned. This
    design adds only the two Phase 2 documents; implementation follows after
    Phase 1 is committed and reviewed.

## Module shape

### Server

`RoomDirectory` remains the application-facing deep module and the only module
that creates or destroys rooms. It gains next-round inventory, selection,
ready, and frozen-round transitions. Complex mechanics stay behind it:

- `inventory/packed-inventory.ts` validates compact sorted vectors, computes
  intersections and deltas, and never learns about sockets or protocol DTOs.
- `inventory/inventory-upload-manager.ts` owns partial connection-bound
  uploads, byte budgets, chunk ordering, digests, and deadlines. Only a fully
  validated `PackedInventory` crosses into `RoomDirectory`.
- `rooms/round-state.ts` owns selection revisions, readiness, frozen rosters,
  probe/load barriers, cancellation, and scheduled starts.
- `ArenaApplication` maps protocol messages/binary transfers to domain calls
  and maps domain effects to text/binary deliveries.
- The WebSocket gateway adds bounded binary receive/send while retaining its
  serialized per-connection receive tail and redacted logging.

Partial uploads are not room state. A disconnect, socket replacement, fatal
protocol error, or upload deadline discards them. Committed inventories remain
seat state through reconnect grace.

### Client

`ArenaSession` remains the only QML-facing Arena module. It gains inventory,
selection, and round orchestration while hiding transfer IDs, binary headers,
revisions used only for consistency, local file paths, and loader callbacks.

Internal seams:

- `ArenaInventorySource`: production SQLite/library-generation adapter and a
  deterministic fake.
- `ArenaRoundLoader`: production adapter over the active profile,
  `ChartDataFactory`, `ChartFactory`, and song database, plus a fake.
- `ArenaBinaryProtocol`: strict internal codec for packed SHA-256 frames.
- `ArenaAvailabilityIndex`: compact sorted common set and atomic delta/reset
  application.
- Existing `ArenaTransport` carries binary frames and transport generations.
- Existing `ArenaScheduler` controls transfer timeouts and synchronized start.

QML receives read-only state, models, and commands. It does not parse JSON or
binary frames, create play configurations, hash files, or decide whether a
round may start.

## Protocol evolution and negotiation

Constants:

```text
protocolMajor = 1
protocolMinor = 1
base capability = rooms-v1
round capability = rounds-v1
```

`client_hello` accepts minor 0 or 1 for major 1 and 1-16 unique capability
names. `server_hello` returns the negotiated minor and the ordered capability
intersection. `rooms-v1` is required for a successful hello. A server with
`rounds-v1` returns it only when the client advertised it.

The Phase 2 client first sends minor 1 with `rooms-v1, rounds-v1`. If a peer
using the prior strict minor-0 codec rejects that initial hello with
`protocol_incompatible`, `ArenaSession` closes it and performs one anonymous reconnect using
minor 0 with only `rooms-v1`. A successful legacy hello sets browse-only/update-
required state and disables login, create, join, and resume on that connection.
Any other fatal error follows the normal Phase 1 recovery path.

Anonymous directory subscription works with `rooms-v1`. `room_create` and
`room_join` require an authenticated connection whose negotiated capabilities
contain `rounds-v1`; otherwise they return `rounds_capability_required` without
room mutation. Every seat in a Phase 2 room therefore supports the same
inventory, selection, and start contract.

The new protocol remains strict. Unknown fields, unsupported enum values,
invalid revisions, or Phase 2 messages on a connection without `rounds-v1`
are rejected. Sensitive failure objects retain no frame, ticket, password,
hash vector, file path, or chat body.

## Binary SHA-256 frame format

All integer fields are unsigned big-endian. The header is exactly 32 bytes:

| Offset | Size | Field |
|---:|---:|---|
| 0 | 4 | ASCII magic `RGA1` |
| 4 | 1 | binary format version `1` |
| 5 | 1 | kind: `1` inventory upload, `2` availability reset, `3` availability add, `4` availability remove |
| 6 | 2 | reserved, must be zero |
| 8 | 16 | raw transfer ID issued by the server |
| 24 | 4 | zero-based chunk index |
| 28 | 4 | hashes in this frame, `0..2047` |

The payload is exactly `hashesInFrame * 32` bytes with no padding. A nonempty
transfer uses nonempty chunks; an empty vector uses no binary chunks. A frame
must be no larger than 65,536 bytes.

Within each logical vector, hashes are strictly increasing and unique across
chunk boundaries. Chunks arrive exactly in declared index order. Transfer ID,
kind, counts, byte totals, and SHA-256 digest of the complete raw vector must
match the preceding JSON control message. Any mismatch discards the incomplete
transfer. The receiver never mutates committed inventory or availability from
an incomplete transfer.

Binary frames are valid only for a room-bound `rounds-v1` connection and a
currently expected transfer. Anonymous, authenticated-but-unseated, unknown,
late, duplicated, or oversized binary traffic is fatal `unexpected_binary` or
`malformed_inventory` traffic and closes the socket after a safe structured
error.

## Client inventory publication

### Control messages

The upload sequence is:

1. `inventory_upload_begin` command.
2. Targeted `inventory_upload_ready` event containing a server-generated
   128-bit `uploadId`.
3. Zero or more kind-1 binary chunks.
4. `inventory_upload_commit` command.
5. `inventory_committed` event followed by member/availability effects.

`inventory_upload_begin.data` contains:

```text
requestId
roomId, roomGeneration, connectionGeneration
libraryGeneration: positive JS-safe integer
hashCount: 0..250000
byteCount: exactly hashCount * 32, at most 8000000
chunkCount: exactly ceil(hashCount / 2047)
vectorDigest: 64 lowercase hex characters
```

`inventory_upload_ready.data` echoes the request and binding, contains the
22-character base64url `uploadId`, and reports a server upload deadline. The
raw 16-byte ID is used in each binary header.

`inventory_upload_commit` uses a new request ID and the same room binding,
upload ID, library generation, declared totals, and vector digest. Commit is
accepted only after all chunks have arrived and the complete packed vector has
passed count, size, ordering, uniqueness, and digest validation.

`inventory_upload_abort` lets a client discard a stale build immediately. A
new valid begin, disconnect, resume replacement, leave, kick, timeout, or
shutdown also discards the partial transfer.

### Revisions

Each seat has:

- `libraryGeneration`: the last client library generation committed for that
  seat; it must increase strictly for that seat lifetime.
- `inventoryRevision`: a server-assigned positive integer incremented only on
  a successful atomic replacement. Zero means no authoritative inventory.
- `inventoryState`: `missing`, `syncing`, or `ready`.

The previous committed inventory remains active during local scanning,
asynchronous snapshot construction, and partial upload. Starting an upload
sets the seat to `syncing`, clears next-round readiness, and prevents a launch;
it does not discard the previous packed vector. Commit atomically replaces the
vector and sets `ready`. Abort/failure returns to `ready` when a previous
inventory exists, otherwise `missing`.

Committed vectors are retained through the 60-second seat reservation and are
deleted on explicit leave, kick, grace expiry, or room destruction. They are
never written to disk or ordinary logs.

## Library generation and scan coalescing

The song-library layer gains one monotonic process-lifetime generation. It is
initialized to `1` after the song database is available, so every publishable
snapshot has the protocol-required positive generation, and increments after
every completed mutation that can change `charts`:

- the transition from a nonempty scan queue to fully idle after all queued
  roots finish or stop;
- synchronous root-folder removal after its database cleanup commits;
- any future direct chart-set mutation at its commit point.

`currentScannedFolder == ""` is not the generation signal because it is also
empty between queued roots. `ScanningQueue` emits a dedicated `queueDrained`
signal only after the final queued item has been removed and no next task was
started. Multiple roots in one queue batch therefore produce one generation.

The production inventory source opens its own SQLite read connection on a
worker and runs the semantic equivalent of:

```sql
SELECT DISTINCT lower(sha256)
FROM charts
WHERE length(sha256) = 64
ORDER BY lower(sha256) COLLATE BINARY
LIMIT 250001;
```

It validates lowercase hexadecimal SHA-256, decodes to raw bytes, removes no
valid distinct row beyond SQL's `DISTINCT`, and rejects the snapshot if the
sentinel 250,001st row exists. The source captures the generation before
querying and checks it again on main-thread completion. A changed generation
discards the result and starts one new build for the newest generation. No
intermediate scan-folder completion publishes an inventory.

The last committed server inventory remains in use while a new snapshot is
built. Joining or creating a room triggers an immediate build for the current
generation. While seated, each later generation coalesces into at most one
in-flight build and one newest pending rebuild.

## Common availability and revisions

### Eligible set

In Selecting, every seat, including a disconnected reserved seat inside grace,
is eligible for the next round. An authoritative common set exists only when
every eligible seat has a committed inventory. A missing or syncing inventory
puts availability in Syncing and prevents ready/start.

During Loading or Playing, the frozen roster is immutable. New or rejoining
non-frozen seats are waiting for the next round. Their inventories participate
in the next-round common set but never alter the frozen selection or play
configuration.

### Server representation

The server stores packed sorted vectors and computes the exact intersection.
The common-set basis is the sorted list of `(memberId, inventoryRevision)` for
all next-round eligible seats. A room `availabilityRevision` increments only
when a new authoritative basis/common vector commits. Revision zero means no
authoritative common set.

Adding an eligible member may intersect the previous common set with the new
inventory. Removing a member or replacing an inventory recomputes from all
eligible committed vectors because the set may expand. A pure two-pointer
packed-vector implementation owns comparison, intersection, and delta logic.

Every availability change clears all next-round ready states. In Selecting,
the current selection is retained only if its SHA-256 remains common;
otherwise selection is cleared and its revision increments. Loading/Playing
state remains frozen.

### Atomic client transfer

The server sends either a full reset or a delta. It sends a reset after join,
resume, resync, revision gap, or whenever a delta would contain at least as
many hashes as the new full set. A delta's total add/remove records may not
exceed 250,000; otherwise a reset is mandatory.

`availability_transfer_begin` contains:

```text
roomId, roomGeneration
transferId
mode: reset | delta
baseRevision: absent for reset, current client revision for delta
targetRevision
basis: 1..16 unique { memberId, inventoryRevision }
resetCount/resetChunkCount/resetDigest, or
addedCount/addedChunkCount/addedDigest and
removedCount/removedChunkCount/removedDigest
```

The server then sends kind-2 reset chunks or kind-4 remove chunks followed by
kind-3 add chunks, and finally `availability_transfer_commit` with the target
revision. A client buffers and validates the complete transfer, applies it to
`ArenaAvailabilityIndex` with one atomic reset/merge, and sends
`availability_applied` for that exact revision. Until the acknowledgement,
that seat cannot ready.

If the base revision is stale, chunks fail validation, a digest differs, or a
newer transfer supersedes the old one, the client discards all staged bytes,
keeps its prior committed index, enters Syncing, and sends one rate-limited
`availability_resync`. The server retains only the current full common set;
resync always receives a targeted reset rather than replay history.

### Client presentation index

`ArenaAvailabilityIndex` stores one compact sorted raw vector, not a `QSet` of
250,000 heap-allocated strings. Its public states are:

- `NotApplicable` outside a seated Arena session.
- `Syncing` while no authoritative revision exists or a transfer is pending.
- `AvailableToAll` when the index is ready and contains the SHA-256.
- `UnavailableToSome` when the index is ready and does not contain it.

It exposes a revision/state notification plus a hash lookup. It may retain the
member IDs from the latest selection rejection for C++ diagnostics/future UI,
but Phase 2 chart rows show only the compact common/unavailable result.

No availability or activation field is added to `ChartData`. Existing table
missing-entry state remains separate from Arena availability.

## Selection snapshot and synchronized options

### Selection command

Any connected eligible seat may send `selection_set` while Selecting. The
command carries the current room binding, `availabilityRevision`, the actor's
`inventoryRevision`, and an immutable `SelectionSnapshot`. Server processing
is serialized; the last accepted command wins.

The server accepts only when:

- the negotiated connection has `rounds-v1` and is bound to the room;
- the room is Selecting;
- availability is authoritative and the supplied revisions are current;
- the actor has acknowledged the current availability;
- the chart SHA-256 is in the exact common set; and
- the complete selection schema is valid.

A rejection leaves the previous selection and every ready state unchanged.
`selection_rejected` returns the request ID, stable reason, and, only for
`not_common`, the bounded unique member IDs whose committed inventory lacks the
hash. ArenaSession stores those IDs in C++ but Phase 2 UI uses a generic
translatable explanation.

An accepted selection increments `selectionRevision`, replaces the immutable
snapshot, records `selectedByMemberId`, clears every ready state, and broadcasts
`selection_changed`. Local option changes without another chart activation
send nothing.

### Exact wire value

```text
SelectionSnapshot
  sha256: 64 lowercase hex
  md5: absent or 32 lowercase hex
  title, subtitle, artist: each at most 200 Unicode code points
  keyMode: 5 | 7 | 10 | 14
  randomSequence: 0..4096 positive JS-safe integers
  noteOrderP1: one exact NoteOrder value
  noteOrderP2: one exact NoteOrder value
  dpMode: off | flip | lr2_flip | battle
  laneSeed: exactly 16 lowercase hex characters (unsigned 64-bit)
  randomizationVersion: 1
```

`NoteOrder` is exhaustive over the existing enum:

```text
normal
mirror
random
s_random
r_random
random_plus
s_random_plus
beatoraja_random
beatoraja_random_ex
lr2_random
lr2_random_ex
```

The seed is a string because JavaScript cannot represent every 64-bit value
exactly. The server validates and relays the value but does not interpret chart
notes. Unknown algorithms, versions, DP modes, key modes, or out-of-range
`#RANDOM` choices are rejected rather than coerced to Normal.

`ArenaSelectionBuilder` captures the currently displayed `ChartData` SHA-256,
metadata, and realized `randomSequence`, then reads only the active profile's
P1/P2 note-order and DP transformation. It generates one 64-bit lane seed.
Gauge, hi-speed, lane cover, lift/hidden, BGA, offsets, target, input mapping,
and presentation settings never enter the snapshot.

DP `battle` here is the synchronized SP-to-DP chart transformation. It is not
`ProfileList::battleActive` and does not enable a second local player.

## Ready and frozen launch

### Ready command

`ready_set` contains `ready`, the room binding, `selectionRevision`,
`availabilityRevision`, and the actor's `inventoryRevision`.

`ready=false` clears only the actor's ready state while Selecting. `ready=true`
requires:

- a connected eligible seat;
- no active local/server inventory upload for that seat;
- authoritative inventory and an acknowledged current availability revision;
- a non-null current selection common at that revision; and
- exact current selection/inventory/availability revisions.

Selection, eligible membership, committed inventory, or authoritative common-
set changes clear every next-round ready state. Disconnect during Selecting
clears that seat's ready state while retaining its committed inventory and
eligibility through grace.

When every eligible seat is connected and ready, the same domain transition
atomically:

1. Captures the selection and selection revision.
2. Captures availability revision and every frozen inventory revision.
3. Freezes the ordered participant roster.
4. Allocates a 128-bit `roundId` and 128-bit `launchAttemptId`.
5. Clears next-round ready flags and enters Loading.

No later selection, join, inventory commit, or ready command can mutate that
frozen launch. A join during Loading or Playing is marked waiting.

### Frozen round snapshot

Room snapshots and `round_loading_started` contain:

```text
roundId, launchAttemptId
selectionRevision, availabilityRevision
selection: complete immutable SelectionSnapshot
participants: ordered unique { memberId, inventoryRevision }
stage: probing | loading | scheduled | playing
```

Resume supplies the complete frozen round snapshot. A frozen participant
resumes its current stage; a waiting seat receives the snapshot for display
but cannot submit probe/load responses.

## Exact probe and deterministic loading

### Probe

The server assigns a different 128-bit nonce to every frozen participant and
sends `round_probe_requested` with the room binding, frozen IDs/revisions,
nonce, and SHA-256. The client resolves a local path by SHA-256, reads the
current file bytes, and computes SHA-256 without trusting the database row.

`round_probe_result` echoes every identifier plus `ok`. On success it echoes
the exact SHA-256. On failure it supplies one sanitized reason:
`missing_file`, `hash_mismatch`, `read_failed`, or `cancelled`.

Only the current connection binding for the frozen member may answer. A stale
nonce, revision, round, attempt, connection generation, waiting member, or
second conflicting answer is rejected. A false answer or 15-second timeout
cancels the launch. This is consistency for honest clients, not possession
proof against modified clients.

### Load

After all probes succeed, the server sends `round_load_requested` with the full
frozen snapshot. `ArenaRoundLoader` reparses the selected local file with the
frozen `randomSequence`, builds the runner with explicit P1/P2 note-order,
DP mode, 64-bit seed, and randomization version, and waits until
`ChartRunner::Ready` (including sounds/BGA readiness).

It does not write those values into profile variables. It derives gauges,
timing, hi-speed, lane cover, lift/hidden, BGA, and other personal behavior
from the active profile through the normal factories.

`round_load_result` reports `ok` only after the runner is Ready. Failure values
are `missing_file`, `hash_mismatch`, `parse_failed`, `unsupported_config`,
`resource_failed`, or `cancelled`; exception text and paths remain local and
redacted. A false answer or 60-second timeout cancels the launch.

The production and fake loader interface is asynchronous and request-
correlated. A profile change, room leave, launch cancellation, superseding
attempt, or session exit cancels and destroys the prepared runner.

### Cancellation policy

Every cancellation returns the room to Selecting and clears ready state.

- `missing_file`, `hash_mismatch`, `read_failed`, `parse_failed`,
  `unsupported_config`, or `resource_failed` clears the selection and
  increments `selectionRevision`.
- Probe/load timeout, explicit frozen-participant leave/kick, or server
  shutdown retains the selection only if it remains common for the current
  next-round availability revision.
- A waiting-member leave/kick does not cancel the frozen launch.

`round_launch_cancelled` carries the round/attempt, stable reason, and the
authoritative resulting selection/revisions. Clients cancel local timers,
destroy prepared runners, and return to normal Arena song select.

## Synchronized start

The server measures heartbeat RTT from nonce send/receive timestamps and keeps
the lowest of the latest eight valid samples per connection for 60 seconds.
When all frozen participants are loaded:

```text
maxRtt = maximum current participant sample, or 0 when unavailable
leadMs = clamp(2000 + maxRtt, 2000, 5000)
startAtServerMs = nowMs + leadMs
startAfterMs(connection) = max(250,
  startAtServerMs - sendNowMs - floor(connectionRtt / 2))
```

The server sends each participant a targeted `round_start_scheduled` containing
the common `startAtServerMs` and its connection-specific `startAfterMs`. The
client schedules from receipt using `ArenaScheduler`'s monotonic clock. The
server schedules an exact domain sweep at `startAtServerMs`, changes the room
to Playing, and broadcasts `round_started`/directory phase.

`ChartRunner` gains a generic held-start seam. The Arena loader holds start
before exposing the runner. Calls to `start()` from Default/LR2/Beatoraja
skins only latch the request while held. `ArenaSession` exposes the prepared
runner to the gameplay screen during the lead, calls `start()` to latch, and
releases the hold at the local scheduled time. Existing non-Arena runners are
never held and retain current behavior.

If cancellation arrives during the lead, the client cancels the scheduler,
pops any pre-start gameplay screen, destroys the runner, and returns to select.
Late schedule/start events are ignored by round and launch-attempt ID.

Phase 2 exposes a typed `roundRunnerStarted(roundId, ChartRunner*)` signal for
Phase 3 to attach telemetry and final-result handling. It adds no telemetry or
result message in this phase.

## Room and member presentation values

Room phase expands to `selecting | loading | playing` in directory summaries,
room snapshots, models, and QML.

Each member view adds:

```text
ready: boolean
inventoryState: missing | syncing | ready
inventoryRevision: nonnegative integer
availabilityAppliedRevision: nonnegative integer
roundState: eligible | waiting | probing | loading | loaded | playing
```

`status` remains `connected | reserved`; it is not overloaded with round
state. User ID, raw inventory, local path, transfer ID, nonce, and connection
generation remain absent from member roles.

The room snapshot adds current `selection`, `selectionRevision`,
`availabilityRevision`, and optional frozen round. `room_snapshot` is still
authoritative for text state; the common hash set follows as a targeted binary
reset before the local availability index becomes Ready.

## Normal song-select integration

After admission, ContentFrame opens the configured normal song-select screen
instead of the Phase 1 holding-room view. `ArenaSession` remains InRoom and
publishes a separate `roomPhase`; it never controls `StackView` directly.

The normal folder/table hierarchy is unchanged. Charts are not filtered.

- Default chart rows reuse the existing red unavailable presentation when
  `ArenaAvailabilityIndex` returns `UnavailableToSome`. Syncing uses a neutral
  disabled state. Existing table-missing red remains independent.
- LR2 prefixes a translatable `(arena unavailable) ` string to an otherwise
  local chart title. Existing `(missing)` remains for table entries absent
  locally.
- Beatoraja reuses its unavailable body/title rendering types. It does not
  receive LR2's text prefix.

Default delegates bind to the index revision/state and query by `sha256`.
`Lr2SelectItemModel` gains a transient Arena availability role/decorator keyed
by its existing SHA-256 role; no `ChartData` member is added.

Activating a local chart while availability is Ready and common calls
`ArenaSession::selectChart(ChartData*)`; it never opens the chart immediately.
Activating an unavailable/syncing chart produces local feedback and leaves the
current server selection untouched. The server performs the same common-set
check. Course, replay, autoplay, and ranking-play shortcuts are disabled in a
room.

The selecting player's current P1/P2 randomization and DP transformation are
captured only at activation. Changing option menus afterward remains local and
does not alter the room selection until a chart is activated again.

Phase 2 presentation is deliberately functional. The full player/chat/ready
overlay and polished Default layout remain Phase 4, but a minimal application-
owned strip must expose current selection, ready action/state, sync/loading
state, and launch errors above every select skin.

## Local battle and profile behavior

The Phase 1 generic `battleAllowed` policy remains active for the entire Arena
session. Entering Arena has already silently set local `battleActive=false`;
the select battle control is hidden/disabled and any attempted true assignment
is reset. Leaving Arena does not restore it.

`DpOptions::Battle` stays available in the single-profile play options and is
captured in the selection snapshot. It never creates a second local profile.

The Default select profile/login panel and any profile-card switch action are
not instantiated or enabled while a seat is active. LR2/Beatoraja have no
native lobby/profile UI; the application overlay offers no profile switch.
This is presentation, not a global setter prohibition. If another path changes
`ProfileList::mainProfile`, the identity adapter performs the established
best-effort leave, clears inventory/selection/load secrets and runner, and
returns to the Arena Browser.

## Error and recovery behavior

- Inventory build failure: retain the previous committed inventory, keep the
  member unready, and expose a retryable local error.
- Upload rejection/timeout: abort partial bytes, restore previous inventory
  state when available, and allow retry under rate limits.
- Common transfer gap/corruption: retain the previous local index, enter
  Syncing, and request one full reset.
- Non-common selection: keep the previous selection and ready states; expose a
  stable rejection without dropping the socket.
- Selection becomes non-common after an authoritative next-round change:
  clear it server-side and clear all ready states.
- Disconnect in Selecting: clear that seat's ready state, retain committed
  inventory, and include the reserved seat in common availability.
- Resume: replace text room state from the snapshot, discard partial transfers,
  request/push a full common reset, rebuild/upload only if the local generation
  is newer than the retained seat generation.
- Disconnect during Loading: preserve the frozen seat for grace but let the
  probe/load deadline cancel if it cannot respond.
- Explicit frozen-participant leave/kick during Loading: cancel the launch.
- Waiting join/leave/kick: alter only next-round state.
- Profile change/logout/exit: cancel builders, transfers, loader, start timer,
  and runner before clearing room state.
- Stale events: ignore or reject by transport generation, room generation,
  inventory revision, availability revision, selection revision, round ID,
  launch-attempt ID, transfer ID, nonce, and request ID as applicable.

## Security, privacy, and resource limits

- Official transport remains WSS; no hash, transfer ID, or token appears in a
  URL.
- Hash vectors are library fingerprints. Logs may contain counts, byte sizes,
  timings, revisions, room/seat IDs, and stable errors, never individual
  hashes, complete vectors, file paths, or digests associated with a user.
- JSON text remains capped at 65,536 client bytes and 4 MiB server bytes.
  Binary frames are independently capped at 65,536 bytes.
- Inventory count is `0..250000`; raw vector bytes are `0..8000000` and must
  equal count times 32. No allocation uses a declared count before all
  arithmetic and byte caps validate.
- One partial upload per connection; 60-second deadline; six accepted begin
  attempts per identity per minute. Availability resync is limited to twelve
  attempts per identity per minute.
- Default process budgets are 128 MiB of partial inventory bytes and 512 MiB
  of committed inventory bytes. A begin/commit that would exceed them returns
  `inventory_capacity_exceeded` and leaves the socket/room alive.
- Partial buffers are released on every terminal path. Committed vectors and
  common sets are released with their seat/room.
- Selection metadata is untrusted text with explicit code-point limits. UI
  renders it as plain text.
- The exact probe reads only the database-resolved chart path for the expected
  SHA-256; no remote path is accepted from protocol data.
- Random sequences and option enums are bounded before chart parsing. The
  server relays them but clients validate again before loading.
- Rate-limit, malformed-transfer, and loader errors are stable and redacted.
- Inventory/probe consistency is not anti-cheat; a modified authenticated
  client may lie.

New stable command error codes include:

```text
rounds_capability_required
inventory_busy
inventory_invalid
inventory_stale
inventory_capacity_exceeded
availability_stale
selection_not_common
selection_stale
ready_not_allowed
round_stale
launch_stage_stale
```

`unexpected_binary` and `malformed_inventory` are fatal protocol errors: the
server sends one redacted structured error when safe and then closes the
connection. All other codes above reject only the correlated command.

Display message keys are closed `arena.*` values and are added to the same
English/Polish translation catalog as Phase 1.

## Testing strategy

### Server unit and integration tests

- Protocol 1.0 anonymous browse and 1.1 `rounds-v1` negotiation in both
  directions; create/join gating.
- Binary header golden bytes, all kinds, exact 65,536-byte boundary, invalid
  magic/version/reserved fields/count/length/kind/transfer/order.
- Inventory zero/one/2,047/2,048/250,000 boundaries; strict ordering,
  duplicates across chunks, count/size/digest mismatch, abort, supersede,
  timeout, disconnect, and process budgets.
- Exact intersections for identical, disjoint, partial, add-member,
  remove-member, replacement, reserved seat, and waiting seat cases.
- Availability reset/delta choice, add/remove disjointness, revision gaps,
  acknowledgement gating, targeted resync, and no partial client-visible
  state.
- Inventory changes clear ready and retain/clear selection exactly as defined.
- Last-accepted selection, exhaustive option values, non-common rejection with
  missing IDs, and immutable snapshots.
- Ready prerequisites, all-ready atomic freeze, one-player start, waiting join,
  disconnected eligible seat, and stale revisions.
- Per-member probe nonce, false/stale/duplicate replies, exact 15-second
  boundary, load barrier, 60-second boundary, cancellation policy, explicit
  leave/kick, and resume.
- RTT sample retention, 2-5 second lead calculation, targeted delay, exact
  scheduled domain transition, and stale scheduled events.
- Real WebSocket text/binary interleaving, backpressure, Linux binary/oversize
  cases, shutdown cleanup, and absence of secrets/hashes in logs.
- Packed-inventory performance at 88,000 expected and 250,000 maximum hashes,
  including a 16-seat identical worst case, with timings reported rather than
  a platform-fragile hard threshold.

### Client tests

- Binary codec byte goldens and every malformed boundary without staged-state
  mutation.
- `ArenaAvailabilityIndex` reset/delta atomicity, stale base, sorted merge,
  compact lookup, state/revision notification, and 250,000-hash bound.
- Queue-drained generation across multiple roots, root removal, overlapping
  build generations, SQLite invalid-hash rejection, cap, and one upload per
  committed generation.
- Join/resume inventory publication, upload abort/retry, availability ack/
  resync, and transport-generation rejection.
- Default/LR2/Beatoraja common/unavailable/syncing presentation while table
  missing remains separate and no `ChartData` field exists.
- Local activation blocks non-common charts and preserves the previous server
  selection.
- Exhaustive enum serialization for all 11 note-order values and four DP
  values; 64-bit seed round trip; P1/P2 distinction; `#RANDOM` sequence.
- Two loads of one fixture produce identical parsed control flow, lane
  permutation, key mode, and random seed while profiles retain gauge,
  hi-speed, and presentation variables unchanged.
- Ready prerequisites, snapshot replacement, waiting joins, exact probe hash,
  load failure, cancellation, start timer, held `ChartRunner::start()`, and
  stale round/attempt handling with fakes.
- Local battle attempts stay off; DP Battle still serializes; Default profile
  switch UI is absent; external profile change leaves and returns Browser.
- Normal non-Arena select/decide/gameplay and local battle regression tests.

### Cross-repository integration

- The TypeScript and C++ JSON/binary golden corpora are byte-identical.
- Two scripted clients with partly overlapping inventories see the exact
  intersection, reject a non-common selection without replacing the current
  selection, accept every selector in last-accepted order, and freeze only
  after both acknowledge availability and ready.
- One client changes its library generation; one upload replaces the old
  revision, resets ready, and updates availability by delta or reset.
- Both clients rehash, load the same immutable config, report Ready, receive
  connection-specific scheduled delays, and start the same round.
- A third client joining during Loading is waiting and does not alter the
  frozen launch.
- Probe mismatch, load failure, timeout, frozen-member leave, and reconnect
  exercise each cancellation/recovery path.

## Later-phase interfaces, not implementations

Phase 2 preserves the following seams for Phase 3/4 without adding their
behavior:

- `ArenaSession::roundRunnerStarted(roundId, ChartRunner*)` supplies the
  frozen round and runner to the future telemetry/result adapter.
- The frozen round snapshot retains participant IDs, selection/options, and
  revisions required to validate telemetry and final results.
- Member `roundState` already includes `playing`; Phase 3 may extend it with
  finished/DNF without changing inventory or selection semantics.
- The minimal select strip consumes the same session values that the polished
  Default and LR2/Beatoraja overlays will use in Phase 4.
- No leaderboard/result model, pacemaker override, result rank, winner field,
  lobby-win mutation, or overlay-placement preference is introduced here.

## Exit criteria

- Protocol 1.1 negotiates `rounds-v1` without breaking anonymous 1.0 browsing.
- A seated client publishes no more than one authoritative packed inventory
  per completed library generation, including a true queue-drained scan.
- The server validates, bounds, stores, replaces, and deletes exact inventories
  without logging library fingerprints.
- Every client applies one authoritative common-set revision atomically and
  marks normal select rows without any Arena field on `ChartData`.
- Any connected eligible player can replace the selection; an unavailable
  chart cannot replace it.
- The selected snapshot covers realized `#RANDOM`, every P1/P2 note-order
  algorithm, DP Off/Flip/LR2 Flip/Battle, 64-bit seed, and algorithm version.
- Gauge, hi-speed, lane cover, BGA, and presentation settings remain personal
  and unchanged.
- All-ready atomically freezes current selection, roster, and inventory/
  availability revisions.
- Every frozen client rehashes the current file, loads an explicit immutable
  configuration, and reports readiness before start is scheduled.
- Failure or timeout cancels the launch without deadlocking the room; waiting
  joins never alter the frozen launch.
- The held-start seam prevents skin timers from starting early and releases
  each runner from a server-derived future deadline.
- Local battle remains silently disabled, DP Battle remains selectable, the
  profile-switch menu is absent in Arena select, and an external profile change
  returns to Browser.
- Focused Bun, Catch2, QML/manual, binary golden, and two-client integration
  tests pass with the documented Linux gateway checks.
