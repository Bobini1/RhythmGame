pragma Singleton

import QtQml

/*!
    \qmltype StandardInputKeys
    \inqmlmodule RhythmGameQml
    \brief Classifies input keys shared by standard skin components.

    This singleton centralizes key-range knowledge so individual screens do not
    duplicate it.

    A play key is Col11 through Col17 or Col21 through Col27, inclusive. Start,
    Select, scratch-direction, keyboard, and function keys are not included.
*/
QtObject {
    /*! Returns whether \a key is a standard player-one or player-two play key. */
    function isPlayKey(key): bool {
        return (key >= BmsKey.Col11 && key <= BmsKey.Col17)
            || (key >= BmsKey.Col21 && key <= BmsKey.Col27);
    }
}
