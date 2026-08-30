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
    readonly property var entries: repeatToMinimumCount(source, minimumCount)

    /*! Returns \a input repeated to satisfy \a count without truncation. */
    function repeatToMinimumCount(input, count) {
        if (!input || input.length === 0 || input.length >= count) {
            return input || [];
        }
        let result = input.slice();
        let limit = Math.ceil(count / input.length) * input.length;
        for (let i = input.length; i < limit; ++i) {
            result.push(input[i % input.length]);
        }
        return result;
    }
}
