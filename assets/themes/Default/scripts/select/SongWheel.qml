import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtMultimedia
import QtQml
import "../common/helpers.js" as Helpers
import "./playOptions"

FocusScope {
    focus: enabled

    Image {
        id: root

        readonly property bool active: parent.focus
        readonly property var bestStats: {
            let scores = songList.current instanceof ChartData && songList.currentItem ? songList.currentItem.children[0].scores : [];
            return Helpers.getBestStats(scores);
        }
        readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
        readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
        property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())
        property BmsResult scoreWithBestPoints: {
            let scores = songList.current instanceof ChartData && songList.currentItem ? songList.currentItem.children[0].scores : [];
            return Helpers.getScoreWithBestPoints(scores);
        }

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
                default:
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
                default:
                    return "purple";
            }
        }

        fillMode: Image.PreserveAspectCrop
        height: parent.height
        source: root.imagesUrl + "bg.png"
        width: parent.width

        onActiveChanged: {
            if (active) {
                previewDelayTimer.restart();
                let currentChart = songList.currentItem.children[0];
                if (typeof currentChart.refreshScores === 'function') {
                    currentChart.refreshScores();
                }
            } else {
                playMusic.stop();
                previewDelayTimer.stop();
                playMusic.waitingForStop = false;
            }
        }

        Item {
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
                anchors.rightMargin: 80
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -50
                currentItem: songList.current
                scrollingText: songList.scrollingText
                z: 2
            }
            Image {
                anchors.left: parent.left
                anchors.leftMargin: -20
                anchors.top: parent.top
                height: 1080
                source: root.iniImagesUrl + "parts.png/containers"
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
            Image {
                id: stageFileFrame

                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -22
                source: root.imagesUrl + "stageFileFrame.png"
            }
            List {
                id: songList

                anchors.right: parent.right
                anchors.rightMargin: 43
                anchors.top: parent.top
                anchors.topMargin: -40
                focus: true
                height: parent.height
                width: parent.width / 2
            }
            Shortcut {
                sequence: "Esc"
                enabled: root.enabled

                Component.onCompleted: {
                    print(parent)
                }

                onActivated: {
                    sceneStack.pop();
                }
                onActivatedAmbiguously: {
                    print("select", enabled);
                }
            }
            MediaPlayer {
                id: playMusic

                property bool waitingForStop: false

                loops: MediaPlayer.Infinite
                source: {
                    let base = songList.current instanceof ChartData ? PreviewFilePathFetcher.getPreviewFilePath(songList.current.chartDirectory) : ""
                    if (base === "") {
                        return base;
                    }
                    if (base[0] !== '/') {
                        base = '/' + base;
                    }
                    return "file://" + base;
                }

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
                anchors.verticalCenterOffset: 160
                current: songList.current
                spacing: 30
            }
            Loader {
                id: grade

                active: root.scoreWithBestPoints !== null
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: -10
                anchors.verticalCenterOffset: -10
                source: "Grade.qml"
            }
            Connections {
                function onMovingInAnyWayChanged() {
                    if (playMusic.waitingForStop) {
                        previewDelayTimer.restart();
                    }
                }

                function onOpenedFolder() {
                    previewDelayTimer.restart()
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
            ScoreInfo {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 80
                anchors.left: parent.left
                anchors.leftMargin: 80
                spacing: 40
            }
            Image {
                id: search

                anchors.bottom: parent.bottom
                anchors.bottomMargin: 5
                anchors.left: parent.left
                anchors.leftMargin: 48
                source: root.iniImagesUrl + "parts.png/search"

                TextEdit {
                    id: searchEdit

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 17
                    anchors.left: parent.left
                    anchors.leftMargin: 48
                    color: "white"
                    font.pixelSize: 20
                    height: 25
                    width: 540

                    Keys.onReturnPressed: {
                        songList.search(searchEdit.text);
                        searchEdit.text = "";
                        songList.focus = true;
                    }
                }
            }
            KeymodeButton {
                id: keymodeButton

                anchors.left: parent.left
                anchors.leftMargin: 900
                anchors.top: parent.top
                anchors.topMargin: 20
            }
            SortButton {
                anchors.left: keymodeButton.left
                anchors.top: keymodeButton.bottom
                anchors.topMargin: 20
            }
            MouseArea {
                id: focusList

                anchors.fill: parent
                onEnabledChanged: {
                    print("song wheel main enabled changed", enabled);
                }
                z: -1

                onClicked: {
                    songList.focus = true;
                }
            }
        }


        Rectangle {
            anchors.fill: parent

            color: {
                let c = Qt.color("black");
                c.a = 0.5;
                return c;
            }

            MouseArea {
                anchors.fill: parent
                enabled: parent.visible
                onClicked: (event) => {
                    playOptions.enabled = false;
                    login.enabled = false;
                }
                onWheel: (wheel) => {
                    wheel.accepted = true;
                }
            }

            visible: playOptions.enabled || login.enabled

            Item {
                anchors.centerIn: parent
                height: 1080
                scale: Math.min(parent.width / 1920, parent.height / 1080)
                width: 1920

                Loader {
                    id: playOptions
                    active: enabled
                    enabled: (Input.start1 || Input.start2) && !login.enabled
                    anchors.centerIn: parent

                    source: ProfileList.battleActive ? "playOptions/PlayOptionsBattle.qml" : "playOptions/PlayOptionsSingle.qml"
                }

                Login {
                    id: login

                    anchors.centerIn: parent
                    enabled: false
                }
                property bool startAndSelectPressed: Input.start1 && Input.select1 || Input.start2 && Input.select2

                onStartAndSelectPressedChanged: {
                    if (startAndSelectPressed) {
                        login.enabled = !login.enabled;
                    }
                }
            }
        }
    }
}