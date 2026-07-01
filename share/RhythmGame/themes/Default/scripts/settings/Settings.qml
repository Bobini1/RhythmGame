import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import RhythmGameQml
import "../common"

Page {
    id: settings
    property int initialTabIndex: 0
    readonly property var themeVars: (Rg.profileList.mainProfile.vars.themeVars.settings || {})[QmlUtils.themeName] || ({})

    ThemeFont {
        id: settingsUiFont
        fileName: settings.themeVars.settingsUiFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    ThemeFont {
        id: settingsHeaderFont
        fileName: settings.themeVars.settingsHeaderFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    font.family: settingsUiFont.fontFamily
    font.weight: settingsUiFont.fontWeight
    font.italic: settingsUiFont.italic

    function applyInitialTabIndex() {
        tabView.currentIndex = Math.max(0, Math.min(tabView.count - 1, initialTabIndex));
    }

    Component.onCompleted: applyInitialTabIndex()
    onInitialTabIndexChanged: applyInitialTabIndex()
    
    header: RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: backButton.height
        spacing: 1

        Button {
            id: backButton
            text: "⏎"
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.boldFontWeight
            font.italic: settingsHeaderFont.italic
            palette {
                button: settings.palette.accent
                buttonText: settings.palette.brightText
            }
            font.pixelSize: 20
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.preferredWidth: tabButton.height
            Layout.preferredHeight: tabButton.height
            onClicked: {
                sceneStack.pop();
            }
        }
        TabBar {
            id: tabView
            Layout.fillWidth: true
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.fontWeight
            font.italic: settingsHeaderFont.italic
            palette.buttonText: palette.windowText

            TabButton {
                id: tabButton
                text: qsTr("Player settings")
            }
            TabButton {
                text: qsTr("Song directories")
            }
            TabButton {
                text: qsTr("Tables")
            }
            TabButton {
                text: qsTr("Themes")
            }
            TabButton {
                text: qsTr("General Settings")
            }
            TabButton {
                text: qsTr("Key config")
            }
        }
    }
    StackLayout {
        id: stackView
        anchors.fill: parent

        currentIndex: tabView.currentIndex

        PlayerSettings {
        }
        SongFolderSettings {
        }
        TableSettings {
        }
        ThemeSettings {
        }
        GeneralSettings {
        }
        KeySettings {
        }
    }
    Shortcut {
        enabled: settings.enabled
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
}
