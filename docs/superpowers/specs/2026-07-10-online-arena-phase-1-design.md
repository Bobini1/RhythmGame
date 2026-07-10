# Online Arena Phase 1 design

## Status

Derived from the approved umbrella design in 2026-07-10-online-arena-design.md. The user authorized autonomous assumptions and implementation without an additional review gate.

## Goal

Deliver a working, testable foundation in which:

- An unauthenticated RhythmGame client can connect to the official Arena service and browse many rooms.
- A logged-in profile can obtain a short-lived IR-issued Arena ticket.
- Two authenticated clients can create, discover, join, and leave public or password-protected rooms.
- Room ownership, kicking, room-lifetime kick bans, chat, disconnect grace, and seat resumption work.
- The server is independently containerized and exposes a health endpoint.
- No chart selection, inventory, ready/start, gameplay, or result behavior is implemented yet.

## Autonomous assumptions

1. The independently deployable Bun server source lives at T:/RhythmGame-IR/arena-server. This keeps deployment material near the existing Netcup/Coolify project without coupling the server process to SvelteKit or the IR database.
2. The official production endpoint is wss://arena.rhythmgame.eu/ws. The client URL is configuration, with ws://127.0.0.1:3001/ws used for local integration.
3. One WebSocket endpoint serves both anonymous browsing and authenticated room sessions. ClientHello is always the first frame and optionally carries the Arena ticket and seat-resume data. Inline login reconnects with an authenticated ClientHello.
4. Phase 1 fixes protocol major/minor at 1.0 and supports the rooms-v1 capability.
5. Rooms have a fixed maximum of 16 seats, a 60-second reconnect grace period, and a bounded 200-message in-memory chat backlog.
6. Password hashing uses Bun.password with Argon2id so the server needs no native password dependency.
7. A fresh IR ticket plus the current Arena seat-resume token is required to reclaim a disconnected seat. Successful resumption rotates the seat token and connection generation.
8. The first UI is intentionally functional rather than polished. The Arena Browser is application-owned QML and remains usable regardless of the selected gameplay skin.
9. All new user-visible strings are translatable. Polish copy can follow after the English flow is stable if translation coverage would otherwise block the foundation.
10. Existing unrelated dirty files in both repositories are user-owned and must not be staged, rewritten, or removed.

## Repository layout

### T:/RhythmGame-IR

- Existing SvelteKit application: IR identity issuer and JWKS endpoint.
- New arena-server/: independent Bun package, tests, Dockerfile, and health endpoint.

### T:/RG

- src/arena/: protocol codec, transport, identity adapter, room models, and ArenaSession.
- RhythmGameQml/Arena/: application-owned Browser and room UI.
- Existing main/Rg/ContentFrame integration points expose the session and navigation.
- test/arena/: Catch2 protocol and session tests.

## IR ticket issuer

### Better Auth configuration

The existing Better Auth application enables its installed jwt plugin alongside bearer.

Required configuration:

- EdDSA signing.
- Issuer https://rhythmgame.eu.
- Audience https://arena.rhythmgame.eu.
- 90-second expiration.
- disableSettingJwtHeader enabled.
- JWKS rotation with retired verification keys retained beyond the maximum ticket lifetime.
- Additional payload claims purpose = arena-connect, protocolMajor = 1, protocolMinor = 0, and a random jti.

The explicit Drizzle auth schema adds the Better Auth jwks model and a generated migration. GET /api/auth/token remains session-authenticated through the existing bearer plugin. Its response must carry Cache-Control: no-store.

### Client identity adapter

Profile gains a narrow asynchronous requestArenaTicket operation using its existing authenticated request factory. It returns only the short-lived JWT or a structured error. The IR bearer token remains private to Profile.

ArenaIdentityProvider observes ProfileList.mainProfileChanged and the active profile's login state. It requests a ticket only when ArenaSession needs to open or re-establish an authenticated socket.

## Phase 1 protocol

### ClientHello

ClientHello is the first WebSocket frame:

- type: client_hello
- protocolMajor: 1
- protocolMinor: 0
- clientVersion: application version string
- capabilities: array containing rooms-v1
- ticket: optional IR JWT
- resume: optional roomId and seatToken

The server responds with server_hello or a structured fatal error. server_hello contains the negotiated version, capabilities, authenticated identity when present, and a room snapshot when resumption succeeds.

