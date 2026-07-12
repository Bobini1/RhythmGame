# Arena Member Identity Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reset the unreleased Arena protocol to exact 1.0, raise room capacity to 32, expose complete public room-member summaries, and display names and avatars throughout Arena UI.

**Architecture:** `T:\RhythmGame-IR\arena-server` remains authoritative for protocol schemas, room capacity, and directory summaries. `T:\RG` mirrors the wire contract into typed C++ models, derives selector display identity in `ArenaSession`, and uses one reusable QML avatar component in room cards and the shared select roster.

**Tech Stack:** TypeScript, Bun, Zod, Qt 6 C++, QAbstractListModel, QML/Qt Quick Controls, Catch2, Qt Quick Test, CMake.

## Global Constraints

- Accept and emit protocol major `1`, minor `0` only.
- Keep rooms, rounds, and competition capabilities without tying them to minor versions.
- Set authoritative capacity to 32; member 33 is rejected.
- Directory summaries return every member in stable seat order as `displayName`, `avatarUrl`, and `connected`, without IDs or private state.
- The lobby card renders only the first four members and `+N`; the API remains complete.
- Internal selection correlation remains `selectedByMemberId`; UI uses `selectedByDisplayName` or “Selected by another player.”
- Preserve unrelated dirty and untracked files in both repositories.
- Use red-green-refactor for each production change.

---

### Task 1: Server Protocol 1.0, Capacity 32, and Complete Directory Members

**Files:**
- Modify: `T:\RhythmGame-IR\arena-server\src\protocol\messages.ts`
- Modify: `T:\RhythmGame-IR\arena-server\src\protocol\codec.ts`
- Modify: `T:\RhythmGame-IR\arena-server\src\application\arena-application.ts`
- Modify: `T:\RhythmGame-IR\arena-server\src\rooms\models.ts`
- Modify: `T:\RhythmGame-IR\arena-server\src\rooms\room.ts`
- Modify: server unit/integration tests, smoke scripts, and protocol fixtures containing protocol minors, capacity 16, or room summaries

**Interfaces:**
- Produces protocol `1.0`, `MAX_MEMBERS = 32`, and `RoomSummary.members: readonly PublicRoomMember[]`.
- `PublicRoomMember` contains `displayName: string`, `avatarUrl: string | null`, and `connected: boolean`.

- [ ] **Step 1: Write failing server tests**

Update protocol tests to require exact minor zero, make a 32-member room valid and a 33-member room invalid, and require complete directory members:

```ts
const parsed = serverMessageSchema.parse({
  type: 'room-list-snapshot',
  revision: 1,
  rooms: [summary]
});
expect(parsed.rooms[0]?.members).toEqual([
  { displayName: 'Alice', avatarUrl: null, connected: true },
  { displayName: 'Bob', avatarUrl: 'https://example.test/bob.png', connected: false }
]);
expect(() => serverMessageSchema.parse(snapshotWithMemberCount(33))).toThrow();
```

Add a room-directory test proving a member identity/status update changes the directory revision and preserves join order.

- [ ] **Step 2: Run server tests and verify RED**

Run:

```powershell
bun test
```

from `T:\RhythmGame-IR\arena-server`. Expected: failures for protocol minor 2, max count 16, and absent summary members.

- [ ] **Step 3: Implement the server contract**

Set `PROTOCOL_MINOR = 0`, `MAX_MEMBERS = 32`, and hello schemas to `z.literal(PROTOCOL_MINOR)`. Simplify negotiation so accepted capabilities are the requested ordered dependency chain at minor zero; `supportsRounds()` and `supportsCompetition()` check capabilities only.

Add:

```ts
export type PublicRoomMember = Readonly<{
  displayName: string;
  avatarUrl: string | null;
  connected: boolean;
}>;
```

to room models and schema. In `summaryFor(room)`, map every join-ordered seat to the three public fields. Ensure `copyRoomSummary()` deep-copies `members`.

- [ ] **Step 4: Update unreleased fixtures and smoke assertions**

Rewrite hello fixtures to minor zero while retaining capability combinations, replace max count 16 with 32, include `members` in every room summary, and replace the old assertion that public summaries omit members with exact public-field assertions.

- [ ] **Step 5: Verify server GREEN**

Run:

```powershell
bun run check
bun test
bun run format:check
```

Expected: type checking, all server tests, and formatting pass.

- [ ] **Step 6: Commit server changes**

Commit only changed `arena-server` files in `T:\RhythmGame-IR` with message `feat: expose Arena room member previews`.

### Task 2: Client Protocol and Directory Model

**Files:**
- Modify: `src/arena/ArenaTypes.h`
- Modify: `src/arena/ArenaProtocol.cpp`
- Modify: `src/arena/ArenaRoomListModel.h`
- Modify: `src/arena/ArenaRoomListModel.cpp`
- Modify: `test/arena/ArenaProtocol.test.cpp`
- Modify: `test/arena/ArenaModels.test.cpp`
- Modify: Arena client/session fixtures that encode protocol minors, capacity 16, or room summaries

**Interfaces:**
- Consumes server Task 1 wire fields.
- Produces `RoomMemberPreview { QString displayName; std::optional<QUrl> avatarUrl; bool connected; }`, `RoomSummary.members`, and QML role `members` as a QVariantList of maps.

- [ ] **Step 1: Write failing client protocol/model tests**

Require `ProtocolMinor == 0`, `RoomCapacity == 32`, decode all summary members, reject 33 members, and require the model role:

```cpp
CHECK(rooms.roleNames().value(ArenaRoomListModel::MembersRole) == "members");
const auto previews = rooms.data(rooms.index(0, 0), ArenaRoomListModel::MembersRole).toList();
CHECK(previews.at(0).toMap().value("displayName") == "Alice");
```

