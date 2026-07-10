# Online Arena multiplayer design

## Status

Approved design recorded on 2026-07-10. This document is the umbrella design for the feature. Implementation is intentionally divided into four separately planned and reviewed phases.

## Context

RhythmGame currently supports local multiplayer: two local profiles can play one chart together. The requested feature is a public online Arena system inspired primarily by osu! multiplayer, while retaining the useful competitive behavior of LR2ArenaEx and Etterna.

The public service must host many independent lobbies. One server must not mean one lobby. It should be deployable through Docker and Coolify on the same Netcup host as RhythmGame IR, but remain operationally independent from IR. IR remains the source of player identity and the destination for normal score uploads.

The application supports Default, LR2, and Beatoraja-compatible skins. LR2 and Beatoraja skins do not define multiplayer screens, so application-owned screens and overlays are required.

## Goals

- Provide one official public Arena service containing many ephemeral lobbies.
- Let unauthenticated users browse lobbies, while requiring an IR login to create or join.
- Support public and password-protected lobbies, owner moderation, chat, reconnection, and temporary lobby win statistics.
- Let any player select any chart at any time; the last server-accepted selection wins.
- Make chart availability understandable even when every player has a different and potentially very large library.
- Synchronize every chart transformation that changes the note pattern.
- Preserve each player's personal gauge and presentation settings.
- Reuse the existing local score-save and IR-upload path.
- Provide live standings, an Arena opponent pacemaker, results, and winner information in every supported skin family.
- Keep transient room, availability, and synchronized play state out of ChartData, gameplay profile settings, IR ranking models, and persistent server storage. Deliberate Arena UI preferences such as overlay placement may be stored per profile.
- Keep the protocol independent of Qt internals so alternative servers remain possible later.

## Non-goals for the initial release

- Guest participation in a room.
- Multiple local players or local battle mode inside one online Arena client.
- Spectators, matchmaking, ratings, tournaments, or persistent room history.
- Song downloading or distribution.
- Turn-based, host-only, vote, or playlist selection modes.
- Persistent chat or persistent Arena win statistics.
- Voice chat.
- Server-side anti-cheat or proof that an authenticated client reported an honest score.
- LR2ArenaEx item or ojama mode.
- Horizontal scaling across multiple Arena replicas.
- A polished custom-server management UI. The endpoint remains configurable, but the official server is the initial product surface.

## Alternatives considered

### Qt Remote Objects

Qt Remote Objects would provide convenient Qt replica objects, but it would expose Qt-specific object semantics as the public internet protocol. Authentication, protocol evolution, non-Qt server implementation, message limits, and compatibility handling would still need custom work. It would also make future alternative servers unnecessarily dependent on Qt.

### GameNetworkingSockets

Valve's GameNetworkingSockets is well suited to latency-sensitive peer or game-state traffic, including UDP transport, reliability choices, connection routing, and NAT-related concerns. Arena does not simulate gameplay on the server. Its traffic is lobby state, inventories, chat, readiness, throttled score telemetry, and final results. That additional transport machinery would not provide useful leverage for the initial feature.

### WebSocket Secure

WSS is the selected transport. Qt has a WebSocket client, Bun has a production WebSocket server, reverse proxies understand WSS, and the protocol can carry both JSON control messages and binary inventory chunks. Gameplay remains local and deterministic; the network coordinates the round rather than simulating it.

## System architecture

### IR identity issuer

RhythmGame IR remains the identity authority and normal score service. It issues short-lived, audience-restricted Arena connection tickets to already authenticated profiles. It does not host room state and does not receive Arena telemetry.

### Arena server

The Arena server is an independently deployable Bun and TypeScript application. One process owns the lobby directory and all in-memory room state. It authenticates connection tickets through IR's public keys, validates commands, computes common chart sets, coordinates rounds, relays chat and telemetry, and computes ephemeral standings.

### ArenaSession client module

ArenaSession is the QML-facing deep module in RhythmGame. It owns authentication, connection and reconnection, protocol validation, room state, chart inventory, selection state, transient play configuration, live standings, and the last result.

Its external interface consists of a small set of commands plus read-only properties and models:

- Browse, create, join, leave, kick, send chat, select chart, and set ready.
- Connection, authentication, room, selection, and round state.
- Room directory, members, chat, live leaderboard, and last-result models.
- Room-relative chart availability.