Anonymous sockets can subscribe to the directory but cannot create, join, chat, or mutate rooms.

### Client commands

- directory_subscribe
- room_create with requestId, bounded room name, and optional password
- room_join with requestId, roomId, and optional password
- room_leave
- room_kick with target member ID
- chat_send with bounded plain text
- heartbeat reply

### Server events

- directory_snapshot and room_directory_updated
- room_snapshot
- room_member_joined, room_member_updated, and room_member_left
- room_owner_changed
- chat_message
- server_heartbeat
- server_going_away
- command_error with requestId, stable error code, and translatable display message key

Every post-join command and room event includes the room generation. Commands from the wrong generation, connection generation, room, or phase are rejected.

### Directory representation

Each room summary contains:

- Stable opaque room ID.
- Name.
- Selecting phase.
- Password indicator.
- Connected count.
- Reserved count.
- Maximum count of 16.

No player identity or chat body is exposed through the anonymous directory.

## Server modules

### Config

Validates environment at startup:

- PORT, default 3001.
- IR_JWKS_URL, default https://rhythmgame.eu/api/auth/jwks.
- IR_ISSUER, default https://rhythmgame.eu.
- ARENA_AUDIENCE, default https://arena.rhythmgame.eu.
- RECONNECT_GRACE_MS, default 60000.
- ROOM_CAPACITY, fixed/default 16.
- CHAT_BACKLOG, default 200.

Invalid production configuration fails startup rather than silently weakening authentication.

### Protocol

Zod schemas validate all JSON frames. Incoming text and binary sizes are capped before parsing. The codec produces discriminated TypeScript unions and stable command-error codes.

### TicketVerifier

Uses jose createRemoteJWKSet and jwtVerify. It pins EdDSA, issuer, audience, purpose, and required claims. It maintains an expiry-bounded jti replay cache.

### RoomDirectory

Owns all rooms and emits public directory snapshots/updates. It is the only module that creates and destroys Room instances.

### Room

Owns:

- Generation and opaque ID.
- Name and optional Argon2id password hash.
- Owner seat ID.
- Connected and reserved member seats.
- Room-lifetime banned IR subjects.
- Bounded chat messages.
- Reconnect timers.

It returns state-transition results instead of writing sockets directly. The WebSocket adapter translates results into protocol events.

### Seat resumption

Joining creates a random seat ID and high-entropy resume token. A lost socket marks the seat disconnected and starts grace. The room keeps identity, ownership history, and chat visibility state during grace.

A resume requires:

- A fresh valid IR ticket for the same subject.
- Matching room ID and resume token.
- A seat still inside grace.

Success closes/replaces any stale socket, increments connection generation, rotates the token, cancels expiry, and returns a complete room snapshot. Grace expiry removes the seat. If the room becomes empty after the final expiry, RoomDirectory destroys it.

### Ownership and kicking

- Creator is owner.
- Owner can kick another seat.
- Kick immediately removes the seat, invalidates its token, and bans its IR subject for the room lifetime.
- Explicit owner leave or owner disconnect transfers ownership to the longest-present connected member.
- When no connected member exists, the next connected/joining member becomes owner.

### Chat

Authenticated joined seats can send bounded plain text. The server assigns message ID and timestamp and relays it. It stores only the latest 200 messages for snapshots. Chat is never written to logs or disk.

### WebSocket application

The Bun entrypoint owns sockets, heartbeat, per-IP and per-identity limits, frame limits, and graceful shutdown. It delegates all state transitions to RoomDirectory/Room and sends their returned events.

GET /healthz reports process/configuration liveness without depending on current IR availability, preventing an IR outage from causing a container restart loop. Authentication failures caused by JWKS/IR availability are exposed through metrics and command errors while anonymous directory browsing continues. The service has no database.

## Client modules

### ArenaTransport

Abstract QObject interface:

- connectTo(url)
- sendText(message)
- sendBinary(bytes)
- close()
- connected, disconnected, textReceived, binaryReceived, and transportError signals

QtWebSocketArenaTransport is the production adapter. FakeArenaTransport records writes and injects events deterministically in Catch2.

### ArenaProtocol

Owns JSON encoding/decoding, typed enums, protocol constants, limits, and rejection of malformed/incompatible frames. QML never sees raw JSON.

### ArenaSession

Public state:

- Disconnected.
- Browsing.
- Connecting authenticated.
- In room.
- Reconnecting.
- Error.

