import QtQuick

/*!
    \qmltype StandardSelectModelAdapter
    \inqmlmodule RhythmGameQml
    \brief Repeats entries to fill the visible rows of a circular selector.

    Supply a list in \l source and the number of rows needed in \l minimumCount.
    If the source is already long enough, \l entries contains it unchanged.
    Otherwise, complete copies of the list are repeated until the minimum is met.
    An empty source produces null placeholders, with at least one row.
    These placeholders belong only to the presentation model.

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
    /*! Contains repeated source entries, or null placeholders for an empty source. */
    readonly property var entries: {
        const items = root.source && root.source.length > 0 ? root.source : [null];
        if (items.length >= root.minimumCount) {
            return items;
        }
        let result = items.slice();
        let limit = Math.ceil(root.minimumCount / items.length) * items.length;
        for (let i = items.length; i < limit; ++i) {
            result.push(items[i % items.length]);
        }
        return result;
    }
}
