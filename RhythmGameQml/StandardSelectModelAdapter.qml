import QtQuick

/*!
    \qmltype StandardSelectModelAdapter
    \inqmlmodule RhythmGameQml
    \brief Repeats entries to fill the visible rows of a circular selector.

    Supply a list in \l source and the number of rows needed in \l minimumCount.
    If the source is already long enough, \l entries contains it unchanged.
    Otherwise, complete copies of the list are repeated until the minimum is met.
    An empty source remains empty.

    Repeated entries still refer to the same items. The adapter doesn't clone
    objects or track focus. A normal list can use StandardSelectState::entries
    directly without this adapter.
*/
QtObject {
    id: root

    /*! Supplies the source list, with one entry per item. */
    property var source: []
    /*! Sets the minimum number of rows needed by the view. */
    property int minimumCount: 0
    /*! Contains the source entries, repeated in complete groups when needed. */
    readonly property var entries: {
        if (!source || source.length === 0 || source.length >= minimumCount) {
            return source || [];
        }
        let result = source.slice();
        let limit = Math.ceil(minimumCount / source.length) * source.length;
        for (let i = source.length; i < limit; ++i) {
            result.push(source[i % source.length]);
        }
        return result;
    }
}