Public commands:

- connectForBrowsing()
- exitArena()
- createRoom(name, password)
- joinRoom(roomId, password)
- leaveRoom()
- kickMember(memberId)
- sendChat(text)
- retry()

Read-only models:

- ArenaRoomListModel.
- ArenaMemberListModel.
- ArenaChatModel.

The session requests an IR ticket only for create/join/resume, reconnects with ClientHello, stores seat tokens only in memory, and replaces all room state from room_snapshot. It never controls StackView directly.

`authenticated` is exposed independently from the screen state because a user can remain on the Browser after leaving a room. `exitArena()` leaves any room when possible, clears pending actions and transient credentials, disconnects, and releases Arena-only application policy gates.

### Application integration

- Rg exposes a constant arenaSession property.
- main.cpp constructs the identity adapter, WebSocket transport, and ArenaSession after ProfileList and before Rg.
- ContentFrame owns openArenaBrowser and reacts to session state.
- The main menu gains an Online Arena entry.
- The Browser shows live rooms, loading/error/empty states, password badges, and create/join controls.
- Logged-out users can browse. Attempting create/join opens inline login using the active profile's existing login operation, then retries authenticated connection after success.
- Joined users see the room roster, owner, chat, leave, and owner kick controls. Phase 2 replaces this holding lobby with Arena song select.

## Error behavior

- Incompatible protocol: close with an update-required message.
- IR unavailable or ticket failure: retain anonymous directory browsing and show login/service error.
- Arena unavailable: retain Browser with retry.
- Wrong password, full room, banned identity, duplicate join, and permission failure: return stable command errors without dropping the socket.
- Malformed or oversized traffic: close after a structured fatal error when safe.
- Socket loss in room: enter Reconnecting and retry with bounded exponential backoff until grace expires.
- Resume failure or grace expiry: clear seat credentials and return to anonymous Browser.
- Server shutdown: show going-away reason and return to Browser.
- Active profile change or logout while joined: send leave when possible, clear seat credentials, and return to Browser.

## Security and privacy

- Official production transport is WSS.
- JWTs and seat tokens never appear in URLs or logs.
- Ticket jti and seat token are separate replay protections.
- Room passwords use Argon2id and password attempts are rate-limited.
- Room names and chat are always treated as untrusted plain text.
- Anonymous directory data excludes member identities.
- No room, password, token, chat, or connection state is persisted.
- Structured logs use room/seat IDs and event names without chat bodies or credentials.

## Tests

### IR

- Better Auth config exposes /api/auth/jwks and authenticated /api/auth/token.
- Ticket rejects an unauthenticated request.
- Ticket claims have EdDSA, exact issuer/audience/purpose, protocol version, jti, and short expiry.
- Token response uses no-store and ordinary session responses do not receive the JWT header.

### Arena server

- Protocol schema acceptance/rejection and size limits.
- Ticket signature/claim failures and jti replay.
- Create/join/list public and password rooms.
- Wrong password, full room, duplicate identity, and kick ban.
- Owner transfer on leave/disconnect.
- Chat ordering, backlog bound, and sanitization contract.
- Disconnect reservation, resume rotation, stale socket generation, grace expiry, and last-seat room destruction with fake time.
- Anonymous mutation rejection.
- Health and graceful shutdown.

### Client

- Protocol codec and incompatibility.
- Anonymous browse.
- Create/join ticket acquisition.
- Room snapshot replaces models.
- Wrong-password errors preserve Browser state.
- Disconnect enters Reconnecting; valid snapshot restores room.
- Resume failure returns Browser.
- Profile change/logout leaves and clears seat token.
- QML smoke/manual verification for main-menu entry, Browser, inline login, room list, password prompt, roster, chat, kick, leave, loading, empty, and error states.

## Exit criteria

- IR issues and Arena verifies a purpose- and audience-restricted short-lived ticket without sharing the IR session token, database, or secret.
- At least two server-side integration clients can create, discover, join, chat, leave, disconnect/resume, transfer ownership, and kick in a password room.
- An unauthenticated RhythmGame client can browse but cannot mutate rooms.
- A logged-in RhythmGame client can create/join and receives live room/chat models.
- The application returns safely to Browser on profile change, logout, server loss, or failed resume.
- The independently built Arena Docker image starts without a database and answers /healthz.
- Focused IR, Bun, Catch2, and cross-repository integration tests pass.
