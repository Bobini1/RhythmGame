# Arena Member Identity and Avatar Design

## Outcome

Arena surfaces display public player identity instead of opaque room identifiers. Room listings expose every current member for future presentation needs, while the initial lobby-card design remains compact. The unreleased protocol is simplified to one exact 1.0 contract, and room capacity increases from 16 to 32.

## Protocol contract

- Arena accepts and emits protocol major `1`, minor `0` only.
- Rooms, rounds, and competition capabilities remain explicit, but capabilities no longer imply protocol minor versions.
- Legacy 1.1/1.2 compatibility branches and fixtures are removed. Breaking the existing test deployment is acceptable because there are no public clients.
- Every directory room summary contains a required `members` array in stable seat order.
- Directory member entries contain only:
  - `displayName`
  - `avatarUrl`
  - `connected`
- Directory summaries do not expose user IDs, member IDs, seat tokens, readiness, inventory state, or other private room state.
- The API returns every room member. It does not impose the lobby card's four-avatar presentation limit.
- Room summaries and directory deltas change when membership, display name, avatar URL, or connection state changes.

## Capacity

- The authoritative room capacity is `32` across server validation, client validation, room models, fixtures, load tests, and UI copy.
- Connected and reserved members both count toward capacity and both appear in the public directory member array.
- The current implementation treats all room members as potential round participants. Spectator-only or social-room capacity is deferred.
- Lists must scroll or virtualize rather than dropping members from the UI.

## Selected-by identity

- `selectedByMemberId` remains the protocol and internal correlation value.
- `ArenaSession` exposes a derived `selectedByDisplayName` property resolved from the authoritative member model.
- The derived property updates when selection or member identity data changes.
- Selection summaries render `Selected by %1` with `selectedByDisplayName`.
- If a selection exists but its member is temporarily absent from the roster, the UI renders `Selected by another player` and never exposes the opaque member ID.

## Current-room avatars

- The joined-room screen keeps its existing 40-pixel member avatars.
- The shared Arena select roster adds a 32-pixel avatar before each member's text and controls.
- Avatar images load asynchronously with a bounded source size and preserve aspect crop.
- Empty, loading-failed, and invalid avatar URLs render a deterministic initial-based fallback.
- Avatar decoration is ignored by accessibility because the containing member row already announces the display name and status.

## Lobby-card avatars

- Each room card renders the first four members from the complete directory member array as overlapping 24-pixel avatars.
- If more than four members exist, the stack ends with a `+N` overflow indicator.
- Empty or failed avatar URLs use the same initial-based fallback as the current-room roster.
- The room card's accessible description includes the visible member display names and the total member count.
- Logged-out and logged-in browsers receive and may see the same public member previews.

## Repository boundaries

- `T:\RhythmGame-IR\arena-server` owns the canonical 1.0 schema, directory summary generation, capacity enforcement, codec fixtures, and server tests.
- `T:\RG` owns matching protocol decoding, C++ room/member models, `ArenaSession` identity derivation, QML avatar presentation, and client tests.
- Client and server changes are deployed together for this unreleased test protocol.

## Regression coverage

- Server tests require exact protocol 1.0 hellos with existing capability combinations and reject other minor versions.
- Server room and directory tests cover 32-member admission, rejection of member 33, complete public member summaries, stable order, reserved/connected status, and identity-driven directory deltas.
- Client protocol tests decode 32-member snapshots and directory summaries and reject 33-member payloads.
- Client model tests cover directory member roles and live updates.
- Session tests cover selector name resolution, identity changes, and the unresolved-member fallback.
- QML tests cover 32-pixel select-roster avatars, four lobby-card avatars, `+N`, image fallback, plain-text names, and accessible descriptions.
