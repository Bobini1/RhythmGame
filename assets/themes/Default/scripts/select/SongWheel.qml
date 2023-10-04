import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic

Pane {
    id: root

    readonly property bool active: StackView.status === StackView.Active
    readonly property string imagesUrl: rootUrl + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

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
        z: songList.count + 1
    }
    Image {
        anchors.left: parent.left

        anchors.right: parent.right
        anchors.rightMargin: 42
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 30
        source: iniImagesUrl + "folders.png/frame"
        z: pathView.count + 1
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
}