import QtQuick 2.15
import RhythmGameQml
import QtQuick.Controls.Basic 2.15
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: screen
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)

    source: imagesUrl + "RGBArtboard_2.svg"

    ColumnLayout {
        id: column
        anchors.top: parent.top
        anchors.topMargin: parent.height * 0.55
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: parent.width * 0.11

        width: parent.width * 0.4

        Button {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 100
            font.pixelSize: 30

            text: qsTr("Song Selection")
            onClicked: {
                sceneStack.pushItem(globalRoot.songWheelComponent);
            }
        }

        Button {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 100
            text: qsTr("Settings")
            font.pixelSize: 30
            onClicked: {
                sceneStack.pushItem(globalRoot.settingsComponent);
            }
        }

        Button {
            Layout.preferredWidth: parent.width
            Layout.preferredHeight: 100
            text: qsTr("Quit")
            font.pixelSize: 30
            onClicked: {
                Qt.quit();
            }
        }

    }

}
