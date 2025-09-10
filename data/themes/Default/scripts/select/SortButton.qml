import RhythmGameQml
import QtQuick 2.0
import "../common/helpers.js" as Helpers

Image {
    id: sortButton

    property int current: {
        let lowercaseSorts = sortButton.options.map(function(option) { return option.toLowerCase(); });
        let selectedSort = themeVars.sortOrder.toLowerCase();
        let index = lowercaseSorts.indexOf(selectedSort);
        return index === -1 ? 0 : index;
    }
    property var options: [QT_TR_NOOP("Title"), QT_TR_NOOP("Artist"), QT_TR_NOOP("BPM"), QT_TR_NOOP("Clear"), QT_TR_NOOP("Score"), QT_TR_NOOP("Level"), QT_TR_NOOP("Total")]
    required property var themeVars

    Binding {
        delayed: true
        target: sortButton.themeVars
        property: "sortOrder"
        value: sortButton.options[sortButton.current].toLowerCase();
    }
    Binding {
        sortButton.current: {
            let lowercaseSorts = sortButton.options.map(function(option) { return option.toLowerCase(); });
            let selectedSort = sortButton.themeVars.sortOrder.toLowerCase();
            let index = lowercaseSorts.indexOf(selectedSort);
            return index === -1 ? 0 : index;
        }
    }

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: qsTr("Sort: %1").arg(qsTr(sortButton.options[sortButton.current]))
    }
    MouseArea {
        function compareByTitle(a, b) {
            let res = a.title.localeCompare(b.title);
            if (res !== 0) {
                return res;
            }
            return a.subtitle.localeCompare(b.subtitle);
        }

        function setSort() {
            let currentSort = sortButton.options[sortButton.current];
            switch (currentSort) {
            case "Title":
                songList.sort = null;
                break;
            case "Artist":
                songList.sort = function (a, b) {
                    let res = a.artist.localeCompare(b.artist);
                    if (res !== 0) {
                        return res;
                    }
                    res = a.subartist.localeCompare(b.subartist);
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case "BPM":
                songList.sort = function (a, b) {
                    let res = a.initialBpm - b.initialBpm;
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case "Clear":
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
            case "Score":
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
            case "Level":
                songList.sort = function (a, b) {
                    let res = a.playLevel - b.playLevel;
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
                break;
            case "Total":
                songList.sort = function (a, b) {
                    let res = a.total - b.total;
                    if (res !== 0) {
                        return res;
                    }
                    return compareByTitle(a, b);
                };
            }
        }

        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor

        Component.onCompleted: {
            setSort();
        }
        onClicked: {
            sortButton.current = (sortButton.current + 1) % sortButton.options.length;
            setSort();
        }
    }
}
