import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import RhythmGameQml
import "../common"
import "SettingsColors.js" as SettingsColors

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

    function tabTextColor(checked, enabled) {
        if (!enabled) {
            return SettingsColors.alpha(settings.palette.windowText, 0.35);
        }
        return checked ? SettingsColors.contrastText(settings.palette.highlight) : settings.palette.windowText;
    }

    function tabFillColor(checked, hovered, down) {
        if (checked) {
            if (down) {
                return SettingsColors.blend(settings.palette.shadow, settings.palette.highlight, 0.18);
            }
            if (hovered) {
                return SettingsColors.blend(settings.palette.light, settings.palette.highlight, 0.12);
            }
            return settings.palette.highlight;
        }
        if (down) {
            return SettingsColors.blend(settings.palette.mid, settings.palette.window, SettingsColors.isLight(settings.palette.window) ? 0.22 : 0.32);
        }
        if (hovered) {
            return SettingsColors.blend(settings.palette.button, settings.palette.window, SettingsColors.isLight(settings.palette.window) ? 0.62 : 0.42);
        }
        return SettingsColors.alpha(settings.palette.window, 0);
    }

    component SettingsTabButton: TabButton {
        id: settingsTabButton

        required property var settingsPage
        required property var headerFont

        implicitHeight: 40
        width: Math.max(136, contentItem.implicitWidth + leftPadding + rightPadding)
        leftPadding: 18
        rightPadding: 18
        topPadding: 8
        bottomPadding: 8
        font.family: headerFont.fontFamily
        font.weight: checked ? headerFont.boldFontWeight : headerFont.fontWeight
        font.variableAxes: checked ? headerFont.boldVariableAxes : headerFont.variableAxes
        font.italic: headerFont.italic

        contentItem: Label {
            text: settingsTabButton.text
            font: settingsTabButton.font
            color: settingsPage.tabTextColor(settingsTabButton.checked, settingsTabButton.enabled)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        background: Rectangle {
            radius: 6
            color: settingsPage.tabFillColor(settingsTabButton.checked, settingsTabButton.hovered, settingsTabButton.down)
            border.width: settingsTabButton.visualFocus ? 2 : (settingsTabButton.checked || settingsTabButton.hovered ? 1 : 0)
            border.color: settingsTabButton.visualFocus
                ? settingsPage.palette.highlight
                : (settingsTabButton.checked ? settingsPage.palette.highlight : SettingsColors.alpha(settingsPage.palette.mid, 0.55))
        }
    }

    Component.onCompleted: applyInitialTabIndex()
    onInitialTabIndexChanged: applyInitialTabIndex()
    
    header: Item {
        implicitHeight: backButton.height

        ToolButton {
            id: backButton

            readonly property color buttonFill: settings.tabFillColor(false, hovered, down)
            readonly property color buttonTextColor: hovered || down
                ? SettingsColors.contrastText(buttonFill)
                : settings.palette.windowText

            text: "‹"
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.boldFontWeight
            font.variableAxes: settingsHeaderFont.boldVariableAxes
            font.italic: settingsHeaderFont.italic
            font.pixelSize: 28
            ToolTip.text: qsTr("Back")
            ToolTip.visible: hovered
            ToolTip.delay: 500
            anchors.left: parent.left
            anchors.top: parent.top
            width: 40
            height: 40
            palette.buttonText: buttonTextColor
            contentItem: Label {
                text: backButton.text
                font: backButton.font
                color: backButton.buttonTextColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: 4
                color: backButton.buttonFill
                border.color: backButton.visualFocus
                    ? settings.palette.highlight
                    : SettingsColors.alpha(settings.palette.mid, 0.55)
                border.width: backButton.visualFocus ? 2 : (backButton.hovered || backButton.down ? 1 : 0)
            }
            onClicked: {
                sceneStack.pop();
            }
        }

        TabBar {
            id: tabView

            readonly property real visibleTabWidth: playerSettingsTab.width + songDirectoriesTab.width
                + tablesTab.width + themesTab.width + generalSettingsTab.width + keyConfigTab.width

            x: Math.max(backButton.width + 1, (parent.width - width) / 2)
            width: Math.min(visibleTabWidth, parent.width - backButton.width - 1)
            height: parent.height
            font.family: settingsHeaderFont.fontFamily
            font.weight: settingsHeaderFont.fontWeight
            font.variableAxes: settingsHeaderFont.variableAxes
            font.italic: settingsHeaderFont.italic
            palette.buttonText: settings.palette.windowText

            SettingsTabButton {
                id: playerSettingsTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("Player settings")
            }
            SettingsTabButton {
                id: songDirectoriesTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("Song directories")
            }
            SettingsTabButton {
                id: tablesTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("Tables")
            }
            SettingsTabButton {
                id: themesTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("Themes")
            }
            SettingsTabButton {
                id: generalSettingsTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("General Settings")
            }
            SettingsTabButton {
                id: keyConfigTab
                settingsPage: settings
                headerFont: settingsHeaderFont
                text: qsTr("Key config")
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
