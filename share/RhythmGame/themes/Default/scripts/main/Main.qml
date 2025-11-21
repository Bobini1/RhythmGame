import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: screen
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)

    background: Image {
        source: imagesUrl + "RGBArtboard_2.svg"
    }

    Kirigami.Dialog {
        id: dlg
        modal: true
        standardButtons: Kirigami.Dialog.Ok
        width: Math.min(800, parent.width)
        height: Math.min(600, parent.height)
        anchors.centerIn: parent

        property string attrText: {
            const attr = new XMLHttpRequest();
            attr.open("GET", "qrc:/ATTRIBUTIONS.md", false);
            attr.send(null);
            attr.responseText;
        }

        property string licenseText: {
            const license = new XMLHttpRequest();
            license.open("GET", "qrc:/LICENSE.md", false);
            license.send(null);
            license.responseText;
        }

        contentItem: ScrollView {
            id: scrollView
            clip: true

            Column {
                Label {
                    id: licenseMd
                    text: dlg.licenseText
                    textFormat: Text.MarkdownText
                    wrapMode: Text.Wrap
                    padding: 12
                    width: scrollView.width
                    onLinkActivated: Qt.openUrlExternally(link)
                    color: palette.text
                }

                Label {
                    id: md
                    text: dlg.attrText
                    textFormat: Text.MarkdownText
                    wrapMode: Text.Wrap
                    padding: 12
                    width: scrollView.width
                    onLinkActivated: Qt.openUrlExternally(link)
                    color: palette.text
                }
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
            anchors.right: parent.right
            anchors.rightMargin: 20
            font.family: "Serif"
            font.pixelSize: 200
            text: "Rhythm Game"
            renderType: Text.CurveRendering
        }

        Column {
            id: column
            anchors.top: parent.top
            anchors.topMargin: parent.height * 0.5
            anchors.horizontalCenter: titleText.horizontalCenter

            width: parent.width * 0.4

            spacing: 5

            Input.onStart1Pressed: {
                globalRoot.pageStack.pushItem(globalRoot.selectComponent);
            }
            Input.onStart2Pressed: {
                globalRoot.pageStack.pushItem(globalRoot.selectComponent);
            }

            Button {
                width: parent.width
                height: 100
                font.pixelSize: 30

                text: qsTr("Song Selection")
                onClicked: {
                    globalRoot.pageStack.pushItem(globalRoot.selectComponent);
                }
            }

            Button {
                width: parent.width
                height: 100
                text: qsTr("Settings")
                font.pixelSize: 30
                onClicked: {
                    globalRoot.pageStack.pushItem(globalRoot.settingsComponent);
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