QML must not parse protocol messages or implement authoritative room transitions.

ArenaSession does not manipulate the QML StackView. ContentFrame observes its public state and owns navigation, while a global overlay layer above the scene stack provides skin-independent Arena chrome.

ArenaSession has internal seams for:

- ArenaTransport, with a QWebSocket production adapter and a fake test adapter.
- ArenaIdentityProvider, with a production adapter over the active Profile and a fake test adapter.
- An injected or controllable scheduler/clock for heartbeat, reconnection, timeouts, and telemetry throttling.
- An inventory source over the chart database.

The production identity adapter calls a narrow authenticated ticket operation on the active Profile. Neither QML nor the transport receives access to the stored IR bearer token.

### Presentation adapters

The Default theme receives native Arena layouts. LR2 and Beatoraja use application-owned overlays and value-resolution adapters. Every presentation consumes the same ArenaSession models; no theme implements networking.

## Identity and authentication

### Existing identity path

RhythmGame already signs in through Better Auth and stores the resulting bearer session token in the operating-system keychain per local profile. That bearer token remains private to communication between RhythmGame and IR and must never be given to Arena.

### Arena ticket

IR enables its installed Better Auth JWT plugin and adds the required JWKS table and migration to its explicit Drizzle auth schema. The plugin uses disableSettingJwtHeader so this purpose-specific JWT is returned only by the explicit ticket request rather than being attached to ordinary session responses.

An authenticated profile requests a ticket from the Better Auth token endpoint. The ticket has these properties:

- Ed25519 signature.
- Approximately 90-second lifetime.
- Issuer fixed to RhythmGame IR.
- Audience fixed to the exact official Arena server identifier, not a generic value reusable by arbitrary servers.
- Purpose fixed to Arena connection.
- Subject containing the opaque IR user ID.
- Display name, avatar reference, verified-email state, protocol version, and random one-time token ID.
- A no-store response policy.

The client sends the ticket as the first application message after establishing WSS. It must not appear in the URL, query string, ordinary logs, or crash diagnostics.

Arena verifies the signature through IR's public JWKS and pins the algorithm, issuer, exact server audience, purpose, expiry, and required claims. Public keys are cached with rotation support. The token ID is accepted only once until its expiry. Binding the audience to the official server prevents a ticket disclosed to a different server from being replayed against the official service.

An authenticated Arena socket remains valid after its short connection ticket expires. A new or re-established authenticated connection obtains a new ticket. Client logout proactively closes Arena even though an already-established socket cannot be revoked solely by ticket expiry.

On joining a room, Arena also issues a high-entropy seat-resume token bound to the verified IR subject, room, seat, and connection generation. It is retained only in client memory and server room state. Reconnection requires both a fresh IR ticket and this resume token. A valid resume atomically replaces any stale socket for the seat, increments the connection generation so late messages from the replaced socket are ignored, and rotates the resume token. The token is deleted on explicit leave, kick, grace expiry, or room destruction.

### Login experience

Lobby browsing is anonymous. Creating or joining a room requires the active profile to be logged into IR. If it is not, the Arena Browser presents the existing email and password login flow inline; the player does not have to return to Settings.

Identity values displayed in a room come from the verified ticket, not from arbitrary client fields. One IR identity can occupy at most one seat in a room. A second ordinary join for an already seated identity is rejected; only the authenticated resume flow can replace its socket.

Authentication discourages anonymous bots and gives moderation a stable identity. It is not an anti-cheat system: a modified authenticated client can still lie about inventory, telemetry, or final score.

## Protocol

### Encoding

- JSON text frames carry version negotiation, authentication, directory, room, chat, selection, readiness, lifecycle, telemetry, and result messages.
- Binary frames carry sorted chart inventories in bounded chunks.
- A canonical protocol schema and golden fixtures define the wire format independently of the C++ and TypeScript implementations.

### Versioning and ordering

The handshake includes a protocol major and minor version plus supported capabilities, including the authentication scheme. An incompatible major version is rejected with a user-facing update message. Minor additions remain backward compatible when the required capabilities overlap.

State-changing messages carry the relevant identifiers:

- Room generation.
- Membership or inventory revision.
- Selection revision.
- Round ID.
- Per-sender sequence number where updates can be repeated.

The server rejects stale or out-of-phase commands. Clients ignore stale events and replace local state with a full authoritative snapshot after authentication, joining, or reconnection.

