pragma Singleton
import QtQml
import RhythmGameQml

/*!
    \qmltype StandardSelectOptions
    \inqmlmodule RhythmGameQml
    \brief Provides optional translated labels for sorting and replay choices.

    Each option has a stable \c value and a translated \c text. Use the value
    when changing a setting. Labels and list positions are not identifiers.
    The labels update when the game language changes.

    StandardSelectState exposes these choices through its display properties.
    You can also use this singleton directly, for example in Settings. It does
    not read or change profile settings, sort entries, or choose a replay.
    Themes can use their own text, models or artwork instead.

    To customize a label, format it in the theme's binding or build a separate
    model. Compare the stable value, not its position in the list. For example,
    a ComboBox can use this model with \c textRole set to \c text and
    \c valueRole set to \c value:
    \code
    model: selection.replayOptions.map(option => ({
        value: option.value,
        text: option.value === StandardSelectState.BestCombo
            ? qsTr("Longest combo") : option.text
    }))
    \endcode
*/
QtObject {
    id: root

    /*!
        All sort choices as \c {value, text} objects in the default cycle order.
        Total sorts by the chart's TOTAL gauge value, not its note count.
    */
    readonly property var sortOptions: [
        { value: SelectSortMode.Title, text: qsTr("Title") },
        { value: SelectSortMode.Artist, text: qsTr("Artist") },
        { value: SelectSortMode.Bpm, text: qsTr("BPM") },
        { value: SelectSortMode.Length, text: qsTr("Length") },
        { value: SelectSortMode.ClearLamp, text: qsTr("Clear") },
        { value: SelectSortMode.ScoreRate, text: qsTr("Score") },
        { value: SelectSortMode.MissCount, text: qsTr("Miss count") },
        { value: SelectSortMode.Level, text: qsTr("Level") },
        { value: SelectSortMode.Total, text: qsTr("Total") },
        { value: SelectSortMode.Directory, text: qsTr("Directory") }
    ]
    /*! Replay choices as \c {value, text} objects. */
    readonly property var replayOptions: [
        { value: 0, text: qsTr("Newest") },
        { value: 1, text: qsTr("Best score") },
        { value: 2, text: qsTr("Best clear") },
        { value: 3, text: qsTr("Best combo") }
    ]

    /*! Returns the display label for \a mode, or an empty string for an unknown value. */
    function sortText(mode: int): string {
        return root.sortOptions.find(option => option.value === mode)?.text ?? "";
    }

    /*! Returns the display label for \a type, or an empty string for an unknown value. */
    function replayText(type: int): string {
        return root.replayOptions.find(option => option.value === type)?.text ?? "";
    }
}
