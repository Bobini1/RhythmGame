import QtQuick
import RhythmGameQml
import QtQuick.Layouts

Image {
    id: root
    focus: true
    required property var chart
    property bool transitionRequested: false
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
        title = title.replace(/\r\n|\n|\r/g, " ");
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

    function decidePlayKey(key) {
        switch (key) {
        case BmsKey.Col11:
        case BmsKey.Col12:
        case BmsKey.Col13:
        case BmsKey.Col14:
        case BmsKey.Col15:
        case BmsKey.Col16:
        case BmsKey.Col17:
        case BmsKey.Col21:
        case BmsKey.Col22:
        case BmsKey.Col23:
        case BmsKey.Col24:
        case BmsKey.Col25:
        case BmsKey.Col26:
        case BmsKey.Col27:
            return true;
        default:
            return false;
        }
    }

    function decideStartSelectKeyCombo(key) {
        return (key === BmsKey.Start1 && Input.select1)
            || (key === BmsKey.Select1 && Input.start1)
            || (key === BmsKey.Start2 && Input.select2)
            || (key === BmsKey.Select2 && Input.start2);
    }

    function startGameplay() {
        if (transitionRequested || !enabled) {
            return;
        }
        transitionRequested = true;
        globalRoot.openGameplay(root.chart);
    }

    function cancelDecide() {
        if (transitionRequested || !enabled) {
            return;
        }
        transitionRequested = true;
        sceneStack.pop();
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
        interval: 5000;
        running: root.enabled && !root.transitionRequested
        onTriggered: root.startGameplay()
    }

    Shortcut {
        sequence: "Esc"
        enabled: root.enabled

        onActivated: root.cancelDecide()
    }

    Shortcut {
        sequence: "Return"
        enabled: root.enabled

        onActivated: root.startGameplay()
    }

    Shortcut {
        sequence: "Enter"
        enabled: root.enabled

        onActivated: root.startGameplay()
    }

    Input.onButtonPressed: (key) => {
        if (root.decideStartSelectKeyCombo(key)) {
            root.cancelDecide();
        } else if (root.decidePlayKey(key)) {
            root.startGameplay();
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onPressed: (mouse) => {
            mouse.accepted = true;
            if (mouse.button === Qt.LeftButton) {
                root.startGameplay();
            } else if (mouse.button === Qt.RightButton) {
                root.cancelDecide();
            }
        }
    }
}
