import QtQuick

/*!
    \qmltype PendingReplyGroup
    \inqmlmodule RhythmGameQml
    \brief Cancels a set of asynchronous requests together.

    Use one group for requests that should be cancelled together, such as data
    for the currently selected chart. Call \l cancelAll before loading a new
    selection. Finished replies leave the group automatically; destroying the
    group cancels any unfinished replies.

    \l track returns the same reply, so the caller can still use its
    \c then() method. This helper requests a full score belonging to the main
    profile and cancels the previous request:

    \qml
    import QtQuick
    import RhythmGameQml

    Item {
        PendingReplyGroup { id: scoreReplies }

        function requestScore(result) {
            scoreReplies.cancelAll();
            return scoreReplies.track(
                Rg.profileList.mainProfile.scoreDb.getScore(result));
        }
    }
    \endqml

    For an ordinary score display, use ScoreDetails instead. It also tracks
    the selected result, profile and loading state.

    A reply must provide a \c resultAvailable property, a \c finished signal
    and a \c cancel() method. Null and already-finished replies are returned
    unchanged and are not retained. Disabling the group does not cancel its
    requests; call \l cancelAll when your custom flow needs that behavior.
*/
Item {
    id: root

    QtObject {
        id: storage

        property var replies: []
    }

    /*! Number of unfinished replies currently owned by the group. */
    readonly property int count: storage.replies.length

    /*! Retains \a reply until it finishes or the group cancels it. */
    function track(reply) {
        if (!reply || reply.resultAvailable) {
            return reply;
        }
        storage.replies.push(reply);
        storage.replies = storage.replies.slice();
        let forget = function() {
            reply.finished.disconnect(forget);
            let index = storage.replies.indexOf(reply);
            if (index >= 0) {
                storage.replies.splice(index, 1);
                storage.replies = storage.replies.slice();
            }
        };
        reply.finished.connect(forget);
        return reply;
    }

    /*! Cancels and releases every unfinished reply owned by the group. */
    function cancelAll() {
        let replies = storage.replies;
        storage.replies = [];
        for (let reply of replies) {
            if (reply && !reply.resultAvailable) {
                reply.cancel();
            }
        }
    }

    Component.onDestruction: cancelAll()
}