### Limits and validation

Every message has a strict schema and byte limit. The server rate-limits connection attempts, ticket use, room creation, password attempts, chat, selection changes, readiness, telemetry, and inventory revisions. Inventory allocation is based on validated received bytes rather than a client-declared count alone.

Protocol errors are structured. A valid but rejected action normally leaves the connection intact. Malformed traffic, repeated rate-limit violations, or authentication failure may close it.

## Lobby directory and room creation

The Arena Browser is entered from the main menu. It shows:

- Room name.
- Connected and reserved player count.
- A fixed initial maximum of 16 players.
- Selecting, loading, or playing state.
- Password indicator.

Password rooms remain visible in the public list. Unlisted/private room discovery is outside the initial scope.

Creating a room requires a non-empty bounded name and an optional password. Passwords are sent only through WSS and stored as a slow hash for the room lifetime. The initial release has one selection mode and no creator-selectable ruleset or capacity.

The server retains only a bounded recent chat backlog in memory for snapshots and reconnects. Chat is plain text, length-limited, rate-limited, and escaped by every presentation.

## Ownership and membership

- The creator is the initial owner.
- The owner can kick any other member.
- A kicked IR identity cannot rejoin that room for the rest of its lifetime.
- Explicit owner departure transfers ownership to the longest-present connected member.
- Owner disconnection also transfers ownership immediately so moderation cannot be stalled; reconnecting does not reclaim it automatically.
- A disconnected member's seat and lobby wins are reserved for 60 seconds.
- A disconnect during Selecting clears that seat's ready state. The reserved seat remains eligible, so the room cannot start until it reconnects and readies, is kicked, or its grace expires.
- A successful reconnect within the grace period restores the same member state and receives a full room snapshot.
- Grace expiry removes the member and invalidates room readiness and availability as needed.
- When no connected member remains, the room survives only until the final reserved seat's grace period expires, then it is destroyed.
- Explicitly leaving removes the seat immediately. Rejoining later creates a new member state and does not restore lobby wins.

New players may join while a round is loading or playing. They enter a waiting state, can see room state and chat, and are excluded from the frozen current-round roster. They become eligible when the room returns to selection.

An explicit leave or kick by a frozen participant during Loading cancels that launch and returns the room to Selecting. During Playing it immediately records that participant as DNF and lets the round continue. Leaving or kicking a waiting member does not affect the frozen round. A former member who joins again after explicit leave or grace expiry receives a new seat; if a round is active, that seat is waiting rather than part of the frozen roster. A kicked member cannot rejoin that room.

## Authoritative room state

The server room phase is:

Selecting -> Loading -> Playing -> Selecting

Result is a client screen, not a server phase. When a round is finalized, the room immediately returns to Selecting with every ready state cleared while retaining lastRoundResult and lobby win totals. A player still viewing results is not ready, so a later round cannot start without that player returning and readying or leaving.

### Selecting

- Any eligible player may submit a selection.
- The last server-accepted selection wins.
- A selection or eligible next-round membership/inventory change clears all next-round ready states.
- A player can ready only after inventory synchronization is authoritative and a valid common chart is selected.
- When every eligible member is connected and ready, the server atomically freezes the selection and roster, creates the round ID, and enters Loading. No later selection can race the frozen pre-load check.

### Loading

Loading begins with the final exact availability probe. After every probe succeeds, the server commands each frozen participant to load the same immutable play configuration.

Every participant reports loaded or a structured failure. When all report loaded, the server broadcasts a short future start deadline based on its connection clock estimates. Arena does not depend on frame-perfect starts, but the common deadline makes live progress comparison meaningful.

A missing file, probe failure, load failure, or loading timeout cancels the launch, clears readiness, invalidates the unusable selection, and returns the room to Selecting with an explicit reason.

### Playing

Gameplay runs locally. A disconnect or local abort does not cancel other players. The seat may reconnect within the grace period; otherwise it becomes DNF. A bounded play deadline prevents a client that never finishes from holding the round forever.

The server finalizes when every frozen participant has submitted a final result, become DNF, or reached the deadline.

## Interaction with local multiplayer and profiles

One Arena client represents exactly one active local profile.

Entering Arena silently sets ProfileList battleActive to false. While Arena is active:

