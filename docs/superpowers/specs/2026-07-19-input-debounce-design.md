# Per-Key Input Debounce Design

## Goal

Prevent switch chatter from producing duplicate hits or leaving a physically
held key logically released, while keeping press latency at zero and maintaining
independent state for every `BmsKey`.

## Chosen behavior

Digital presses are accepted immediately. A digital release is held pending for
`debounceMs`; if the same key is pressed again before that key's deadline, only
that pending release is canceled. If the deadline expires while the key remains
physically released, one logical release is emitted with the original input
timestamp.

This collapses both `down-up-down` press chatter and `up-down-up` release
chatter without manufacturing a second hit. A debounce value of zero keeps
presses and releases immediate.

## State and scheduling

`InputTranslator` owns one state record and one precise, single-shot release
timer per `BmsKey`. Each state record distinguishes the latest physical state,
the public debounced state, and an optional pending release timestamp. No key
shares a timer, pending timestamp, or transition state with another key.

Changing the debounce setting to zero commits pending releases immediately.
Changing it to another positive value restarts pending per-key deadlines using
the new interval. The C++ property reset value is 5 ms, matching the runtime and
Default-theme defaults.

Synthetic releases used by analog scratch direction changes, scratch timeout,
and key unbinding remain immediate. Delaying those transitions would allow
opposite scratch directions to overlap even though they are not mechanical
button chatter.

## Signal and timestamp behavior

The existing `buttonPressed`, `buttonReleased`, per-button property-change, and
tick signals remain the public interface. Pending releases keep the logical
button pressed and its tick timer running. A committed release stops the tick
timer and emits the existing release signals once. Although delivery is delayed
by the debounce interval, the emitted release carries its original hardware
timestamp so gameplay judgment does not acquire timing offset.

## Regression coverage

Public-path tests cover press chatter, release chatter, a stable release,
zero-debounce behavior, two keys with independent pending transitions, and the
5 ms property reset. Tests create a Qt core event loop so the production timers
are exercised rather than replacing the scheduler with test-only logic.
