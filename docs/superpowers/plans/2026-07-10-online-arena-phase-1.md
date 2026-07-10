# Online Arena Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Deliver authenticated multi-room Arena browsing, creation, passwords, ownership, kicking, chat, and reconnectable room seats across IR, the new Bun server, and the RhythmGame client.

**Architecture:** IR issues short-lived Ed25519 tickets through Better Auth and exposes public JWKS. A database-free Bun server owns ephemeral room state behind a versioned WSS protocol. RhythmGame exposes one deep ArenaSession module to application-owned QML through fakeable transport and identity seams.

**Tech Stack:** Qt 6.12/C++20/QML/Qt WebSockets/Catch2, Bun 1.3/TypeScript/Zod/jose/Bun.password/Bun test, SvelteKit/Better Auth/Drizzle/PostgreSQL/Vitest, Docker/Coolify.

## Global Constraints

- Preserve all unrelated dirty files in T:/RG and T:/RhythmGame-IR; stage only task-owned paths.
- Official issuer is https://rhythmgame.eu, audience is https://arena.rhythmgame.eu, purpose is arena-connect, ticket TTL is 90 seconds, and protocol is 1.0.
- The official client endpoint is configurable and defaults to wss://arena.rhythmgame.eu/ws; local integration uses ws://127.0.0.1:3001/ws.
- Room capacity is 16, reconnect grace is 60 seconds, and chat backlog is 200 messages.
- No IR session token, room password, JWT, resume token, or chat body may appear in Arena logs.
- Arena server state is in-memory and single-replica; it must not receive IR database credentials or Better Auth secrets.
- QML never parses wire messages and ArenaSession never manipulates StackView.
- Use TDD for domain and protocol behavior; commits contain one independently testable deliverable.

---

### Task 1: IR Arena ticket issuer

**Files:**
- Create: `T:/RhythmGame-IR/src/lib/server/auth/arena-ticket.ts`
- Create: `T:/RhythmGame-IR/src/lib/server/auth/tests/arena-ticket.test.ts`
- Modify: `T:/RhythmGame-IR/src/lib/server/auth/config.ts`
- Modify: `T:/RhythmGame-IR/src/lib/server/database/schemas/auth.ts`
- Create: generated `T:/RhythmGame-IR/src/lib/server/database/migrations/0002_*.sql`
- Create: generated `T:/RhythmGame-IR/src/lib/server/database/migrations/meta/0002_snapshot.json`
- Modify: `T:/RhythmGame-IR/src/lib/server/database/migrations/meta/_journal.json`

**Interfaces:**
- Produces: authenticated `GET /api/auth/token -> { token: string }` with `Cache-Control: no-store`.
- Produces: public `GET /api/auth/jwks`.
- Produces claims consumed by Task 3: `sub`, `name`, `picture`, `emailVerified`, `purpose`, `protocolMajor`, `protocolMinor`, and `jti`.

- [ ] **Step 1: Write failing ticket tests**

Create Vitest coverage that makes an authenticated bearer request, decodes the JWT, and asserts:

```ts
expect(header.alg).toBe('EdDSA');
expect(payload.iss).toBe('https://rhythmgame.eu');
expect(payload.aud).toBe('https://arena.rhythmgame.eu');
expect(payload.purpose).toBe('arena-connect');
expect(payload.protocolMajor).toBe(1);
expect(payload.protocolMinor).toBe(0);
expect(payload.exp! - payload.iat!).toBe(90);
expect(response.headers.get('cache-control')).toContain('no-store');
```

Also assert unauthenticated status 401, distinct `jti` values, matching JWKS `kid`, and no `set-auth-jwt` header on `/api/auth/get-session`.

- [ ] **Step 2: Run the focused test and confirm failure**

Run from `T:/RhythmGame-IR`:

```powershell
bun run test:server -- src/lib/server/auth/tests/arena-ticket.test.ts
```

Expected: failure because the JWT plugin/JWKS table is absent.

- [ ] **Step 3: Add the JWKS schema**

Add:

