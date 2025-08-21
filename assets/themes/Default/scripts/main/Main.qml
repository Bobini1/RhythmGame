import QtQuick 2.15
import QtQuick.Controls.Basic 2.15
import QtQuick.Layouts
import RhythmGameQml

Image {
    id: screen
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)

    source: imagesUrl + "RGBArtboard_2.svg"

    Dialog {
        id: dlg
        modal: true
        standardButtons: Dialog.Ok
        width: 800
        height: 600

        property string mdText: {
            const xhr = new XMLHttpRequest();
            xhr.open("GET", "qrc:/ATTRIBUTIONS.md", false);
            xhr.send(null);
            return xhr.responseText
        }

        contentItem: ScrollView {
            anchors.fill: parent
            clip: true

            Text {
                id: md
                text: dlg.mdText
                textFormat: Text.MarkdownText
                wrapMode: Text.Wrap
                padding: 12
                width: parent.width
                onLinkActivated: Qt.openUrlExternally(link)
            }
        }
    }


    Item {
        id: scaledRoot
        anchors.centerIn: parent

        width: 1920
        height: 1080
        scale: Math.min(screen.width / width, screen.height / height)
        transformOrigin: Item.Center

        Text {
            id: titleText
            anchors.top: parent.top
            anchors.topMargin: 160
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: 280
            font.family: "Serif"
            font.pixelSize: 200
            text: "Rhythm Game"
        }

        Column {
            id: column
            anchors.top: parent.top
            anchors.topMargin: parent.height * 0.50
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: 280

            width: parent.width * 0.4

            spacing: 5

            Button {
                width: parent.width
                height: 100
                font.pixelSize: 30

                text: qsTr("Song Selection")
                onClicked: {
                    sceneStack.pushItem(globalRoot.songWheelComponent);
                }
            }

            Button {
                width: parent.width
                height: 100
                text: qsTr("Settings")
                font.pixelSize: 30
                onClicked: {
                    sceneStack.pushItem(globalRoot.settingsComponent);
                }
            }

            Button {
                width: parent.width
                height: 100
                text: qsTr("Attributions")
                font.pixelSize: 30
                onClicked: {
                    dlg.open();
                }
            }

            Button {
                width: parent.width
                height: 100
                text: qsTr("Quit")
                font.pixelSize: 30
                onClicked: {
                    Qt.quit();
                }
            }

        }
    }
}
