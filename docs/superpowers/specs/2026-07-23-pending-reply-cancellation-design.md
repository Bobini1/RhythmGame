# Pending Reply Cancellation Design

## Context

RhythmGame depends on Qt Interface Framework only for
`QIfPendingReply<T>`. The dependency is otherwise unused, but it adds a Qt
module to CMake, vcpkg, the Windows overlay ports, and the Nix package and
development-shell definitions.

The current asynchronous APIs are exposed by `ScoreDb` and `OnlineScores`.
QML consumes their replies through `then(success, failed)` and, in one place,
the `valid`, `resultAvailable`, and `success` properties. `ScoreDb` also
provides `cancelPending()`, which makes the task-producing object responsible
for cancelling every outstanding database query. That is the ownership model
this change must remove.

## Goals

- Remove every build and packaging dependency on Qt Interface Framework.
- Preserve the QML behavior used by existing callers:
  `then(success, failed)`, `valid`, `resultAvailable`, `success`, and `value`.
- Put cancellation on the returned reply with `reply.cancel()`.
- Make the QML component that owns an asynchronous request retain and cancel
  that request instead of calling a cancellation method on `ScoreDb`.
- Abort underlying network work when Qt provides an abort operation.
- Cooperatively stop or discard database and parsing work that cannot be
  interrupted safely.
- Guarantee that success, failure, and cancellation are terminal and that only
  the first terminal operation wins.
- Keep all implementation on public Qt APIs.

## Non-goals

- Promise chaining, result transformation, progress reporting, or multiple
  results.
- A public distinction between failure and cancellation. No current QML caller
  needs it.
- Forcefully interrupting a SQLite statement that is already executing.
- Refactoring unrelated cancellation APIs such as `OnlineRankingModel`.

## Public QML API

Add a non-creatable, QML-visible `PendingReply` object with these members:

```cpp
Q_PROPERTY(bool valid READ isValid CONSTANT)
Q_PROPERTY(bool resultAvailable READ isResultAvailable NOTIFY finished)
Q_PROPERTY(bool success READ isSuccessful NOTIFY finished)
Q_PROPERTY(QVariant value READ value NOTIFY finished)

Q_INVOKABLE void then(const QJSValue& success,
                      const QJSValue& failed = QJSValue());
Q_INVOKABLE void cancel();

Q_SIGNAL void finished();
```

All returned instances are valid. A new reply starts with
`resultAvailable == false` and `success == false`.

`then()` accepts the same two callbacks used today. Once the reply succeeds,
the success callback receives the converted result. Once the reply fails or is
cancelled, the failure callback runs without an argument. Registering callbacks
after the reply has reached a terminal state invokes the matching callback
immediately. Calling `then()` again replaces the previously registered
callbacks, matching `QIfPendingReply`.

`cancel()` is idempotent. On the first call while pending, it:

1. marks the reply terminal;
2. requests cooperative cancellation from its producer;
3. invokes the currently installed underlying cancellation handler;
4. exposes `resultAvailable == true` and `success == false`;
5. invokes the failure callback and emits `finished()`.

Calls to `cancel()`, success, or failure after a terminal result are ignored.
There is no public `cancelled` property because no current call site branches
on that distinction.

## C++ Producer API

Keep result typing on the producer side with a copyable
`PendingReplySource<T>`. Constructing a source with its producer creates the
QML handle, parents that handle to the producer while pending, and owns their
shared operation state. The source exposes:

```cpp
PendingReply* reply() const;
bool succeed(const T& value);
bool fail();
std::stop_token stopToken() const;
void setCancellationHandler(std::function<void()> handler);
```

The source and the QML handle represent the same operation. Asynchronous
callbacks capture the source by value. `succeed()` and `fail()` return whether
they won the terminal-state transition, which lets producers clean up values
that lose a race with cancellation.

Reply completion and cancellation-handler replacement happen on the
application thread. Worker code uses the thread-safe stop token and posts its
completion back to the application thread, matching the existing
`ScoreDb`/`OnlineScores` execution model. Installing a cancellation handler
after cancellation has already won invokes that handler immediately; this
closes the race between stages of a multi-request operation.

