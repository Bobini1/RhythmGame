import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Item {
    id: playArea

    property double blackWidth: 48.0 / totalWidthAbs * parent.width
    property list<int> columns: []
    property double redWidth: 108.0 / totalWidthAbs * parent.width
    readonly property int spacing: 2
    readonly property int totalWidthAbs: 3 * 48 + 4 * 60 + 108 + spacing * 6
    property double whiteWidth: 60.0 / totalWidthAbs * parent.width

    anchors.bottom: parent.bottom
    anchors.top: parent.top
    width: totalWidthAbs

    Playfield {
        id: playfield

        border.color: "black"
        border.width: 10
        columns: playArea.columns
        spacing: playArea.spacing
        y: chart.position * root.greenNumber
    }
    Row {
        id: laserRow

        function shootLaser(index) {
            laserRowChildren.itemAt(playArea.columns[index]).start();
        }

        anchors.bottom: parent.bottom
        height: parent.height
        spacing: playArea.spacing
        width: parent.width

        Repeater {
            id: laserRowChildren

            model: playArea.columns.length

            // laser beam (animated)
            LaserBeam {
                columnIndex: playArea.columns[index]
                image: root.laserImages[index]
            }
        }
    }
    Connections {
        function onHit(tap) {
            laserRow.shootLaser(tap.column);
        }

        target: chart.score
    }
}