```ts
export const jwks = pgTable('jwks', {
	id: bigint('id', { mode: 'number' }).primaryKey().generatedAlwaysAsIdentity(),
	publicKey: text('public_key').notNull(),
	privateKey: text('private_key').notNull(),
	createdAt: timestamp('created_at').defaultNow().notNull(),
	expiresAt: timestamp('expires_at')
});
```

Export it through the existing auth schema and include `jwks` in the explicit Drizzle adapter schema.

- [ ] **Step 4: Implement the Arena JWT plugin helper**

Use `jwt` from `better-auth/plugins` with:

```ts
export const ARENA_ISSUER = 'https://rhythmgame.eu';
export const ARENA_AUDIENCE = 'https://arena.rhythmgame.eu';

export function createArenaJwtPlugin() {
	return jwt({
		disableSettingJwtHeader: true,
		jwks: {
			keyPairConfig: { alg: 'EdDSA', crv: 'Ed25519' },
			rotationInterval: 60 * 60 * 24 * 30,
			gracePeriod: 60 * 60 * 24
		},
		jwt: {
			issuer: ARENA_ISSUER,
			audience: ARENA_AUDIENCE,
			expirationTime: '90s',
			definePayload: ({ user }) => ({
				name: user.name,
				picture: user.image ?? null,
				emailVerified: user.emailVerified,
				purpose: 'arena-connect',
				protocolMajor: 1,
				protocolMinor: 0,
				jti: crypto.randomUUID()
			})
		}
	});
}
```

Add a Better Auth after hook that sets `Cache-Control: no-store` when `ctx.path === '/token'`.

- [ ] **Step 5: Generate the migration**

Run:

```powershell
bun run db:generate
```

Expected: one migration adding the five-column `jwks` table and updated migration metadata.

- [ ] **Step 6: Run IR verification**

```powershell
bun run test:server -- src/lib/server/auth/tests/arena-ticket.test.ts
bun run check
bun run lint
```

Expected: focused tests, Svelte check, Prettier check, and ESLint pass.

- [ ] **Step 7: Commit only IR ticket paths**

```powershell
git add src/lib/server/auth/arena-ticket.ts src/lib/server/auth/tests/arena-ticket.test.ts src/lib/server/auth/config.ts src/lib/server/database/schemas/auth.ts src/lib/server/database/migrations
git commit -m "feat: issue Arena identity tickets"
```

### Task 2: Arena server package, configuration, and codec

**Files:**
- Create: `T:/RhythmGame-IR/arena-server/package.json`
- Create: `T:/RhythmGame-IR/arena-server/bun.lock`
- Create: `T:/RhythmGame-IR/arena-server/tsconfig.json`
- Create: `T:/RhythmGame-IR/arena-server/.env.example`
- Create: `T:/RhythmGame-IR/arena-server/src/config.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/protocol/messages.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/protocol/codec.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/protocol/errors.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/unit/config.test.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/unit/codec.test.ts`

**Interfaces:**
- Produces: `ArenaConfig`, `ClientMessage`, `ServerMessage`, and strict `decodeClientMessage(text)`.
- Consumed by: Tasks 3-5.

- [ ] **Step 1: Create package scripts and failing config tests**

Pin `jose@6.2.3`, `zod@4.3.6`, `@types/bun@1.3.11`, TypeScript, and Prettier. Scripts must include `check`, `test`, `format:check`, and `verify`.

Test exact defaults:

```ts
expect(config.port).toBe(3001);
expect(config.roomCapacity).toBe(16);
expect(config.reconnectGraceMs).toBe(60_000);
expect(config.chatBacklog).toBe(200);
```

- [ ] **Step 2: Run tests and confirm failure**

```powershell
bun --cwd arena-server install
bun --cwd arena-server test tests/unit/config.test.ts
```

Expected: failure because config does not exist.

- [ ] **Step 3: Implement validated configuration**

Use Zod coercion for host/port/URLs/limits. Reject non-HTTP JWKS URLs, non-positive ports, capacity other than 16, and reconnect grace outside safe bounds.

- [ ] **Step 4: Write failing protocol tests**

Cover anonymous and ticket-bearing `client_hello`, optional resume, directory subscription, room create/join/leave/kick, chat, heartbeat, unknown fields, unsupported version, malformed JSON, and the 64 KiB text limit.

