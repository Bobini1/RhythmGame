pragma Singleton

import QtQml

/*!
    \qmltype StandardInputKeys
    \inqmlmodule RhythmGameQml
    \brief Classifies input keys shared by standard skin components.

    This singleton centralizes key-range knowledge so individual screens do not
    duplicate it.
*/
QtObject {
    /*! Returns whether \a key is a standard player-one or player-two play key. */
    function isPlayKey(key): bool {
        return (key >= BmsKey.Col11 && key <= BmsKey.Col17)
            || (key >= BmsKey.Col21 && key <= BmsKey.Col27);
    }
}