- Local two-player battle cannot be enabled.
- The battle control is hidden or disabled.
- Any later attempt to set battleActive true is defensively reset.
- The profile-switch menu is unavailable in Arena song select.

Leaving Arena does not restore local battle automatically; it remains off.

If the active profile changes through any other path, Arena aborts local Arena activity, leaves the room, and returns to the Arena Browser. A change while merely browsing updates the identity/login state without a room leave.

The synchronized DP Battle chart transformation remains available. It is a play-option transformation and is distinct from local two-player battleActive.

## Chart inventory and common availability

### Exact inventory

The client publishes one canonical library inventory when creating or joining a room, then republishes it for every completed library revision while it holds a room seat. Anonymous browsing and an authenticated connection that has not entered a room do not disclose the library.

- SHA-256 is the authoritative chart identity.
- Hashes are sorted, strictly increasing, unique, and packed as raw 32-byte values.
- The upload is chunked and committed only after the declared size and count validate.
- The initial hard cap is 250,000 charts, or 8 MB of raw hashes.

At approximately 88,000 charts the payload is about 2.8 MB. This is acceptable for room entry and provides exact common-set browsing. Generic compression is not required initially because sorted cryptographic hashes compress poorly.

The server retains each compact binary inventory as room-seat state through the reconnect grace period and does not log it. It deletes the inventory on explicit leave, kick, grace expiry, or room destruction. It computes an exact intersection for every seat eligible for the next round, including a disconnected seat still inside its reconnect grace period. Adding a member can intersect incrementally; removing a member requires recomputing because the common set can expand. A frozen round continues using its frozen selection and roster even if the next-round common set changes.

Every common-set result is tied to the room generation and every member's inventory revision.

### Song-library changes

The library layer must expose one monotonic library generation for every completed mutation that can change the chart set. This includes a drained scan batch, root-folder removal, and any other direct chart-database mutation. The scanner specifically needs a real queue-drained or idle transition: currentScannedFolder becoming empty is insufficient because it also happens between queued root folders.

The last completed inventory remains active while scanning so a long scan does not freeze the lobby. When the entire queue becomes idle, or another mutation commits:

1. The client queries the completed chart database for distinct SHA-256 values.
2. It captures the library generation and builds the inventory asynchronously.
3. If the generation changes before the build completes, it discards that result and rebuilds from the newer generation.
4. It creates and uploads one new inventory revision.
5. The server recomputes the common set.
6. All next-round ready states are cleared.
7. In Selecting, the current selection is retained only if it remains common; otherwise it is cleared with a room message. A Loading or Playing round remains frozen and unaffected.

Multiple queued root scans are coalesced into one completed inventory upload.

### Selection presentation

Arena availability is transient room state and must not be added to ChartData.

A separate C++ ArenaAvailabilityIndex is keyed by SHA-256 and exposes:

- Not applicable outside an active room.
- Syncing.
- Available to all.
- Unavailable to one or more members.

It may retain the missing member identities for backend inspection or future UI, but the initial compact chart rows need not display them.

Selection-model adapters expose an Arena availability role to themes. Existing BMSTable missing-entry behavior remains separate:

- Default uses the existing unavailable red presentation.
- Beatoraja uses its unavailable body/title types.
- LR2 prefixes an explanatory Arena-unavailable string.

The normal folder/table hierarchy remains visible; charts are not filtered away. Activating a chart that is not common must not replace the current room selection. The client blocks it for immediate feedback and the server enforces the same rule.

### Final exact probe

The exact common inventory is browsing state, not the final loading authority. All-ready first freezes the selection, roster, selection revision, and member inventory revisions, creates the round ID and launch-attempt ID, and enters Loading. The first Loading step sends every frozen participant a nonce-bound probe for the selected SHA-256. The client rehashes the current file content, or uses an immutable loader reference already verified to that content, and replies with the frozen round, selection, inventory revision, launch attempt, nonce, and SHA-256. A false reply, identity mismatch, revision mismatch, or timeout cancels launch.

This guarantees consistency among honest clients. It does not prove possession against a deliberately modified client.

## Selection and synchronized play configuration

The selecting player's current chart-transforming options become an immutable selection snapshot. They are broadcast only when the chart is selected; option-menu changes alone are local.

The snapshot contains:

