# How theme behavior is organized {#theme_architecture}

Themes choose standard behavior separately for each screen. A menu can use
`StandardMainActions`, a selector can use `StandardSelectController`, and
gameplay can use `StandardGameplayFlow`. Each component handles common
behavior while the theme draws its own screen.

The game keeps screen creation, navigation and local play object lifetime in
`ContentFrame`. A theme calls named operations through `globalRoot` when it
needs to navigate. It does not manage the underlying screen stack.

`ContentFrame` loads the screen configured for each role. It does not try other
roles or call theme methods by name. Standard components handle actions such as
folder reloads and Arena score submission. An Arena result receives its round
ID in the result context before the screen is created.

Arena panels also belong to the theme. The optional `StandardArenaSelectOverlay`,
`StandardArenaGameplayOverlay` and `StandardArenaResultOverlay` provide room
controls, standings and chat for their respective screens. Each overlay can
replace its `panelComponent` while keeping placement and chat behavior.
Gameplay and result contexts report `arenaActive`, so targets and input rules
work without an overlay. The host does not infer whether a theme has its own
Arena presentation.

`StandardMultiplayerFlow` connects the browser and opens the room selector or
scheduled gameplay. All screens use the same stack. The selector's standard
actions leave the room and return to the retained browser, waiting if another
screen currently covers selection. Removing the browser ends the Arena session.
No shell or nested stack is needed.
The multiplayer flow releases each prepared Arena runner when its gameplay
screen leaves the stack. The host only pushes or replaces screens and keeps
local runners alive for them. Decide calls `replaceGameplay`; Arena calls
`openGameplay`, which always pushes.
LR2 has internal entry files for each screen role. They configure its shared
renderer, which reads the selected CSV and settings itself. LR2 also creates
its customization editor there and derives legacy result fields from the
supplied context. ContentFrame uses the same screen inputs for LR2 and QML themes.

The [tutorial](theme-tutorial/index.md) teaches one complete component per
screen before introducing optional helpers. Each lesson has a separate theme
folder that can be installed without the others.

## What the design needs to support

A new author should be able to replace a menu without implementing song
selection, gameplay or settings. Players must also be able to mix screens
from different themes. Shared behavior therefore needs to work without
depending on Default's files or another screen's private properties.

Authors should have control over appearance, animation and input presentation.
At the same time, a change to course progression or retry should usually be
fixed in the shared component, without requiring every theme to copy the fix.
The interface needs to make custom behavior possible without making it the
starting point for beginners.

The number of properties alone doesn't tell us how easy an interface is.
An author also needs to know when to call methods, which objects stay alive
and what happens when a screen fails to load. The standard flow should handle
those rules wherever the rules are common to themes.

## Alternatives considered

| Approach                                                                    | Benefit                                                                   | Cost                                                                                             | Decision                              |
|-----------------------------------------------------------------------------|---------------------------------------------------------------------------|--------------------------------------------------------------------------------------------------|---------------------------------------|
| The host automatically installs all standard behavior                       | Very little code in a new theme                                            | Hidden input handlers can conflict with custom ones, and the host must infer each screen's needs | Keep behavior explicit in each screen |
| Every screen derives from a standard visual root                            | Data and behavior are available from one object                           | It constrains the root type or adds another wrapper around the same behavior                     | Keep visual roots free for now        |
| Authors assemble only small behavior components                             | Custom selectors can choose every part                                    | Beginners must understand overlapping input, history and completion rules                        | Keep this option for custom flows     |
| Each screen has a complete standard component, with smaller parts available | Common screens need little wiring, while custom behavior remains possible | The reference must distinguish included parts from optional additions                            | Use this approach                     |

Automatic behavior would hide too much in the host. A manifest flag or global
`StandardTheme` object would have to find the current role, data and settings.
A handler added by the theme could then duplicate an invisible handler. Adding
a standard feature could also change a custom theme without an explicit choice
in that screen's source.

A required visual root would make simple examples shorter, but it would limit
authors who want a `Rectangle`, `FocusScope` or their own component as the root.
An optional wrapper avoids that restriction but adds another public type.
`StandardGameplayFlow` already needs only a context binding inside the root,
so a wrapper currently removes little work. Reconsider one if repeated theme
code shows a concrete benefit.

