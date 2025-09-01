import QtQml
import QtQuick
import RhythmGameQml

Rectangle {
    id: judgementCountsContainer

    color: "darkslategray"
    Repeater {
        id: judgementCountsModel

        model: side.score.judgementCounts
        delegate: Item {
            required property var judgement
            required property int count
        }
    }
    Column {
        Repeater {
            id: judgementCounts

            anchors.fill: parent
            anchors.margins: 8

            model: side.score.judgementCounts.rowCount()
            readonly property var judgements: ["Empty Poor", "Poor", "Bad", "Good", "Great", "Perfect"]

            delegate: Text {
                required property int index
                color: "white"
                fontSizeMode: Text.Fit
                textFormat: Text.PlainText
                text: {
                    let row = (judgementCountsModel.count, judgementCountsModel.itemAt(side.score.judgementCounts.rowCount() - index - 1));
                    // ignore judgements like MineHit
                    if (!row || row.judgement > Judgement.Perfect) {
                        return "";
                    }
                    return judgementCounts.judgements[row.judgement] + ": " + row.count;
                }
            }
        }
    }
    Connections {
        function onHit(tap) {
            if (!tap.points) {
                return;
            }
            let judgement = tap.points.judgement;
            if (judgement === Judgement.Poor || judgement === Judgement.Bad) {
                bga.poorVisible = true;
                poorLayerTimer.restart();
            }
        }

        target: side.score
    }
}