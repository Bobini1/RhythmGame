import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import RhythmGameQml
import "../common"

Image {
    id: screen
    readonly property string imagesUrl: Qt.resolvedUrl(".") + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: QmlUtils.fileName.slice(0, QmlUtils.fileName.lastIndexOf("/") + 1)
    readonly property var themeVars: (Rg.profileList.mainProfile.vars.themeVars.main || {})[QmlUtils.themeName] || ({})

    ThemeFont {
        id: mainTitleFont
        fileName: screen.themeVars.mainTitleFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    ThemeFont {
        id: mainMenuFont
        fileName: screen.themeVars.mainMenuFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    ThemeFont {
        id: mainDialogFont
        fileName: screen.themeVars.mainDialogFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    source: imagesUrl + "RGBArtboard_2"

    Image {
        anchors.fill: parent
        source: imagesUrl + "flower-back0716"
        fillMode: Image.PreserveAspectCrop
        z: -2
        mirror: true

        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: 0.75
        }
    }

    AudioPlayer {
        id: bgm
        looping: true
        source: Rg.profileList.mainProfile.vars.generalVars.bgmPath + "main";
        playing: screen.enabled
        fadeInMillis: 1000
    }

    Dialog {
        id: dlg
        modal: true
        standardButtons: Dialog.Ok
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
                    font.family: mainDialogFont.fontFamily
                    font.weight: mainDialogFont.fontWeight
                    font.italic: mainDialogFont.italic
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
                    font.family: mainDialogFont.fontFamily
                    font.weight: mainDialogFont.fontWeight
                    font.italic: mainDialogFont.italic
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
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: 280
            font.family: mainTitleFont.fontFamily
            font.weight: mainTitleFont.fontWeight
            font.italic: mainTitleFont.italic
            font.pixelSize: 200
            text: "RhythmGame"
        }

        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#202020"
            elide: Text.ElideRight
            font.family: mainMenuFont.fontFamily
            font.weight: mainMenuFont.fontWeight
            font.italic: mainMenuFont.italic
            font.pixelSize: 28
            fontSizeMode: Text.HorizontalFit
            horizontalAlignment: Text.AlignRight
            maximumLineCount: 1
            minimumPixelSize: 12
            text: qsTr("%1 %2").arg(Qt.application.name).arg(Qt.application.version)
            width: 500
        }

        Pane {
            anchors.top: parent.top
            anchors.topMargin: parent.height * 0.5
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: 280

            width: parent.width * 0.4
            Column {
                id: column
                anchors.fill: parent

                spacing: 5

                Input.onStart1Pressed: {
                    globalRoot.openSelect();
                }
                Input.onStart2Pressed: {
                    globalRoot.openSelect();
                }

                Button {
                    width: parent.width
                    height: 100
                    font.family: mainMenuFont.fontFamily
                    font.weight: mainMenuFont.fontWeight
                    font.italic: mainMenuFont.italic
                    font.pixelSize: 30

                    text: qsTr("Song Selection")
                    onClicked: {
                        globalRoot.openSelect();
                    }
                }

                Button {
                    width: parent.width
                    height: 100
                    text: qsTr("Settings")
                    font.family: mainMenuFont.fontFamily
                    font.weight: mainMenuFont.fontWeight
                    font.italic: mainMenuFont.italic
                    font.pixelSize: 30
                    onClicked: {
                        globalRoot.openSettings();
                    }
                }

                Button {
                    width: parent.width
                    height: 100
                    text: qsTr("Attributions")
                    font.family: mainMenuFont.fontFamily
                    font.weight: mainMenuFont.fontWeight
                    font.italic: mainMenuFont.italic
                    font.pixelSize: 30
                    onClicked: {
                        dlg.open();
                    }
                }

                Button {
                    width: parent.width
                    height: 100
                    text: qsTr("Quit")
                    font.family: mainMenuFont.fontFamily
                    font.weight: mainMenuFont.fontWeight
                    font.italic: mainMenuFont.italic
                    font.pixelSize: 30
                    onClicked: {
                        Qt.quit();
                    }
                }
            }
        }
    }
}