- [ ] **Step 5: Implement strict protocol unions**

Every client message has:

```ts
type ClientMessage = {
	type: string;
	requestId?: string;
	data: unknown;
};
```

Implement discriminated Zod schemas with `.strict()`; error responses expose stable codes but never echo passwords/tickets/chat bodies.

- [ ] **Step 6: Verify and commit package foundation**

```powershell
bun --cwd arena-server run verify
git add arena-server
git commit -m "feat: define Arena server protocol"
```

### Task 3: Ticket verification and replay protection

**Files:**
- Create: `T:/RhythmGame-IR/arena-server/src/auth/identity.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/auth/ticket-verifier.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/auth/jose-ticket-verifier.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/auth/replay-guard.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/helpers/test-ticket.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/unit/ticket-verifier.test.ts`

**Interfaces:**
- Produces:

```ts
export interface TicketVerifier {
	verify(ticket: string, now: Date): Promise<VerifiedArenaTicket>;
}
```

- Consumed by: ArenaApplication in Task 5.

- [ ] **Step 1: Write Ed25519 verification tests**

Generate a local Ed25519 key pair. Test valid ticket plus wrong algorithm, issuer, audience, purpose, protocol major, missing identity claims, expiry, lifetime over 120 seconds, and duplicate `jti`.

- [ ] **Step 2: Confirm the tests fail**

```powershell
bun --cwd arena-server test tests/unit/ticket-verifier.test.ts
```

- [ ] **Step 3: Implement verifier and replay guard**

Production uses `createRemoteJWKSet(new URL(config.irJwksUrl))` and `jwtVerify` pinned to `algorithms: ['EdDSA']`, issuer, and audience. Convert claims to:

```ts
type ArenaIdentity = {
	userId: string;
	displayName: string;
	avatarUrl: string | null;
};
```

Consume `jti` only after all claims validate and retain it until `exp`.

- [ ] **Step 4: Verify and commit**

```powershell
bun --cwd arena-server run verify
git add arena-server/src/auth arena-server/tests/helpers/test-ticket.ts arena-server/tests/unit/ticket-verifier.test.ts
git commit -m "feat: verify Arena identity tickets"
```

### Task 4: Room domain state

**Files:**
- Create: `T:/RhythmGame-IR/arena-server/src/rooms/models.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/rooms/password-hasher.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/rooms/bun-password-hasher.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/rooms/room.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/rooms/room-directory.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/helpers/fake-clock.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/helpers/fake-password-hasher.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/unit/room.test.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/unit/room-directory.test.ts`

**Interfaces:**
- Produces: pure room commands/results, public `RoomSummary`, private `RoomSnapshot`, and deliveries without socket calls.
- Consumed by: ArenaApplication in Task 5.

- [ ] **Step 1: Write room creation/join tests**

Test creator ownership, fixed capacity, password hashing/verification, wrong password, duplicate identity, public summary privacy, and complete private snapshot.

- [ ] **Step 2: Confirm failure**

```powershell
bun --cwd arena-server test tests/unit/room.test.ts
```

- [ ] **Step 3: Implement minimal room state**

Represent seats with opaque IDs, verified identity, join order, connection generation, connected/reserved state, lobby wins initialized to zero, and SHA-256 hash of a random 256-bit resume token. Return the plaintext token only in the private join/resume result.

- [ ] **Step 4: Add owner/kick/leave tests and implementation**

Test owner-only kick, no self-kick, room-lifetime user ban, explicit owner transfer to oldest connected member, immediate destruction after the last explicit leave, and no token leakage in public snapshots.

- [ ] **Step 5: Add disconnect/resume tests and implementation**

With fake time, test reservation for 60 seconds, resume requiring matching subject/token, stale socket replacement, connection-generation increment, token rotation, expiry removal, owner transfer on disconnect, and destruction after the last reserved seat expires.

- [ ] **Step 6: Add chat tests and implementation**

Test server-derived author, trimming, empty/over-500 rejection, five messages per ten seconds, ordered broadcast, and truncation to 200 messages.

- [ ] **Step 7: Verify and commit**

