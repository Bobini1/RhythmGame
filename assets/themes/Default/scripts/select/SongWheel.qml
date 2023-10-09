import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia
import QtQml

Pane {
    id: root

    readonly property bool active: StackView.status === StackView.Active
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

    function getDiffColor(diff) {
        switch (diff) {
        case "beginner":
            return "green";
        case "normal":
            return "blue";
        case "hyper":
            return "orange";
        case "another":
            return "red";
        case "insane":
            return "purple";
        }
    }
    function getDiffColorInt(diff) {
        switch (diff) {
        case 1:
            return "green";
        case 2:
            return "blue";
        case 3:
            return "orange";
        case 4:
            return "red";
        case 5:
            return "purple";
        }
    }

    height: parent.height
    width: parent.width

    onActiveChanged: {
        if (active) {
            previewDelayTimer.restart();
            let currentChart = songList.currentItem.children[0];
            if (typeof currentChart.getClearType === 'function') {
                currentChart.clearType = currentChart.getClearType();
            }
        } else {
            playMusic.stop();
            previewDelayTimer.stop();
            playMusic.waitingForStop = false;
        }
    }


    Pane {
        focus: true
        anchors.centerIn: parent
        height: 1080
        scale: Math.min(parent.width / 1920, parent.height / 1080)
        width: 1920

        RowLayout {
            anchors.left: parent.left
            anchors.top: parent.top
            height: parent.height
            width: parent.width / 2

            StageFile {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.leftMargin: 80
                Layout.preferredHeight: 480
                Layout.preferredWidth: 640
                Layout.topMargin: 120
            }
            Banner {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.leftMargin: 80
                Layout.preferredHeight: 80
                Layout.preferredWidth: 300
                Layout.topMargin: 200
            }
        }
        Selector {
            anchors.right: parent.right
            anchors.rightMargin: 42
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -50
            currentItem: songList.current
            scrollingText: songList.scrollingText
            z: songList.count + 1
        }
        Image {
            anchors.left: parent.left
            anchors.leftMargin: -20
            anchors.top: parent.top
            height: 1080
            source: iniImagesUrl + "parts.png/containers"
            width: 1920
        }
        ScrollBar {
            id: vbar

            active: true
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.top: parent.top
            hoverEnabled: true
            orientation: Qt.Vertical
            size: songList.height
        }
        List {
            id: songList

            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: -40
            height: parent.height
            width: parent.width / 2
        }
        Shortcut {
            enabled: active
            sequence: "Esc"

            onActivated: {
                sceneStack.pop();
            }
        }
        MediaPlayer {
            id: playMusic

            property bool waitingForStop: false

            loops: MediaPlayer.Infinite
            source: songList.current instanceof ChartData ? PreviewFilePathFetcher.getPreviewFilePath(songList.current.directory) : ""

            audioOutput: AudioOutput {
                id: audioOutput

            }

            onSourceChanged: {
                playMusic.stop();
                previewDelayTimer.stop();
                waitingForStop = playMusic.source !== "";
            }
        }
        Difficulty {
            anchors.left: parent.left
            anchors.leftMargin: 80
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 170
            current: songList.current
            spacing: 30
        }
        Connections {
            function onMovingInAnyWayChanged() {
                if (playMusic.waitingForStop) {
                    previewDelayTimer.restart();
                }
            }

            target: songList
        }
        Timer {
            id: previewDelayTimer

            interval: 300

            onTriggered: {
                playMusic.play();
            }
        }
    }
}