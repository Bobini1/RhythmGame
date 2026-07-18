# Arena Browsing Automatic Retry Design

## Goal

Prevent the Online Arena lobby browser from remaining indefinitely on
"Connecting to Arena..." when a WebSocket attempt stalls after TCP connection
or before the first room-directory snapshot arrives.

## Connection lifecycle

An anonymous browsing attempt has ten seconds to complete the entire readiness
path: transport connection, WebSocket upgrade, protocol hello, directory
subscription, and the first directory snapshot.

If the attempt times out, disconnects, or reports a transport error before the
directory becomes ready, the session closes that transport and schedules a new
anonymous attempt. Retry delays use exponential backoff of 0.5, 1, 2, 4, and 8
seconds, capped at 8 seconds. Retries continue while Arena remains active.

A received directory snapshot marks the browser ready, cancels the attempt
timeout, and resets the next retry delay to 0.5 seconds. Leaving Arena cancels
both the attempt timeout and any scheduled retry.

## Error behavior

Recoverable connection failures do not enter the terminal Error state and do
not require a manual Retry action. The browser remains in its connecting state
while the automatic retry loop is active.

Protocol errors, incompatible capabilities, and other non-transport failures
retain their existing terminal handling and explanatory UI. Authenticated room
admission and in-room reconnect behavior remain unchanged.

## Implementation boundary

The retry policy belongs to `ArenaSession`, which owns connection state and the
scheduler. `QtWebSocketArenaTransport` continues to report transport events and
does not own retry policy. The existing scheduler task IDs and backoff constants
may be reused where their lifetimes do not overlap, but browsing and in-room
reconnect state must remain explicitly separated.

No new QML controls or manual retry UI are introduced.

## Verification

Focused C++ state-machine coverage will prove that:

- a browsing attempt times out after ten seconds and schedules a retry;
- retry delays grow to the eight-second cap;
- a directory snapshot cancels the timeout and resets the backoff;
- leaving Arena cancels pending browsing work;
- protocol failures remain terminal rather than entering the retry loop.

No UI tests or tests that parse source files as text are added.
