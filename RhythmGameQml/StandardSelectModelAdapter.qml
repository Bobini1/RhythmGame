import QtQuick

/*!
    \qmltype StandardSelectModelAdapter
    \inqmlmodule RhythmGameQml
    \brief Repeats logical entries for circular-list presentation.

    This optional adapter supplies enough rows to fill every visible slot while
    preserving the source as a logical, unique list.
*/
QtObject {
    id: root

    /*! Logical, unique entries to adapt. */
    property var source: []
    /*! Minimum number of entries required by the presentation. */
    property int minimumCount: 0
    /*! Adapted entries, repeated to a multiple of \l source when necessary. */
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
