import RhythmGameQml
import QtQuick 2.0

Image {
    id: sortButton

    property int current: 0
    property var options: ["Title", "Artist", "BPM", "Clear", "Score", "Level", "Total"]

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: "Sort: " + sortButton.options[sortButton.current]
    }
    MouseArea {
        function setSort() {
            let currentSort = sortButton.options[sortButton.current];
            switch (currentSort) {
            case "Title":
                songList.sort = null;
                break;
            case "Artist":
                songList.sort = function (a, b) {
                    return a.artist.localeCompare(b.artist);
                };
                break;
            case "BPM":
                songList.sort = function (a, b) {
                    return a.initialBpm - b.initialBpm;
                };
                break;
            case "Clear":
                songList.sort = function (a, b) {
                    let scores1 = ScoreDb.getScoresForChart(a.sha256);
                    let scores2 = ScoreDb.getScoresForChart(b.sha256);
                    let clearType1 = root.getClearType(scores1);
                    let clearType2 = root.getClearType(scores2);
                    return root.clearTypePriorities.indexOf(clearType2) - root.clearTypePriorities.indexOf(clearType1);
                };
                break;
            case "Score":
                songList.sort = function (a, b) {
                    let scores1 = ScoreDb.getScoresForChart(a.sha256);
                    let scores2 = ScoreDb.getScoresForChart(b.sha256);
                    let score1 = root.getScoreWithBestPoints(scores1);
                    let score2 = root.getScoreWithBestPoints(scores2);
                    if (!score1 && !score2) {
                        return 0;
                    }
                    if (!score1) {
                        return 1;
                    }
                    if (!score2) {
                        return -1;
                    }
                    return (score2.points / score2.maxPoints) - (score1.points / score1.maxPoints);
                };
                break;
            case "Level":
                songList.sort = function (a, b) {
                    return a.playLevel - b.playLevel;
                };
                break;
            case "Total":
                songList.sort = function (a, b) {
                    return a.total - b.total;
                };
            default:
                console.log("Unknown sort type: " + currentSort);
            }
        }

        anchors.fill: parent

        Component.onCompleted: {
            setSort();
        }
        onClicked: {
            sortButton.current = (sortButton.current + 1) % sortButton.options.length;
            setSort();
        }
    }
}