```powershell
bun --cwd arena-server run verify
git add arena-server/src/rooms arena-server/tests/helpers arena-server/tests/unit/room*.test.ts
git commit -m "feat: manage ephemeral Arena rooms"
```

### Task 5: Arena application, WebSocket gateway, and Docker

**Files:**
- Create: `T:/RhythmGame-IR/arena-server/src/application/delivery.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/application/arena-application.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/transport/start-server.ts`
- Create: `T:/RhythmGame-IR/arena-server/src/main.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/integration/arena-application.test.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/integration/websocket.test.ts`
- Create: `T:/RhythmGame-IR/arena-server/tests/integration/health.test.ts`
- Create: `T:/RhythmGame-IR/arena-server/Dockerfile`
- Create: `T:/RhythmGame-IR/arena-server/.dockerignore`
- Create: `T:/RhythmGame-IR/arena-server/README.md`

**Interfaces:**
- Produces: anonymous/public WSS `/ws` and `GET /healthz`.
- Consumed by: client and integration tests.

- [ ] **Step 1: Write application orchestration tests**

Use a fake TicketVerifier and RoomDirectory to test hello-first enforcement, anonymous directory access, authenticated create/join/resume, room broadcasts, kick delivery, heartbeat, disconnect, sweep, and stable structured errors.

- [ ] **Step 2: Implement ArenaApplication**

Expose:

```ts
connect(connectionId: string): readonly Delivery[];
receive(connectionId: string, message: ClientMessage, now: number): Promise<readonly Delivery[]>;
disconnect(connectionId: string, now: number): readonly Delivery[];
sweep(now: number): readonly Delivery[];
```

Keep socket objects outside this module.

- [ ] **Step 3: Write and implement real HTTP/WebSocket tests**

Start Bun on an ephemeral port. Test health, room list, upgrade path, two real clients, frame limits, chat delivery, close/resume, and graceful shutdown.

- [ ] **Step 4: Add the production entrypoint**

Use `Bun.serve` with typed socket data, payload/backpressure limits, periodic heartbeat/sweep, signal-triggered `server_going_away`, and redacted structured logs.

- [ ] **Step 5: Add Docker/operator files**

Pin `oven/bun:1.3.14-alpine`, install frozen dependencies, run as `bun`, expose 3001, and health-check `/healthz`. Document Coolify base directory `/arena-server`, port, environment, WSS path, single replica, and room loss on restart.

- [ ] **Step 6: Verify and commit**

```powershell
bun --cwd arena-server run verify
docker build -t rhythmgame-arena:phase1 arena-server
git add arena-server
git commit -m "feat: serve Arena lobbies over WebSocket"
```

### Task 6: Client protocol, models, and fake seams

**Files:**
- Create: `T:/RG/src/arena/ArenaTypes.h`
- Create: `T:/RG/src/arena/ArenaProtocol.h`
- Create: `T:/RG/src/arena/ArenaProtocol.cpp`
- Create: `T:/RG/src/arena/ArenaTransport.h`
- Create: `T:/RG/src/arena/ArenaIdentityProvider.h`
- Create: `T:/RG/src/arena/ArenaScheduler.h`
- Create: `T:/RG/src/arena/ArenaRoomListModel.h/.cpp`
- Create: `T:/RG/src/arena/ArenaMemberListModel.h/.cpp`
- Create: `T:/RG/src/arena/ArenaChatModel.h/.cpp`
- Create: `T:/RG/test/arena/FakeArenaTransport.h`
- Create: `T:/RG/test/arena/FakeArenaIdentityProvider.h`
- Create: `T:/RG/test/arena/FakeArenaScheduler.h`
- Create: `T:/RG/test/arena/ArenaProtocol.test.cpp`
- Create: `T:/RG/test/arena/ArenaModels.test.cpp`
- Modify: `T:/RG/CMakeLists.txt`
- Modify: `T:/RG/test/CMakeLists.txt`

**Interfaces:**
- Produces C++ wire types/models and fake seams consumed by Task 7.

- [ ] **Step 1: Write failing C++ codec tests**

Use golden Phase 1 messages for anonymous/ticket/resume hello, directory snapshot, room snapshot, chat, owner change, and errors. Assert malformed JSON, missing/unknown fields, protocol major mismatch, and limits are rejected without partial state.

