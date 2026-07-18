# LR2 Unavailable Song Marker Design

## Goal

Replace the long, localized native-LR2 prefixes for unavailable songs with one compact, language-independent marker that fits legacy skin title fields and is representable by their Shift-JIS fonts.

## Marker and Scope

Use `× ` (U+00D7 MULTIPLICATION SIGN followed by one ASCII space) for every native LR2 song title that represents either of these conditions:

- a local chart that is unavailable to at least one Arena member;
- a chart referenced by a table or course but missing locally.

The marker is not translated. Its CP932/Shift-JIS representation is `81 7E`, and the inspected ht51 title fonts contain the glyph.

Apply the marker consistently to LR2 bar display text, title roles, selected-entry text, value-resolver title strings, and missing course-stage titles. Preserve the original chart title after the marker.

Beatoraja semantics do not change: Arena-unavailable charts continue to use Beatoraja body/title types without a text prefix, and missing course stages retain Beatoraja's `(no song) ` prefix. The Default theme and other non-LR2 screens do not change.

## Implementation Shape

Expose one native-LR2 unavailable-song prefix from `Lr2SelectContext.qml` and pass it through the existing select-model boundary. Use that same value for both missing-entry and Arena-unavailable decoration in `Lr2SelectItemModel`, and for native-LR2 missing title generation in `Lr2SkinValueResolver`.

Keep a C++ default equal to `× ` so direct model users and tests receive the same marker before any QML binding is established. Rename the existing `arenaUnavailablePrefix` model property and its backing API to `unavailableSongPrefix` throughout the internal LR2 implementation. Do not add a second marker setting or a user-facing preference.

Remove the obsolete `(arena unavailable) ` and `(missing) ` translation messages when refreshing the translation catalogs. Do not add replacement symbol entries to the catalogs.

## Verification

Add or update focused LR2 model coverage proving that native LR2 produces exactly `× Song` for both a missing table entry and an Arena-unavailable chart. Cover the native-LR2 resolver paths for missing table entries and missing course stages, while retaining the existing Beatoraja `(no song) ` expectation.

Run the focused LR2 tests, QML lint/tests that cover the changed files, translation regeneration, and a build of the affected targets. Confirm that no active translation source still contains either legacy prefix and that unrelated translation entries remain unchanged.
