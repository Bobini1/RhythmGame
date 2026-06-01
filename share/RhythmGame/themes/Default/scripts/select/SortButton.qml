import RhythmGameQml
import QtQuick 2.0
import "../common/helpers.js" as Helpers

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

    onSelectedModeChanged: mouseArea.setSort()

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: qsTr("Sort: %1").arg(qsTr(sortButton.textForMode(sortButton.selectedMode)))
    }
    MouseArea {
        id: mouseArea

        function compareByTitle(a, b) {
            let res = (a.title || "").localeCompare(b.title || "");
            if (res !== 0) {
                return res;
            }
            return (a.subtitle || "").localeCompare(b.subtitle || "");
        }

        function compareNumberWithMissing(aValue, bValue) {
            let aMissing = aValue === null || aValue === undefined || !isFinite(aValue);
            let bMissing = bValue === null || bValue === undefined || !isFinite(bValue);
            if (aMissing && bMissing) {
                return 0;
            }
            if (aMissing) {
                return 1;
            }
            if (bMissing) {
                return -1;
            }
            return aValue - bValue;
        }

        function setSort() {
            switch (sortButton.selectedMode) {
            case SelectSortMode.Directory:
                songList.sort = null;
                break;
            case SelectSortMode.Title:
                songList.sort = compareByTitle;
                break;
            case SelectSortMode.Artist:
                songList.sort = function (a, b) {
                    let res = (a.artist || "").localeCompare(b.artist || "");
                    if (res !== 0) {
                        return res;
                    }
                    res = (a.subartist || "").localeCompare(b.subartist || "");
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.Bpm:
                songList.sort = function (a, b) {
                    let res = compareNumberWithMissing(a.initialBpm, b.initialBpm);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.Length:
                songList.sort = function (a, b) {
                    let res = compareNumberWithMissing(a.length, b.length);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.ClearLamp:
                songList.sort = function (a, b) {
                    let scores1 = songList.scores[a.md5] || [];
                    let scores2 = songList.scores[b.md5] || [];
                    let clearType1 = Helpers.getClearType(scores1);
                    let clearType2 = Helpers.getClearType(scores2);
                    let res = Helpers.clearTypePriorities.indexOf(clearType2) - Helpers.clearTypePriorities.indexOf(clearType1);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.ScoreRate:
                songList.sort = function (a, b) {
                    let scores1 = songList.scores[a.md5] || [];
                    let scores2 = songList.scores[b.md5] || [];
                    let score1 = Helpers.getScoreWithBestPoints(scores1);
                    let score2 = Helpers.getScoreWithBestPoints(scores2);
                    if (!score1 && !score2) {
                        return 0;
                    }
                    if (!score1) {
                        return 1;
                    }
                    if (!score2) {
                        return -1;
                    }
                    let res = (score2.result.points / score2.result.maxPoints) - (score1.result.points / score1.result.maxPoints);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.MissCount:
                songList.sort = function (a, b) {
                    let stats1 = Helpers.getBestStats(songList.scores[a.md5] || []);
                    let stats2 = Helpers.getBestStats(songList.scores[b.md5] || []);
                    let res = compareNumberWithMissing(stats1 ? stats1.missCount : null, stats2 ? stats2.missCount : null);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.Level:
                songList.sort = function (a, b) {
                    let res = compareNumberWithMissing(a.playLevel, b.playLevel);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case SelectSortMode.TotalNotes:
                songList.sort = function (a, b) {
                    let res = compareNumberWithMissing(a.total, b.total);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            default:
                songList.sort = compareByTitle;
            }
        }

        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor

        Component.onCompleted: {
            setSort();
        }
        onClicked: {
            let current = sortButton.indexForMode(sortButton.selectedMode);
            let next = current < 0 ? 0 : (current + 1) % sortButton.options.length;
            sortButton.generalVars.selectSortMode = sortButton.options[next].value;
            setSort();
        }
    }
}
