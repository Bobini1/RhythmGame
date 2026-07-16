# Themeable Multiplayer Screen Design

## Goal

Make the online multiplayer room browser a normal theme screen so theme authors can replace its presentation just as they can replace the main, select, or settings screens. This change does not redesign the Default theme's current multiplayer browser.

## Screen identity

The theme screen key is `multiplayer`. The Default implementation lives at `scripts/multiplayer/Multiplayer.qml`; its visible title remains **Online Arena**. The broader name leaves room for future online modes without renaming the screen contract. Existing Arena backend types retain their current names.

`multiplayer` is independently selectable in Theme Settings. Existing profiles receive the Default implementation automatically through the existing theme-configuration defaulting mechanism. A theme appears as a choice for this screen only when its manifest provides a `multiplayer` script.

## Ownership boundary

`RhythmGameQml` continues to own application and protocol behavior:

- `ArenaSession` and its models;
- Arena connection, authentication, room admission, and error state;
- the application shell that enters and exits multiplayer and switches between the browser and song select;
- reusable, presentation-neutral Arena controls that are also needed by overlays or other screens.

The selected theme owns the multiplayer browser presentation:

- room list and empty state;
- login prompt;
- create-room and password dialogs;
- room member previews;
- labels, spacing, fonts, colors, and other visual choices;
- user interaction that invokes the exposed session operations.

The current `RhythmGameQml/Arena/ArenaBrowser.qml` implementation moves into the Default theme. Components used only by that browser move with it. Components shared with gameplay, result, select overlays, or application-level accessibility remain in `RhythmGameQml`.

## Screen contract and navigation

`ContentFrame` resolves the configured `multiplayer` script and creates it inside the existing Arena shell. The screen receives the active `ArenaSession` and profile explicitly. It exposes an exit request to the shell; the shell remains responsible for stack navigation and session teardown.

Room creation, joining, retrying, and login continue to use the existing public session/profile APIs. Entering a room still causes the shell to hide the browser and open the configured select screen. Leaving a room restores the same multiplayer screen instance and its focus without reconnecting solely because the presentation changed.

A malformed or unloadable configured screen must fail visibly and return safely to the previous application screen rather than leaving an active, invisible Arena session.

## Theme and translation integration

The Default `theme.json` adds its `multiplayer` script. Theme Settings adds Multiplayer to its stable screen order and gives it a translated display name.

User-visible strings owned by the moved browser belong to the Default theme translation catalogs. Strings from reusable core Arena components remain in the core catalog. Translation extraction must not leave duplicate active entries for the removed core screen.

## Compatibility and scope

Third-party themes need no changes: profiles default to the Default multiplayer screen until another installed theme supplies one. LR2 and Beatoraja skins remain select/gameplay/result skins and do not implicitly become multiplayer browser implementations.

This work does not change the Arena protocol, room behavior, lobby layout, gameplay overlays, or result overlays.

## Verification

- Run QML linting and build the QML resources with the Default multiplayer script loaded from disk.
- Verify theme scanning exposes `multiplayer` and an existing profile defaults it to the Default family.
- Regenerate and validate the Default theme translations.
- Smoke-test entering multiplayer, login/create/join dialogs, entering select, returning to the room browser, and exiting Arena.
- Do not add source-string parsing tests or UI layout tests.
