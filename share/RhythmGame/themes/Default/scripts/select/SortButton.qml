import RhythmGameQml
import QtQuick 2.0

Image {
    id: sortButton

    required property var generalVars
    readonly property int selectedMode: generalVars ? generalVars.selectSortMode : SelectSortMode.Title
    property var options: [
        { text: QT_TR_NOOP("Title"), value: SelectSortMode.Title },
        { text: QT_TR_NOOP("Artist"), value: SelectSortMode.Artist },
        { text: QT_TR_NOOP("BPM"), value: SelectSortMode.Bpm },
        { text: QT_TR_NOOP("Length"), value: SelectSortMode.Length },
        { text: QT_TR_NOOP("Clear"), value: SelectSortMode.ClearLamp },
        { text: QT_TR_NOOP("Score"), value: SelectSortMode.ScoreRate },
        { text: QT_TR_NOOP("Miss Count"), value: SelectSortMode.MissCount },
        { text: QT_TR_NOOP("Level"), value: SelectSortMode.Level },
        { text: QT_TR_NOOP("Total"), value: SelectSortMode.TotalNotes },
        { text: QT_TR_NOOP("Directory"), value: SelectSortMode.Directory }
    ]

    function indexForMode(mode) {
        for (let i = 0; i < options.length; ++i) {
            if (options[i].value === mode) {
                return i;
            }
        }
        return -1;
    }

    function textForMode(mode) {
        let index = indexForMode(mode);
        return index >= 0 ? options[index].text : QT_TR_NOOP("Title");
    }

    function cycle(delta) {
        let current = sortButton.indexForMode(sortButton.selectedMode);
        let step = delta === undefined ? 1 : delta;
        let next = current < 0 ? 0 : (current + step) % sortButton.options.length;
        if (next < 0) {
            next += sortButton.options.length;
        }
        sortButton.generalVars.selectSortMode = sortButton.options[next].value;
    }

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: qsTr("Sort: %1").arg(qsTr(sortButton.textForMode(sortButton.selectedMode)))
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: sortButton.cycle(1)
    }
}