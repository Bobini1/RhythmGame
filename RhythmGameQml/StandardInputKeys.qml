pragma Singleton

import QtQml

/*!
    \qmltype StandardInputKeys
    \inqmlmodule RhythmGameQml
    \brief Identifies bound lane keys used by the standard screen controls.

    Call \l isPlayKey when a custom handler needs the same check as decide or
    result input. The singleton recognizes Col11 through Col17 and Col21 through
    Col27. It excludes Start, Select, scratch directions and ordinary keyboard
    or function keys.
*/
QtObject {
    /*! Returns whether \a key is a standard player-one or player-two play key. */
    function isPlayKey(key): bool {
        return (key >= BmsKey.Col11 && key <= BmsKey.Col17)
            || (key >= BmsKey.Col21 && key <= BmsKey.Col27);
    }
}
