# Arena Phase 4 client verification

Date: 2026-07-11  
Client implementation head: `74ef15f096734c8b7dc94439ca7da572e49f245d`  
Server head: `bd7d9bd`  
Protocol: 1.2

This report covers the integrated RhythmGame Arena client, theme presentation,
accessibility, localization, and cross-repository protocol contract. Server,
container, load, privacy, and Coolify evidence is recorded in
`arena-server/docs/phase4-verification.md` in the RhythmGame-IR repository.

## Placement architecture

Arena placement uses the existing profile theme-vars pipeline exclusively.
`Vars.cpp` seeds hidden `arenaOverlay{K5,K7,K10,K14,Result}` normalized geometry
fields in the relevant theme property maps. `ArenaOverlayPlacementFrame.qml`
reads and commits those four fields at interaction boundaries. `-1` remains the
unset/default sentinel.

There is no Arena placement QObject, C++ placement class, standalone settings
file, JSON store, `placementStore`, or visibility preference. The compact live
leaderboard remains forced while Arena gameplay owns the active runner. The
only profile-wide non-geometry value is the first-use F2 hint version.

## Automated client gates

The final integrated target build completed successfully:

```text
cmake --build --preset dev-rel --target \
  RhythmGame_test RhythmGame_arena_qml_test RhythmGame_qml_qmllint \
  RhythmGame_exe Default_translations release_translations -j 2
exit 0
```

The QML lint target retains existing project warning classes, including
unqualified nested-component references and build-tree type-resolution
warnings, but emitted no build-stopping diagnostic.

The complete configured CTest suite passed:

```text
ctest --preset dev-rel --output-on-failure
275 passed, 0 failed
```

This includes the Arena QML suite plus protocol, identity, profile cleanup,
inventory/rescan, selection/availability, round loading, synchronized play,
randomization, battle/flip, standings, result, pacemaker, LR2/Beatoraja,
translation, and non-Arena regression contracts.

The translation release gate produced:

```text
English: 6/6 finished plural translations
Japanese: 732 source fallbacks; locale remains unadvertised
Polish: 732 generated, 729 finished, 3 pre-existing unfinished outside Arena
Arena Polish: 332/332 active identities, 0 unfinished
Arena English: 4 numerus-only identities, complete two-form translations
```

The translation contract compares context, source, comment, and numerus
identity. Polish and Japanese Arena source identities match exactly; English is
intentionally plural-only.

## Protocol non-drift

SHA-256 was computed after normalizing CRLF to LF. Client and server copies all
match:

| Fixture | SHA-256 |
| --- | --- |
| `protocol-v1.json` | `f8fdb269b8e0421db1a244563d78d2b6a2d001069c4fc0fa54139b19f93912ac` |
| `phase2-text-goldens.json` | `cb5d7d7e05245b1a80f2ca20951accc75013e3e5f52afba1d039ad3280647cf0` |
| `phase2-binary-goldens.json` | `ba4535da55a7a70383c72ae783b915d0f518aff87d1a0992adb9d391f44886a0` |
| `phase3-text-goldens.json` | `843c6e6c9d2c5e599119b988bc4c0edda9948167b6031426c4676454aec7820b` |

## Presentation and accessibility traceability

