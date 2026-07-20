# Arena skin score-target compatibility design

## Scope

Expose the strongest valid Arena opponent as the live score target in the
Default, LR2, and Beatoraja gameplay presentations. This change does not alter
Arena selection, standings, result-screen semantics, protocol messages, or the
persisted offline target setting.

## Compatibility contract

- Arena is an ordinary score-target provider. It is not LR2 ghost battle, a
  saved rival record, or Internet Ranking connectivity.
- Preserve the selected ghost display option (`34` through `37`). Keep LR2
  ghost-battle option `623` and rival-comparison option `625` inactive. Do not
  repurpose online option `51`.
- LR2 target string `1` and Beatoraja target/rival strings `1` and `3` expose
  the selected Arena opponent's display name during Arena gameplay.
- Existing target numbers and bargraphs consume the selected opponent's latest
  exact EX score. Current and final target values intentionally coincide while
  that opponent is active and become final once the opponent finishes.
- Beatoraja timer `352` is active while a valid target exists and the local EX
  score is at least that target's EX score. It turns off if a newer snapshot
  moves the target ahead again.

## Delay behavior

Before the first valid standings snapshot, the Arena target name is empty and
the target score is zero. Target-dependent Default-theme presentation remains
hidden during this interval. Skin option topology remains stable throughout
gameplay; availability never enables ghost battle or rival comparison.

Each accepted `ArenaOpponentTarget::changed` notification refreshes cached LR2
numbers, target strings, target bargraphs, and the Beatoraja target timer. The
score is never interpolated or projected between snapshots. Leader changes are
applied atomically, and the existing retained-tie behavior prevents needless
identity switching at equal scores.

## Components

- `Lr2SkinScreenWrapper.qml` owns the Arena-aware target text, cached-renderer
  refresh, and Beatoraja timer transition logic.
- `Gameplay.qml`, `Side.qml`, and `PlayArea.qml` retain the Default theme's
  existing Arena target-score override while carrying target availability to
  the ghost-score presentation.
- `ArenaOpponentTarget` remains the sole selector for the strongest valid
  opponent; no additional ranking logic is introduced in QML.

## Verification

- Lint every changed QML file with the configured Qt import paths.
- Build the Default and LR2 QML targets.
- Run the focused Arena opponent-target tests and the full available CTest
  suite.
- Confirm the final diff keeps options `623`, `625`, and `51` unchanged and
  contains no unrelated worktree changes.