- SHA-256 plus display metadata and optional compatibility hashes.
- Realized BMS #RANDOM control-flow choices.
- Player 1 note-order algorithm and its parameters.
- Player 2 note-order algorithm and its parameters.
- DP mode: Off, Flip, LR2 Flip, or Battle.
- Lane-random seed.
- Randomization algorithm/version identifier.

All supported note-order values, including Random, S-Random, Mirror, and related algorithms, travel through the same typed snapshot rather than a partial hand-maintained subset.

Every client loads through an explicit immutable ArenaPlayConfig. ChartLoader must not temporarily mutate profile variables. This also makes deterministic two-client tests possible.

Changing local note-randomization or DP options after selection has no effect on the current room selection. The player must select a chart again to publish a new snapshot.

Gauge, hi-speed, lane cover, lift/hidden, BGA, and comparable play-comfort or presentation settings remain personal. Gauge and clear state are reported for display but do not determine the winner.

## Live gameplay

### Telemetry

Clients send coalesced live telemetry at approximately five updates per second, not once per judgement. Each update contains:

- Round ID and monotonically increasing sequence.
- Current EX score.
- Chart progress.
- Max combo and bad/poor count.
- Judgement counts.
- Gauge and play status.

The server retains only the newest update per participant and broadcasts current room standings. A final result supersedes all live telemetry; late live packets cannot overwrite it. Network telemetry never affects local judgement or score calculation.

### Leaderboard and detail view

The compact live leaderboard shows:

- Competition rank.
- Player name.
- EX score.
- Progress.
- Connected, loading, playing, finished, or DNF status.

It can expand on demand to show LR2ArenaEx-parity details: BP, max combo, judgement counts, gauge, and synchronized random/DP options.

### Forced Arena opponent pacemaker

Arena always replaces the effective gameplay score target with the strongest opponent's latest score. This is transient and does not write the player's saved score-target setting.

The target is the highest current EX score among other round participants. When that opponent changes, the effective pacemaker changes. Finished scores remain valid targets. If no opponent data exists, the Arena target reports that no opponent target is currently available rather than falling back to and mutating the saved target.

Leaving Arena automatically exposes the unchanged normal profile target again.

Default gameplay uses the dynamic Arena target in its target/ghost presentation. LR2 and Beatoraja value resolution receives the same transient target so compatible skin pacemaker elements continue to work.

### Gameplay chat

Chat is hidden during gameplay by default. A deliberate Arena chat action opens it without changing room state. Sending is optional while actively playing; opening chat must not implicitly pause gameplay.

## Results, winners, and IR scores

### Normal score path

ChartRunner finishes through the existing local result pipeline. The score is saved locally and uploaded to IR normally using the active profile's existing authenticated path. Arena must not introduce a branch that bypasses, delays, or proxies this upload.

Arena final-result submission happens after the local finish result exists and is independent of the IR upload outcome. An IR upload failure does not invalidate an Arena round, and an Arena outage does not prevent later IR score synchronization.

### Arena standings

The Arena server ranks valid final results solely by EX score:

- Higher EX score wins, regardless of gauge, clear type, combo, or BP.
- Exact EX ties are joint positions.
- Competition ranking is used: 1, 1, 3.
- Every joint first-place player is a winner.
- DNF is listed below valid results without a numeric competitive rank.

Gauge, clear, combo, BP, judgements, and options are result detail only.

The result participant count is the frozen round-roster size, including DNF players. A DNF local player has no Arena rank: Default renders a dash, while the numeric LR2/Beatoraja rank resolver returns 0.

Lobby wins are temporary. Every joint winner receives one lobby win when the frozen round contained at least two participants. The values survive a reconnect within the grace period but not an explicit leave, kick, or room destruction.

The server retains the latest result and winner summary until the next round finishes. No lobby or result history is persisted after room destruction.

## Screen and skin integration

### Navigation

The flow is:

Main menu -> Arena Browser -> Arena song select -> Gameplay -> Result

The Arena Browser is application-owned and works regardless of the selected gameplay skin. It contains room browsing, creation, password entry, inline login, and connection/error states.

After joining, RhythmGame reuses its normal song-select screen in Arena mode rather than duplicating library navigation.

### Default song select

The Default theme integrates:

- Current players and statuses.
- Owner marker and kick action.
- Ready state.
- Lobby wins and last winner.
- Current room selection and shared option summary.
- Chart availability.
- Chat.

### LR2 and Beatoraja song select