- [ ] **Step 2: Run focused client tests and verify RED**

Run:

```powershell
cmake --build build/dev-rel --config Release --target RhythmGame_test --parallel 2
build/dev-rel/test/bin/RhythmGame_test.exe "[arena][ArenaProtocol],[arena][ArenaModels]"
```

Expected: failures for old protocol constants, capacity, missing summary parsing, and missing role.

- [ ] **Step 3: Implement client types and parsing**

Remove legacy/round minor constants, set `ProtocolMinor = 0` and `RoomCapacity = 32`, accept exact 1.0, and make capability validation independent of minor version. Parse required room-summary `members` with the same identity URL constraints used by full members.

- [ ] **Step 4: Expose directory members to QML**

Add `MembersRole` and return a `QVariantList` whose entries contain exactly `displayName`, string `avatarUrl`, and `connected`. Include the role in room upsert change notifications when member previews differ.

- [ ] **Step 5: Update client fixtures and verify GREEN**

Rewrite client test payloads to exact 1.0, capacity 32, and required room-summary members. Run the focused command from Step 2; expect all selected tests to pass.

- [ ] **Step 6: Commit client protocol changes**

Commit only Task 2 files in `T:\RG` with message `feat: expose Arena directory members`.

### Task 3: Selector Display Identity

**Files:**
- Modify: `src/arena/ArenaMemberListModel.h`
- Modify: `src/arena/ArenaMemberListModel.cpp`
- Modify: `src/arena/ArenaSession.h`
- Modify: `src/arena/ArenaSession.cpp`
- Modify: `RhythmGameQml/Arena/ArenaSelectionSummary.qml`
- Modify: `test/arena/ArenaModels.test.cpp`
- Modify: `test/arena/ArenaSession.test.cpp`
- Modify: `test/qml/FakeArenaSession.qml`
- Modify: `test/qml/tst_ArenaDefaultSelect.qml`

**Interfaces:**
- Produces `ArenaMemberListModel::displayNameForMemberId(QStringView) const` and QML property `ArenaSession.selectedByDisplayName`.

- [ ] **Step 1: Write failing identity tests**

Add model/session tests that resolve `member-2` to `Bobini`, update after an identity upsert, and return empty for an absent member. Update the QML summary test to expect `Selected by Bobini` and fallback `Selected by another player`, never `member-2`.

- [ ] **Step 2: Verify RED**

Run focused C++ and Arena QML tests. Expected: missing model method/property and raw ID output.

- [ ] **Step 3: Implement derived identity**

Add the member-model lookup. Add `selectedByDisplayName` with its own notify signal to `ArenaSession`; emit when selection or member identity/model state changes. Keep the raw member ID property unchanged.

- [ ] **Step 4: Render the display name and fallback**

Bind `ArenaSelectionSummary` to `selectedByDisplayName`. When `selectedByMemberId` is non-empty but the display name is empty, render `qsTr("Selected by another player")`.

- [ ] **Step 5: Verify GREEN and commit**

Run focused session/model and Arena QML tests, then commit Task 3 files with message `fix: show Arena selector display name`.

### Task 4: Reusable Avatar, Select Roster, and Lobby Cards

**Files:**
- Create: `RhythmGameQml/Arena/ArenaAvatar.qml`
- Modify: `RhythmGameQml/CMakeLists.txt`
- Modify: `RhythmGameQml/Arena/ArenaRosterView.qml`
- Modify: `RhythmGameQml/Arena/ArenaBrowser.qml`
- Modify: `test/qml/tst_ArenaDefaultSelect.qml`
- Create: `test/qml/tst_ArenaBrowser.qml`

**Interfaces:**
- Consumes `avatarUrl`, `displayName`, and directory `members` role.
- Produces reusable `ArenaAvatar` with initial fallback; roster size 32; lobby-card size 24, first four plus `+N`.

- [ ] **Step 1: Write failing QML presentation tests**

Require roster object `arenaRosterAvatar-member-1` at 32x32, fallback initial `B`, four room-card avatars for a six-member summary, and overflow text `+2`. Require member names in the room card accessible description.

- [ ] **Step 2: Verify RED**

Build `RhythmGame_arena_qml_test` and run `ctest -R "^ArenaQml$"`; expect missing avatar objects and overflow UI.

- [ ] **Step 3: Create and register `ArenaAvatar`**

Implement a clipped circular `Rectangle` with asynchronous `Image`, bounded `sourceSize`, `PreserveAspectCrop`, and a centered plain-text uppercase initial visible unless the image is ready. Mark internal image/text decoration accessibility-ignored.

- [ ] **Step 4: Add select-roster avatars**

Consume the existing `avatarUrl` member role and place a 32x32 `ArenaAvatar` before the member text column without changing kick/readiness behavior.

- [ ] **Step 5: Add lobby-card avatar stacks**

Render `members.slice(0, 4)` as overlapping 24x24 avatars. Derive overflow as `members.length - 4`, render `+N` only when positive, and append all display names to the room card's accessible description.

- [ ] **Step 6: Verify GREEN and commit**

Run Arena QML tests and commit Task 4 files with message `feat: show Arena member avatars`.

### Task 5: Cross-repository Verification

- [ ] **Step 1: Verify the server**

Run `bun run verify` from `T:\RhythmGame-IR\arena-server`.

- [ ] **Step 2: Verify the client**

Run the aggregate Release build and `ctest --test-dir build/dev-rel -C Release --output-on-failure` from `T:\RG`. If the game executable is open, close it before the aggregate link.

- [ ] **Step 3: Inspect scope**

Run `git diff --check`, `git status --short`, and recent logs in both repositories. Confirm unrelated local files remain untouched.