- [ ] **Step 2: Confirm failure**

```powershell
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaProtocol --output-on-failure
```

- [ ] **Step 3: Implement typed codec**

Use `QJsonDocument/QJsonObject`, explicit required-field helpers, and `std::variant` decoded messages. Never expose raw QJsonObject to QML.

- [ ] **Step 4: Write model tests and implement replace/incremental operations**

Test exact role names, full snapshot replacement, updates by opaque ID, stable order, removal, clear, and no dangling QObject-backed references.

- [ ] **Step 5: Verify and commit**

```powershell
ctest --preset dev-rel -R "ArenaProtocol|ArenaModels" --output-on-failure
git add CMakeLists.txt test/CMakeLists.txt src/arena test/arena
git commit -m "feat: add Arena protocol models"
```

### Task 7: Client ticket operation, transport, and ArenaSession

**Files:**
- Modify: `T:/RG/src/resource_managers/Profile.h`
- Modify: `T:/RG/src/resource_managers/Profile.cpp`
- Create: `T:/RG/src/arena/QtWebSocketArenaTransport.h/.cpp`
- Create: `T:/RG/src/arena/ProfileArenaIdentityProvider.h/.cpp`
- Create: `T:/RG/src/arena/QtArenaScheduler.h/.cpp`
- Create: `T:/RG/src/arena/ArenaSession.h/.cpp`
- Create: `T:/RG/test/arena/ArenaSession.test.cpp`
- Modify: `T:/RG/CMakeLists.txt`
- Modify: `T:/RG/vcpkg.json`
- Modify as required: `T:/RG/flake.nix`, `T:/RG/nix/packages/rhythmgame.nix`, `T:/RG/nix/shells/default.nix`

**Interfaces:**
- Produces the final Phase 1 QML-facing ArenaSession contract defined in the Phase 1 design.
- Consumed by Task 8.

- [ ] **Step 1: Write failing ArenaSession tests**

Cover anonymous hello/directory, logged-out create/join gating, pending action after ticket, ticket-bearing reconnect, room snapshot replacement, password error preservation, chat/member events, leave/kick, disconnect/resume/token rotation, logout/profile change cleanup, grace failure, and explicit `exitArena()` cleanup through fake scheduling.

- [ ] **Step 2: Confirm focused failure**

```powershell
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R ArenaSession --output-on-failure
```

- [ ] **Step 3: Add Profile::requestArenaTicket**

Issue authenticated `GET api/auth/token` through the existing private request factory. Return a small QObject operation or signals containing only the ticket/error; never return the bearer request or token.

- [ ] **Step 4: Implement ArenaSession against fakes**

Implement the approved state enum with `Browsing` plus a separate `authenticated` property, commands including `exitArena()`, pending create/join action, snapshot replacement, request correlation, reconnect backoff through injected cancellable scheduling, seat token memory, and identity-change cleanup.

- [ ] **Step 5: Add QWebSocket adapter and build dependencies**

Add `WebSockets` to `find_package(Qt6 COMPONENTS ...)`, link `Qt6::WebSockets`, add clean-worktree `qtwebsockets` to vcpkg, and include Nix Qt WebSockets where required. Implement the adapter as a thin signal bridge.

- [ ] **Step 6: Verify and commit**

```powershell
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame_test -j 2
ctest --preset dev-rel -R Arena --output-on-failure
git add CMakeLists.txt vcpkg.json flake.nix nix src/arena src/resource_managers/Profile.h src/resource_managers/Profile.cpp test/arena
git commit -m "feat: connect the Arena client session"
```

### Task 8: Application wiring and Phase 1 QML

