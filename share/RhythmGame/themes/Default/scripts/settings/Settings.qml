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
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    ThemeFont {
        id: settingsHeaderFont
        fileName: settings.themeVars.settingsHeaderFont
        fallbackFileName: "file:NotoSans-VariableFont_wdth,wght.ttf"
    }

    font: settingsUiFont.uiFont({
        weight: settingsUiFont.fontWeight,
        variableAxes: settingsUiFont.variableAxes,
        italic: settingsUiFont.italic
    })

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

    component SettingsTabButton: Button {
        id: settingsTabButton

        required property var settingsPage
        required property var headerFont
        required property bool selected

        readonly property real minimumTabWidth: 72
        readonly property real reservedTextWidth: Math.ceil(Math.max(regularTextSizer.implicitWidth, checkedTextSizer.implicitWidth))

        signal selectedRequested()

        implicitWidth: Math.max(minimumTabWidth, reservedTextWidth + leftPadding + rightPadding)
        implicitHeight: 40
        width: implicitWidth
        leftPadding: 14
        rightPadding: 14
        topPadding: 8
        bottomPadding: 8
        font: headerFont.uiFont({
            weight: selected ? headerFont.boldFontWeight : headerFont.fontWeight,
            variableAxes: selected ? headerFont.boldVariableAxes : headerFont.variableAxes,
            italic: headerFont.italic
        })

        onClicked: selectedRequested()

        Label {
            id: regularTextSizer

            visible: false
            text: settingsTabButton.text
            font: settingsTabButton.headerFont.uiFont({
                weight: settingsTabButton.headerFont.fontWeight,
                variableAxes: settingsTabButton.headerFont.variableAxes,
                italic: settingsTabButton.headerFont.italic
            })
        }

        Label {
            id: checkedTextSizer

            visible: false
            text: settingsTabButton.text
            font: settingsTabButton.headerFont.uiFont({
                weight: settingsTabButton.headerFont.boldFontWeight,
                variableAxes: settingsTabButton.headerFont.boldVariableAxes,
                italic: settingsTabButton.headerFont.italic
            })
        }

        contentItem: Label {
            text: settingsTabButton.text
            font: settingsTabButton.font
            color: settingsPage.tabTextColor(settingsTabButton.selected, settingsTabButton.enabled)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        background: Rectangle {
            radius: 6
            color: settingsPage.tabFillColor(settingsTabButton.selected, settingsTabButton.hovered, settingsTabButton.down)
            border.width: settingsTabButton.visualFocus ? 2 : (settingsTabButton.selected || settingsTabButton.hovered ? 1 : 0)
            border.color: settingsTabButton.visualFocus
                ? settingsPage.palette.highlight
                : (settingsTabButton.selected ? settingsPage.palette.highlight : SettingsColors.alpha(settingsPage.palette.mid, 0.55))
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
            font: settingsHeaderFont.uiFont({
                weight: settingsHeaderFont.boldFontWeight,
                variableAxes: settingsHeaderFont.boldVariableAxes,
                italic: settingsHeaderFont.italic,
                pixelSize: 28
            })
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

        Flickable {
            id: tabViewport

            readonly property real edgeGap: 1
            readonly property real availableWidth: Math.max(0, parent.width - backButton.width - edgeGap)
            readonly property real tabStripWidth: tabView.implicitWidth
            readonly property bool overflowing: tabStripWidth > availableWidth

            x: overflowing ? backButton.width + edgeGap : Math.max(backButton.width + edgeGap, (parent.width - width) / 2)
            width: Math.min(tabStripWidth, availableWidth)
            height: parent.height
            contentWidth: tabStripWidth
            contentHeight: height
            clip: overflowing
            interactive: overflowing
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            function ensureCurrentTabVisible() {
                if (!overflowing || width <= 0) {
                    contentX = 0;
                    return;
                }

                const currentTab = tabView.itemAt(tabView.currentIndex);
                if (!currentTab) {
                    return;
                }

                const margin = 12;
                const leftEdge = currentTab.x;
                const rightEdge = currentTab.x + currentTab.width;
                const maxContentX = Math.max(0, contentWidth - width);

                if (leftEdge < contentX + margin) {
                    contentX = Math.max(0, leftEdge - margin);
                } else if (rightEdge > contentX + width - margin) {
                    contentX = Math.min(maxContentX, rightEdge - width + margin);
                }
            }

            onWidthChanged: Qt.callLater(ensureCurrentTabVisible)
            onContentWidthChanged: Qt.callLater(ensureCurrentTabVisible)

            Row {
                id: tabView

                property int currentIndex: 0
                readonly property int count: 6

                function itemAt(index) {
                    switch (index) {
                    case 0:
                        return playerSettingsTab;
                    case 1:
                        return songDirectoriesTab;
                    case 2:
                        return tablesTab;
                    case 3:
                        return themesTab;
                    case 4:
                        return generalSettingsTab;
                    case 5:
                        return keyConfigTab;
                    default:
                        return null;
                    }
                }

                width: implicitWidth
                height: tabViewport.height

                onCurrentIndexChanged: Qt.callLater(tabViewport.ensureCurrentTabVisible)

                SettingsTabButton {
                    id: playerSettingsTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 0
                    text: qsTr("Player settings")
                    onSelectedRequested: tabView.currentIndex = 0
                }
                SettingsTabButton {
                    id: songDirectoriesTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 1
                    text: qsTr("Song directories")
                    onSelectedRequested: tabView.currentIndex = 1
                }
                SettingsTabButton {
                    id: tablesTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 2
                    text: qsTr("Tables")
                    onSelectedRequested: tabView.currentIndex = 2
                }
                SettingsTabButton {
                    id: themesTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 3
                    text: qsTr("Themes")
                    onSelectedRequested: tabView.currentIndex = 3
                }
                SettingsTabButton {
                    id: generalSettingsTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 4
                    text: qsTr("General Settings")
                    onSelectedRequested: tabView.currentIndex = 4
                }
                SettingsTabButton {
                    id: keyConfigTab
                    settingsPage: settings
                    headerFont: settingsHeaderFont
                    selected: tabView.currentIndex === 5
                    text: qsTr("Key config")
                    onSelectedRequested: tabView.currentIndex = 5
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
                uiFont: settingsUiFont
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