The implementation uses `qjsEngine(reply)` and
`QJSEngine::toScriptValue()` to invoke JavaScript callbacks without Qt private
headers.

## Lifetime Model

While pending, the reply is parented to its producer (`ScoreDb` or
`OnlineScores`). This prevents JavaScript garbage collection from destroying a
temporary returned object before its asynchronous producer finishes.

After reaching a terminal state, the reply detaches from the producer and is
assigned JavaScript ownership. QML may retain it for property inspection or
allow it to be garbage-collected. Destroying the producer while a reply is
pending requests cancellation and makes all later producer completions no-ops.

All `QJSValue` callbacks are stored, invoked, and destroyed on the application
thread.

## ScoreDb Migration

Every asynchronous `ScoreDb` method returns `PendingReply*` and creates a
matching `PendingReplySource<T>`.

Each worker captures the source and its stop token. It checks cancellation
before starting expensive work and before posting a result. A SQLite statement
already in progress may run to completion, but its result is discarded after
cancellation.

Remove `ScoreDb::cancelPending()` and the shared `std::stop_source`. Cancellation
must not be addressed to the database object.

The two QML owners that currently call `ScoreDb.cancelPending()` will instead
maintain their own active-reply collections:

- `RhythmGameQml/Lr2/Lr2SelectContext.qml`
- `share/RhythmGame/themes/Default/scripts/select/List.qml`

Each component:

1. records every `PendingReply` it creates for its refresh operation;
2. does not record a reply that is already terminal, and otherwise removes it
   through the reply's `finished` signal;
3. calls `cancel()` on its still-pending replies before starting a replacement
   refresh;
4. calls `cancel()` on its still-pending replies during component destruction.

This makes cancellation local to the UI work that owns the request. One
component can no longer cancel unrelated queries started by another component.

## OnlineScores Migration

`OnlineScores` methods return `PendingReply*`.

For a live `QNetworkReply`, the source's cancellation handler calls
`QNetworkReply::abort()`. The multi-stage Tachi lookup replaces the handler as
ownership moves from the chart-resolution request to the score request. If
cancellation wins between stages, the next stage is not started.

The background score parser checks the source stop token before expensive
conversion and before publishing the `BmsScore`. If cancellation wins after a
score object has been allocated, the completion path schedules that object for
deletion instead of leaking it.

## Error Handling

- Invalid input completes as failed before the method returns.
- Network and parsing errors log at their current severity and fail the reply.
- Cancellation-triggered `QNetworkReply::OperationCanceledError` does not
  produce an error log or a success result.
- A second terminal operation is ignored without invoking callbacks twice.
- A non-callable JavaScript callback produces a QML warning and is not stored.
- A JavaScript exception raised by a callback is left on the associated
  `QJSEngine` for the normal QML error-reporting path.

## Dependency Cleanup

- Remove `InterfaceFramework` from the Qt component search and
  `Qt6::InterfaceFramework` from target linkage.
- Remove `qtinterfaceframework` from `vcpkg.json`.
- Delete `vcpkgOverlayPortsWindows/qtinterfaceframework`.
- Remove the custom `qtinterfaceframework` package construction and arguments
  from `flake.nix`.
- Remove it from the Nix development shell and RhythmGame package inputs.
- Delete `nix/packages/qtinterfaceframework.nix`.
- Update source comments that name `QIfPendingReply`.

Historical design and plan documents remain unchanged.

## Testing

Add focused C++ tests for:

- initial properties;
- immediate and asynchronous success;
- immediate and asynchronous failure;
- JavaScript result conversion for a registered gadget and a `QObject*`;
- callback registration after completion;
- cancellation invoking the failure callback;
- cancellation handler invocation exactly once;
- repeated cancellation;
- success/failure after cancellation;
- cancellation after success/failure;
- producer destruction while pending;
- safe cleanup of a late pointer result.

Add QML integration coverage for returning `PendingReply*`, calling `then()`,
and calling `cancel()` on the returned object.

Verify the affected target and full test executable, then configure the project
with no Qt Interface Framework package available. Finally, search tracked source
and manifests to ensure no live dependency reference remains.