**Files:**
- Modify: `T:/RG/src/main.cpp`
- Modify: `T:/RG/RhythmGameQml/Rg.h`
- Modify: `T:/RG/RhythmGameQml/Rg.cpp`
- Modify: `T:/RG/RhythmGameQml/QmlForeignTypes.h`
- Modify: `T:/RG/RhythmGameQml/CMakeLists.txt`
- Create: `T:/RG/RhythmGameQml/Arena/ArenaBrowser.qml`
- Create: `T:/RG/RhythmGameQml/Arena/ArenaLoginPanel.qml`
- Create: `T:/RG/RhythmGameQml/Arena/ArenaRoom.qml`
- Modify: `T:/RG/RhythmGameQml/ContentFrame.qml`
- Modify: `T:/RG/share/RhythmGame/themes/Default/scripts/main/Main.qml`
- Modify: `T:/RG/cmake/translations.cmake`

**Interfaces:**
- Consumes: ArenaSession from Task 7.
- Produces: user-visible anonymous Browser, inline login, and holding-room UI.

- [ ] **Step 1: Wire ArenaSession into Rg/main**

Construct transport, identity provider, and session after ProfileList; expose a constant non-owning `ArenaSession*` property on Rg and register its enum for QML.

- [ ] **Step 2: Add Browser navigation**

Add `openArenaBrowser()` to ContentFrame. It calls `connectForBrowsing()`, pushes the application-owned component, and lets QML react to session state. Add an Online Arena main-menu button.

- [ ] **Step 3: Implement Browser and inline login**

Use Qt Quick Controls and layouts. Include:

- Connection/retry status.
- Room list with lock, phase, connected/reserved/16.
- Empty/loading/error states.
- Create dialog and password join dialog.
- Inline active-profile email/password login using existing Profile login methods.

- [ ] **Step 4: Implement holding room**

Show room name, owner/member list, connected/reserved state, chat backlog/input, owner kick, and leave. Escape/render chat as plain text.

- [ ] **Step 5: Add translations and perform QML checks**

Add QML files to the module and translation extraction. Run the available QML cache compilation as part of the executable build and inspect at 1280x720 plus a narrow window.

- [ ] **Step 6: Verify and commit**

```powershell
cmake --build --preset dev-rel --target RhythmGame_exe -j 2
ctest --preset dev-rel -R Arena --output-on-failure
git add src/main.cpp RhythmGameQml share/RhythmGame/themes/Default/scripts/main/Main.qml cmake/translations.cmake
git commit -m "feat: add the Arena lobby browser"
```

### Task 9: Cross-repository Phase 1 integration

**Files:**
- Create: `T:/RhythmGame-IR/arena-server/scripts/phase1-smoke.ts`
- Modify: `T:/RhythmGame-IR/arena-server/README.md`
- Modify: `T:/RG/docs/superpowers/specs/2026-07-10-online-arena-phase-1-design.md` only if verified behavior required clarification

**Interfaces:**
- Validates the deployed contract shared by all prior tasks.

- [ ] **Step 1: Add a scripted two-client smoke test**

The script starts/targets a local server and proves:

1. Anonymous directory is readable.
2. Client A creates a password room.
3. Client B sees it, fails a wrong password, then joins correctly.
4. Chat reaches both clients.
5. Non-owner kick fails; owner kick succeeds and bans rejoin.
6. A fresh room supports disconnect, fresh JWT plus resume token, token rotation, owner transfer, grace expiry, and destruction.

- [ ] **Step 2: Run complete server/IR verification**

```powershell
bun run test:server
bun run check
bun --cwd arena-server run verify
docker build -t rhythmgame-arena:phase1 arena-server
```

- [ ] **Step 3: Run complete client verification**

```powershell
cmake --preset dev-rel
cmake --build --preset dev-rel --target RhythmGame_test RhythmGame_exe -j 2
ctest --preset dev-rel --output-on-failure
```

- [ ] **Step 4: Run the local contract smoke**

Use a local test issuer or injected verifier; never use production session credentials in tests.

```powershell
bun --cwd arena-server run scripts/phase1-smoke.ts
```

Expected: all nine assertions complete and the server exits cleanly.

- [ ] **Step 5: Review against Phase 1 exit criteria**

Check every exit criterion in the Phase 1 spec and record any intentional rough edge in the implementation report. Do not leave placeholders in code or docs.

- [ ] **Step 6: Commit smoke/verified doc changes**

```powershell
git add arena-server/scripts/phase1-smoke.ts arena-server/README.md
git commit -m "test: verify the Arena lobby flow"
```
