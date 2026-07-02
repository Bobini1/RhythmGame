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
    font.variableAxes: settingsUiFont.variableAxes
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

        ToolButton {
            id: backButton
            text: "‹"
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.boldFontWeight
            font.variableAxes: settingsHeaderFont.boldVariableAxes
            font.italic: settingsHeaderFont.italic
            font.pixelSize: 28
            ToolTip.text: qsTr("Back")
            ToolTip.visible: hovered
            ToolTip.delay: 500
            Layout.alignment: Qt.AlignLeft | Qt.AlignTop
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            palette.buttonText: hovered ? settings.palette.brightText : settings.palette.windowText
            background: Rectangle {
                radius: 4
                color: backButton.hovered ? settings.palette.accent : settings.palette.window
                border.color: settings.palette.mid
                border.width: 1
            }
            onClicked: {
                sceneStack.pop();
            }
        }
        TabBar {
            id: tabView
            Layout.fillWidth: true
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.fontWeight
            font.variableAxes: settingsHeaderFont.variableAxes
            font.italic: settingsHeaderFont.italic
            palette.buttonText: settings.palette.windowText

            TabButton {
                id: tabButton
                text: qsTr("Player settings")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: tabButton.checked ? settings.palette.accent : (tabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: tabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
            TabButton {
                id: songDirectoriesTabButton
                text: qsTr("Song directories")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: songDirectoriesTabButton.checked ? settings.palette.accent : (songDirectoriesTabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: songDirectoriesTabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
            TabButton {
                id: tablesTabButton
                text: qsTr("Tables")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: tablesTabButton.checked ? settings.palette.accent : (tablesTabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: tablesTabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
            TabButton {
                id: themesTabButton
                text: qsTr("Themes")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: themesTabButton.checked ? settings.palette.accent : (themesTabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: themesTabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
            TabButton {
                id: generalSettingsTabButton
                text: qsTr("General Settings")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: generalSettingsTabButton.checked ? settings.palette.accent : (generalSettingsTabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: generalSettingsTabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
            TabButton {
                id: keyConfigTabButton
                text: qsTr("Key config")
                palette.buttonText: checked ? settings.palette.brightText : settings.palette.windowText
                font.weight: checked ? settingsHeaderFont.boldFontWeight : settingsHeaderFont.fontWeight
                background: Rectangle {
                    color: keyConfigTabButton.checked ? settings.palette.accent : (keyConfigTabButton.hovered ? settings.palette.midlight : settings.palette.window)
                    border.color: keyConfigTabButton.checked ? settings.palette.accent : settings.palette.mid
                    border.width: 1
                }
            }
        }
    }
    Item {
        anchors.fill: parent
        anchors.margins: 16

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
                tabTextColor: settings.palette.windowText
            }
            GeneralSettings {
            }
            KeySettings {
            }
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
