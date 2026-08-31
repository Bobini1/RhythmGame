import QtQuick

/*!
    \qmltype PendingReplyGroup
    \inqmlmodule RhythmGameQml
    \brief Owns and cancels a group of asynchronous replies.

    Use separate instances for operations with independent cancellation
    lifetimes. Tracked replies are forgotten when they finish and are cancelled
    when \l cancelAll is called or the group is destroyed.
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