An application-owned overlay presents the same room information above the skin. The existing selection adapters provide Arena-unavailable chart styling. The overlay owns moderation, readiness, chat, and shared option presentation because legacy skins have no native concepts for them.

### Gameplay overlay customization

The live leaderboard is application-owned and rendered above every gameplay skin.

In the Default theme it participates in the existing F2 customizeMode and uses the existing drag/resize interaction style. In LR2 and Beatoraja gameplay, the wrapper provides an equivalent F2 mode with:

- Visible customization frame.
- Mouse drag and resize.
- Keyboard nudging.
- Reset to default.
- Suppression of gameplay input while an overlay customization interaction is active.

The normalized overlay rectangle is persisted by:

- Active profile.
- Resolved gameplay skin.
- Relevant layout variant, including SP/DP or key mode where the skin layout differs.

It is not persisted per room. It is clamped after viewport, resolution, or aspect-ratio changes. The first use defaults to the top-right safe area and shows a brief F2 positioning hint. An opened gameplay chat panel attaches beside the configured leaderboard and flips direction when necessary to remain on screen.

### Default result

Arena becomes a transient source in the existing RankingPosition widget alongside RhythmGame, LR2IR, and Tachi.

- Every Arena result opens with Arena selected.
- The player can cycle to normal internet-ranking providers while their queries/upload proceed normally.
- Arena is never written to generalVars.rankingProvider.
- Cycling during an Arena result does not replace the saved preferred internet provider.
- The next Arena result starts on Arena again.
- Arena position is immediately available and has no external ranking link.

Default result uses a transient effectiveResultSource separate from the saved generalVars.rankingProvider. Cycling updates only that transient source for the current result. Arena also has its own display mode: it shows current round position and participant count without the old/new-position delta arrow expected by internet-ranking results.

The result view also shows the complete Arena standings, winner, and lobby wins.

### LR2 and Beatoraja result

During an Arena result, the existing result-number meanings are adapted:

- Number 179 resolves to the local player's Arena position.
- Number 180 resolves to the Arena participant count.

Outside Arena they retain their existing internet-ranking behavior. Arena does not fabricate entries in OnlineRankingModel.

The universal result overlay shows winner and standings even when a skin does not render those number fields.

## Error and recovery behavior

- Arena unavailable: show a retryable Browser error; IR and offline play remain unaffected.
- IR unavailable: anonymous browsing and already-authenticated sockets continue; new authenticated joins/creates cannot obtain tickets.
- Server restart or deployment: all ephemeral rooms are lost. The server sends a going-away message when graceful shutdown is possible; clients return to Browser with an explanation.
- Socket loss: automatically reconnect with bounded exponential backoff during the 60-second grace period, then return to Browser if recovery fails.
- Wrong password: remain in Browser and expose no information beyond rejection.
- Inventory upload failure: the member remains unready and can retry; it cannot start a round with a non-authoritative inventory.
- Eligible next-round membership or inventory change: clear next-round readiness and recompute next-round availability without mutating a frozen Loading or Playing round.
- Selected file disappears: the final probe cancels loading and clears the invalid selection.
- Load failure or timeout: cancel only the launch, not the room.
- Disconnect or abort during play: preserve the rest of the round and mark DNF after recovery/deadline rules.
- Late/stale messages: ignore or reject by generation, revision, round, and sequence.
- Profile change: leave and return to Arena Browser.
- Attempted local battle activation: silently force it off.

## Privacy and security

- WSS is mandatory for the official service.
- Inventory hashes are sensitive library fingerprints. They are retained only for the connection/room lifetime, never placed in ordinary logs, and removed after disconnect expiry.
- Passwords, IR tickets, seat-resume tokens, chat bodies, and full inventories are excluded from structured operational logs.
- Room names and chat are untrusted plain text and are escaped in every UI.
- Public room-list, authentication, password, chat, telemetry, selection, and inventory endpoints have independent limits.
- Binary inventory count, ordering, uniqueness, size, chunk sequence, and deadline are validated.
- The server never allocates from an untrusted declared size without a matching byte cap.
- One official server replica is used while state is process-local.
- The protocol provides consistency and moderation, not score attestation. IR continues applying its own score validation independently.

## Deployment

Arena is a separate deployable Bun container behind Coolify at a host such as arena.rhythmgame.eu.

Required properties:

- Pinned Bun base-image version.
- Multi-stage Docker build.
- Non-root runtime user where supported.
- .dockerignore excluding environment files, VCS data, dependencies, tests artifacts, and local caches.
- WSS upgrade forwarding and proxy idle settings suitable for long-lived connections.
- /healthz liveness/readiness endpoint.
- CPU, memory, connection, and payload limits.
- Structured redacted logs and basic metrics for connections, rooms, errors, round state, and inventory timings.
- Graceful shutdown message before the process exits.

Arena receives only public JWKS and issuer/audience configuration plus its own operational secrets. It receives no IR database credentials or Better Auth signing secret.

The official endpoint is configuration rather than a protocol constant, but the initial UI and IR ticket audience permit only the official service. A future custom server may implement the same room protocol, but its authentication scheme requires a separate design: for example, an IR-registered audience or the server's own identity issuer. The client must never send an official-server ticket to an arbitrary endpoint.

Horizontal scaling is deferred. Running multiple independent replicas would split the lobby directory and room ownership. A later scale-out design requires explicit lobby routing and shared coordination such as Redis/pub-sub or another durable ownership mechanism.

## Testing strategy

### Server tests

- Pure room-state reducer tests with fake clocks and deterministic IDs.
- Ownership, kick ban, reconnect, waiting join, and destruction invariants.
- Last-accepted selection and readiness-reset behavior.
- Exact inventory validation and intersection across joins, leaves, and revisions.
- Scan-triggered revision replacement.
- Availability probes, loading cancellation, scheduled start, DNF, and finalization.
- Live telemetry coalescing and final-result precedence.
- Competition ranking and joint wins.
- JWT validation for signature, issuer, audience, purpose, expiry, token ID reuse, and JWKS rotation.
- Protocol schemas, size limits, malformed frames, rate limits, and version incompatibility.
- Large-inventory performance and memory tests at and beyond the expected 88,000-chart scale.

### Client tests

Use the existing Catch2 and CTest infrastructure with fake ArenaTransport, ArenaIdentityProvider, inventory source, and controllable time.

Cover:

- Anonymous browse and authenticated join/create gating.
- Ticket request, handshake, join, full snapshot, disconnect, and recovery.
- Password rejection, kick, owner transfer, and room destruction.
- Stale revision and round messages.
- Unavailable selection preserving the previous server selection.
- Membership/inventory changes resetting readiness.
- Queue-drained scan coalescing into one upload.
- Profile changes and logout leaving the room.
- Local battle attempts being reset.
- Exact ArenaPlayConfig serialization and deterministic chart transformation.
- P1/P2 note order, #RANDOM decisions, lane seed, DP Flip/LR2 Flip/Battle.
- Personal gauge/hi-speed settings remaining untouched.
- Telemetry throttling, forced opponent pacemaker, final precedence, and DNF.
- 1, 1, 3 ranking.
- Default transient ranking provider behavior.
- LR2/Beatoraja result numbers 179 and 180.

Qt WebSockets must become an explicit CMake component and linked/deployed dependency. Real-network tests remain a separate integration layer.

### Integration and presentation verification

- Start the Docker server and drive multiple scripted clients through create, join, password, select, ready, load, play, result, disconnect, and reconnect.
- Verify IR ticket issuance and JWKS validation against a test issuer.
- Verify existing local score save and IR upload still execute.
- Inspect Default, LR2, and Beatoraja song select, gameplay, and result paths.
- Check common/unavailable charts, inline login, owner controls, chat, last winner, and lobby wins.
- Check leaderboard compact/expanded modes and forced pacemaker.
- Check F2 move/resize/reset across representative SP and DP skins and multiple aspect ratios.
- Confirm profile controls and local battle controls are absent or ineffective in Arena.
- Verify server/container health, graceful shutdown behavior, and Coolify WSS proxying.

## LR2ArenaEx parity and intentional differences

The initial Arena includes or exceeds LR2ArenaEx's ordinary competitive behavior:

- Connection and authenticated identity.
- Host/owner moderation.
- Chart identity and common availability.
- Readiness and load barrier.
- Shared random seed and stronger complete option synchronization.
- Live score, progress, judgements, BP, combo, gauge, and option display.
- Strongest-opponent pacemaker.
- Chat.
- Movable gameplay presentation.
- Disconnect recovery.
- Explicit winner, result rank, and win statistics.

Intentional differences:

- One official server hosts many rooms instead of one server equaling one lobby.
- Any player may select rather than host-only selection.
- Missing charts are prevented before start instead of causing an opaque loading wait.
- Owner auto-rotation after every song is omitted because ownership is moderation, not song-picking authority.
- Item/ojama mode is deferred as a separate game mode.

## Delivery phases

This umbrella design is too large for one implementation change. Each phase receives its own implementation plan and review checkpoint.

### Phase 1: foundation, identity, and lobbies

- Canonical protocol and fixtures.
- IR JWT/JWKS migration and ticket issuance.
- Bun server shell, authentication, room directory, room lifecycle, passwords, ownership, kick, chat, reconnect, health, and Docker development setup.
- Qt WebSockets dependency, ArenaSession skeleton, transport/identity seams, Browser navigation, anonymous browsing, and inline login.

Exit criterion: two authenticated clients can reliably create, discover, join, chat, disconnect/reconnect, transfer ownership, and leave a password room without song-selection or gameplay behavior.

### Phase 2: inventory, selection, and synchronized start

- Exact binary inventories and intersection.
- Queue-drained scan revision.
- ArenaAvailabilityIndex and theme selection adapters.
- Room selection, immutable ArenaPlayConfig, readiness, final availability probe, loading, start deadline, and waiting joins.
- Local-battle lock and profile-change behavior.

Exit criterion: clients with different libraries can select only a common chart and deterministically load the same transformed note data without mutating profiles.

### Phase 3: gameplay and results

- Live telemetry and compact/expanded leaderboard models.
- Forced strongest-opponent pacemaker.
- Disconnect/DNF/finalization rules.
- EX-score standings, joint winners, lobby wins, and last-result snapshot.
- Existing local save and IR upload integration.
- Default and legacy result ranking adapters.

Exit criterion: a complete multi-client round produces live comparison, identical play configuration, normal IR uploads, deterministic Arena standings, and recoverable disconnect behavior.

### Phase 4: skin UX and production hardening

- Default integrated Arena layouts.
- LR2/Beatoraja song-select and result overlays.
- Global gameplay overlay and F2 per-profile/per-skin/per-layout customization.
- Chat presentation and accessibility/localization polish.
- End-to-end, load, malformed-input, privacy, and representative-skin verification.
- Production Coolify configuration, metrics, rate limits, operational documentation, and graceful deployment behavior.

Exit criterion: the feature is usable with representative Default, LR2, and Beatoraja configurations and can be safely operated as the official single-replica public service.

## Acceptance criteria

- The official service shows multiple concurrent rooms and never treats the entire server as one lobby.
- An unauthenticated player can browse but cannot create or join.
- A logged-in player joins using an IR-issued purpose-restricted ticket without exposing the IR bearer session to Arena.
- Passwords, owner kick, room-lifetime kick bans, ownership transfer, reconnect grace, and ephemeral destruction behave as specified.
- A reconnect can reclaim only its reserved seat using a fresh IR ticket and the matching in-memory seat-resume token; stale socket messages cannot mutate the seat.
- Local battle is silently disabled and cannot be re-enabled in Arena.
- The profile menu is unavailable in Arena song select; any external profile change leaves the room.
- Any player can replace the selection; a non-common chart cannot replace it.
- A completed song-library scan or other committed chart-set mutation publishes one generation-consistent inventory and resets/revalidates next-round state.
- All clients load the same SHA-256, #RANDOM choices, note randomization, DP mode, seed, and algorithm version through explicit transient configuration.
- Gauge, hi-speed, and presentation preferences remain personal and unchanged.
- All-ready atomically freezes the selection and roster and enters Loading; probe or load failures return the room to selection rather than deadlocking it.
- The final probe verifies the selected file content still matches the frozen SHA-256 and revision identifiers.
- New mid-round joins wait for the next round.
- Gameplay shows live competition and always uses the strongest opponent as the transient pacemaker.
- Scores save and upload to IR through the normal path.
- Winner and ranks use EX score and competition ties.
- Default, LR2, and Beatoraja results expose Arena position, with Default able to cycle to normal internet rankings.
- The live overlay can be moved and resized per profile, skin, and layout with F2.
- Destroying the room removes chat, inventories, selection, results, and lobby wins.
- The single-replica Docker service deploys through Coolify with WSS, health checks, limits, redacted logs, and no IR database access.
