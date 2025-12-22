import QtQuick
import RhythmGameQml
import QtQuick.Layouts

Image {
    id: root
    required property var chart
    readonly property int difficulty: {
        let diff = chart.chartData?.difficulty;
        if (diff === undefined) {
            return 0;
        }
        return diff;
    }
    readonly property string titleString: {
        let title = chart.chartData?.title;
        if (title === undefined) {
            return chart.course.name;
        }
        if (chart.chartData?.subtitle !== "") {
            title += " " + chart.chartData.subtitle;
        }
        return title;
    }
    readonly property string genreString: {
        let genre = chart.chartData?.genre;
        if (genre === undefined) {
            return "Course";
        }
        return genre;
    }
    readonly property string artistString: {
        let artist = chart.chartData?.artist;
        if (artist === undefined) {
            return "";
        }
        return artist;
    }
    readonly property string subartistString: {
        let subartist = chart.chartData?.subartist;
        if (subartist === undefined) {
            return "";
        }
        return subartist;
    }
    source: "images/bg.png"

    readonly property string diffColor: {
        switch (root.difficulty) {
            case 1:
                return "#89CC89";
            case 2:
                return "#89CCCC";
            case 3:
                return "#CCA46C";
            case 4:
                return "#CC6868";
            case 5:
                return "#CC6699";
            default:
                return "#808080";
        }
    }

    onEnabledChanged: {
        if (enabled) Qt.callLater(() => sceneStack.pop());
    }

    // stops all sounds when leaving the screen
    Component.onDestruction: {
        chart.destroy();
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        y: parent.height / 2
        spacing: titleRect.height / 9

        Rectangle {
            id: genreRect
            width: parent.width
            height: width / 45
            color: root.diffColor
            opacity: 0.8
            property real pos: 1;
            x: parent.width * pos

            Item {
                width: parent.width * 2 / 3
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                    margins: height / 20
                }
                Text {
                    text: root.genreString
                    font.pixelSize: 200
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    minimumPixelSize: 1
                    anchors {
                        fill: parent
                    }
                }
            }

            NumberAnimation {
                id: genreAnim
                target: genreRect
                property: "pos"
                to: 0
                duration: 1000
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            id: titleRect
            width: parent.width
            height: width / 30
            color: root.diffColor
            opacity: 0.8
            property real pos: 1;
            x: parent.width * pos

            Item {
                width: parent.width * 2 / 3
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                    margins: height / 20
                }
                Text {
                    text: root.titleString
                    font.pixelSize: 200
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    minimumPixelSize: 1
                    anchors {
                        fill: parent
                    }
                }
            }

            SequentialAnimation {
                id: titleAnim

                PauseAnimation { duration: 200 }
                NumberAnimation {
                    target: titleRect
                    property: "pos"
                    to: 0
                    duration: 800
                    easing.type: Easing.OutCubic
                }
            }
        }

        Rectangle {
            id: artistRect
            width: parent.width
            height: width / 45
            color: root.diffColor
            opacity: 0.8
            property real pos: 1;
            x: parent.width * pos

            RowLayout {
                width: parent.width * 2 / 3
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                    margins: artistRect.height / 20
                }

                Text {
                    text: root.subartistString
                    font.pixelSize: 200
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: implicitWidth === 0 ? 0 : parent.width / 2
                    minimumPixelSize: 1
                }
                Text {
                    text: root.artistString
                    font.pixelSize: 200
                    color: "white"
                    fontSizeMode: Text.VerticalFit
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: implicitWidth === 0 ? 0 : parent.width / 2
                    minimumPixelSize: 1
                }
            }

            SequentialAnimation {
                id: artistAnim
                PauseAnimation { duration: 400 }
                NumberAnimation {
                    target: artistRect
                    property: "pos"
                    to: 0
                    duration: 600
                    easing.type: Easing.OutCubic
                }
            }
        }
    }

    Component.onCompleted: {
        stagefileAnim.start();
        genreAnim.start();
        titleAnim.start();
        artistAnim.start();
    }

    Image {
        id: stagefile
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: pos * parent.width
        width: parent.width / 3 - parent.width / 24
        height: width * (3/4)
        asynchronous: true
        property real pos: - 1 / 3 + 1 / 24
        NumberAnimation {
            id: stagefileAnim
            target: stagefile
            property: "pos"
            to: 1 / 24
            duration: 1000
            easing.type: Easing.OutCubic
        }
        source: {
            let dir = chart.chartData?.chartDirectory;
            print(dir, chart.chartData);
            if (dir === undefined || !chart.chartData?.stageFile) {
                return "";
            }
            if (dir[0] !== "/") {
                dir = "/" + dir;
            }
            let stageFileWithoutExt = chart.chartData.stageFile.replace(/\.[^/.]+$/, "");
            return "file://" + dir + stageFileWithoutExt;
        }

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.5
            z: -1
        }
    }

    AudioPlayer {
        id: decideSound
        source: Rg.profileList.mainProfile.vars.generalVars.bgmPath + "decide";
        playing: true
    }

    Timer {
        interval: Math.max(3000, Math.min(decideSound.length, 8000));
        running: true
        onTriggered: {
            globalRoot.openGameplay(root.chart);
        }
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.enabled

        onActivated: {
            sceneStack.pop();
        }
    }
}