Small components are useful for a selector with custom browsing or input.
They are harder to teach as the default because authors must know which
parts overlap and how their state changes fit together. The complete
controller includes those parts, while the smaller components remain
available for authors who need to replace one responsibility.

## Use a complete component for each screen

| Screen                 | Starting component         | What the theme supplies                                           |
|------------------------|----------------------------|------------------------------------------------------------------|
| Main menu              | `StandardMainActions`      | Buttons, labels and images                                       |
| Multiplayer browser    | `StandardMultiplayerFlow`  | Room list, join and create controls                              |
| Song selection         | `StandardSelectController` | List or wheel, focus movement and optional preview or ranking UI |
| Decide                 | `StandardDecideFlow`       | Chart or course details and optional buttons                     |
| Gameplay               | `StandardGameplayFlow`     | Notes, BGA, scores and animation timing                          |
| Chart or course result | `StandardResultInput`      | Score display and optional confirmation buttons                  |

Start by changing the visuals or a specific option. Replace an action when
the standard operation no longer fits. A complete controller already creates
its included components, so adding those components beside it can handle
the same input twice.

`StandardSelectController` supports both Default's circular view and the
tutorial's finite list. The controller tracks browsing state, while the theme
chooses how movement wraps or animates. `StandardSelectReload` keeps the
selection reload rules with selection behavior instead of in the host.

`StandardGameplayFlow` handles startup, retry, stage results and course
continuation. `startReady` and `finishReady` let a theme delay transitions for
animations while keeping standard saving and navigation. `stageActivated`
and `closing` let it update or clean up its presentation.

## Supply data for the screen's role

Decide and gameplay receive `GameplayContext`. It gives both single charts
and courses the same current-chart interface. The context remains the same
when decide is replaced by gameplay, and its chart and players update together
when a course installs the next stage.

Every gameplay theme supports courses. Current-stage data stays separate from
course details and course combo state, so drawing notes doesn't require
different code for a course runner. The context exposes existing objects
rather than copying changing scores and positions into a JavaScript map.

Normal results receive `ResultContext`, including after a course stage.
Course summaries receive `CourseResultContext`. A result participant pairs
its profile with the appropriate saved score type. The result's chart and
stage fields describe the completed stage, even after the originating
gameplay context has advanced.

The game chooses Result and Course Result independently. It does not use a
normal result screen as a fallback for a course summary. Default's shared
private result view is a choice within that theme, not a requirement of the API.

## Keep lifetime and failure handling in the game

`ContentFrame` creates a replacement screen before removing the old screen.
If creation fails, the current screen remains available. The host also keeps
local play objects alive from decide through gameplay and results, then
releases them when they are no longer needed.

The standard gameplay flow captures completed-stage data before advancing a
course. It waits for the preceding result to close before starting the next
stage. Saving happens once, and retry, exit and natural completion cannot
start competing transitions.

If a result screen fails to load, the flow keeps the saved result for another
presentation attempt. `retryTransition()` tries to open the screen again
without saving or advancing the course again. The internal `GameplaySession`
holds those rules separately from input and audio so they can be tested.
It reads the runner, stage and play mode from one `GameplayContext`; callers
do not supply separate copies of that state.
Theme authors do not need to assemble it themselves.

Inactive screens cannot start gameplay or navigate through the standard flow.
The theme still stops its own audio and disables its own shortcuts. Arena
controls its own startup and play object lifetime, and standard quick retry
is unavailable there.

## Keep LR2 separate from the public contract

LR2 is an internal adapter. It may reuse a Standard component, call an
internal helper or implement its own behavior. Shared code is useful where
the behavior fits, but LR2's needs don't require a new public theme property.

Add public options for uses that ordinary QML themes need. When sharing code
would make that interface harder to use, let the LR2 adapter use a different
implementation. Check LR2 separately when shared behavior changes.

## Maintain the interfaces together

Prefer a specific option or notification over replacing an entire action.
Document every replacement's responsibility. A `try...Action` callback
returns true when it handled a request, while false lets the standard action
continue. A cleanup callback can run alongside the standard action instead
of replacing it. The property reference must say which contract applies.

Update the theme guide, generated reference and installable examples when
changing a public interface. Check mixed themes as well as Default. The
design reduces the code a theme needs to maintain, but it is not a promise
that the API will never change.