| Requirement | Evidence |
| --- | --- |
| Browser/login/create/join/password flow | Arena browser/session tests; keyboard and accessible list contracts |
| Native Default room surface | unscaled screen-space `ArenaSelectPanel`; roster, chat, selection, persistent ready reason, moderation, leave, and minimum-target tests |
| LR2/Beatoraja room surface | bounded `ArenaLegacySelectOverlay`; shared roster/chat/selection models |
| Per-player availability | atomic availability index and legacy/default unavailable-row tests; no `ChartData` mutation |
| Library rescan while in room | inventory generation/queue-drain/rescan tests |
| Song options | immutable selected-song configuration and protocol tests for randomization, battle, flip, and DP options |
| Forced gameplay standings | host ownership and forced-visible policy tests; complete 16-row QML model |
| Gameplay chat | closed-by-default, F8, Enter/Shift+Enter/Escape priority, unread and adjacent/clamped geometry tests |
| Existing-theme placement | normalized geometry, K5/K7/K10/K14/result separation, clamp, drag, eight grips, keyboard resize/reset, and persistence tests |
| Default result | unscaled screen-space panel, winner/local standing first, complete rows, Arena ranking-source default and provider cycling tests |
| LR2/Beatoraja result | movable application overlay plus unchanged number 179/180 compatibility tests |
| Accessible status | deduplicated reconnect, selection invalidation, authoritative launch-cancellation reasons, winner, remote DNF, and connection-transition tests |
| Keyboard focus | roving browser/roster/chat/gameplay/result lists, stable member-ID focus across live-standings resets/reorder, sticky chat review, visible focus, and keyboard grip tests |
| Remote text safety | `Text.PlainText` source contracts on room names, display names, chart metadata, and chat bodies |
| Local score/IR behavior | Arena final capture remains independent of IR submission; ordinary upload path is unchanged |
| Local battle/profile policy | battle is silently unavailable in Arena; active profile changes leave Arena and return to browser |

The Default result panel is mounted outside its 1920x1080 scaled skin root so
buttons retain screen-space target sizes at smaller resolutions. Result prose
uses the title font, numeric fields use the stats font, and translated text
enables context font merging.

## Review findings resolved

Independent integrated review findings were fixed before the release gate:

1. Final result standings now use one list-owned roving focus stop, so every
   off-screen row remains reachable without adding one Tab stop per delegate.
2. The shared result announcer waits for the complete final model and includes
   every remote DNF even after the gameplay surface has been destroyed.
3. `RoundLaunchCancelled.reason` is retained as bounded transient session
   status, mapped for every protocol enum value, announced once per launch
   attempt, and cleared on the next attempt or session cleanup.
4. Room and gameplay chat stop following the tail when keyboard navigation
   reviews an older row; End/Newest explicitly restores tail following.
5. The ready-disabled explanation stays visible while the Default room panel
   is on its chat tab.
6. The Default room panel is mounted outside the 1920x1080 design transform so
   its interactive controls retain at least 32 scene pixels at tested compact
   viewports.

Earlier review in the same phase also corrected final-snapshot connection
wording, retained live-standings focus by stable member ID across model resets,
removed deferred chat-row focus stealing after submit, returned focus to
gameplay after the first-use hint, and committed keyboard grip operations
through the existing theme-vars path.

## Manual and external gates

Automated tests instantiate the application-owned Default/legacy surfaces at
representative viewport sizes and exercise all layout variants. A live
two-instance visual matrix with real Default, LR2, and Beatoraja skins at every
UI/text scale was not executed in this noninteractive release pass. It remains
a recommended visual acceptance pass, particularly for third-party skin
occlusion and user-selected overlay placement.

Public deployment is not claimed. The current DNS check is:

```text
Resolve-DnsName arena.rhythmgame.eu
arena.rhythmgame.eu : DNS name does not exist.
```

This environment also has no Netcup/Coolify credentials and cannot discover the
real Traefik container-network CIDR. The production image, one-replica Coolify
runbook, exact proxy policy, anonymous WebSocket/container smokes, and container
verification are complete in RhythmGame-IR. DNS, TLS, Coolify deployment, the
real proxy CIDR, and public-origin WSS smoke remain operator actions; no
authentication, proxy, or WSS policy was weakened to simulate them.

## Implementation assumptions retained

- Official rooms are ephemeral and intentionally disappear on server restart.
- The official service uses one in-memory replica; one server is not one room.
- Arena admission requires the active IR login; unauthenticated users may browse
  the Arena area and use inline login UI but cannot join/create.
- Scores continue through the ordinary IR upload path independently of Arena.
- Any room member may select any commonly available chart at any time.
- Local battle cannot be enabled during Arena, and the profile menu is absent
  from Arena song select.
- Changing active profile leaves Arena and returns to the Arena browser.
