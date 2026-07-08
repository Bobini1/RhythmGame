import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import "SettingsColors.js" as SettingsColors

ColumnLayout {
    required property color tabTextColor

    Layout.fillHeight: true
    Layout.fillWidth: true
    spacing: 0

    SettingsWorkspaceScaffold {
        id: pageScaffold

        Layout.fillHeight: true
        Layout.fillWidth: true
        SettingsPageHeader {
            title: qsTr("Themes")
            subtitle: qsTr("Choose screens, theme families, and per-screen theme options.")
        }

        RowLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 12

            WorkbenchPanel {
                title: qsTr("Screens")
                subtitle: qsTr("Configure each theme screen.")
                Layout.fillHeight: true
                Layout.preferredWidth: 220
                Layout.maximumWidth: 260

                Item {
                    id: themeTabView

                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    property int currentIndex: 0

                    property var orderedScreens: {
                        let configKeys = Rg.profileList.mainProfile.themeConfig.keys();
                        let order = ["k7", "k7battle", "k5", "k5battle", "k10", "k14", "main", "settings", "select", "decide", "result", "courseResult"];
                        return configKeys.sort((a, b) => {
                            let indexA = order.indexOf(a);
                            let indexB = order.indexOf(b);
                            if (indexA === -1) indexA = Infinity;
                            if (indexB === -1) indexB = Infinity;
                            return indexA - indexB;
                        });
                    }

                    property var displayNames: {
                        return {
                            "k5": QT_TR_NOOP("5k"),
                            "k5battle": QT_TR_NOOP("5k Battle"),
                            "k10": QT_TR_NOOP("10k"),
                            "k7": QT_TR_NOOP("7k"),
                            "k7battle": QT_TR_NOOP("7k Battle"),
                            "k14": QT_TR_NOOP("14k"),
                            "main": QT_TR_NOOP("Main Menu"),
                            "settings": QT_TR_NOOP("Settings"),
                            "select": QT_TR_NOOP("Select"),
                            "decide": QT_TR_NOOP("Decide"),
                            "result": QT_TR_NOOP("Result"),
                            "courseResult": QT_TR_NOOP("Course Result")
                        };
                    }

                    function screenLabel(screen) {
                        return qsTr(displayNames[screen] || screen);
                    }

                    ListView {
                        id: screenList

                        anchors.fill: parent
                        boundsBehavior: Flickable.StopAtBounds
                        currentIndex: themeTabView.currentIndex
                        flickableDirection: Flickable.AutoFlickIfNeeded
                        highlightMoveDuration: 0
                        highlightRangeMode: ListView.ApplyRange
                        preferredHighlightBegin: 40
                        preferredHighlightEnd: height - 40
                        snapMode: ListView.SnapToItem
                        spacing: 4
                        model: themeTabView.orderedScreens

                        delegate: ItemDelegate {
                            id: screenButton

                            width: ListView.view.width
                            implicitHeight: 36
                            leftPadding: 12
                            rightPadding: 12
                            topPadding: 8
                            bottomPadding: 8
                            onClicked: themeTabView.currentIndex = index

                            contentItem: Label {
                                text: themeTabView.screenLabel(modelData)
                                color: tabTextColor
                                font.bold: screenButton.ListView.isCurrentItem
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                                maximumLineCount: 1
                            }

                            background: Rectangle {
                                radius: 6
                                color: SettingsColors.rowFill(screenButton.palette, screenButton.ListView.isCurrentItem, screenButton.hovered)

                                Rectangle {
                                    width: 18
                                    height: 2
                                    radius: 1
                                    visible: screenButton.ListView.isCurrentItem
                                    color: screenButton.palette.highlight
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 4
                                }
                            }
                        }
                    }
                }
            }

            StackLayout {
                Layout.fillHeight: true
                Layout.fillWidth: true
                currentIndex: themeTabView.currentIndex

                Repeater {
                    model: themeTabView.orderedScreens

                    ColumnLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        spacing: 12

                        WorkbenchPanel {
                            Layout.fillWidth: true
                            title: themeTabView.displayNames[modelData] ? qsTr(themeTabView.displayNames[modelData]) : modelData
                            subtitle: qsTr("Theme family for this screen.")

                            ComboBox {
                                id: themeComboBox

                                Layout.fillWidth: true
                                Layout.maximumWidth: 420

                                function themeDisplayName(name) {
                                    const text = String(name);
                                    if (!text.toLowerCase().endsWith(".lr2skin)")) {
                                        return text;
                                    }
                                    const filenameStart = text.lastIndexOf(" (");
                                    return filenameStart === -1 ? text : text.slice(0, filenameStart);
                                }

                                function baseScreenForAlias(screen) {
                                    if (screen === "k5") {
                                        return "k7";
                                    }
                                    if (screen === "k5battle") {
                                        return "k7battle";
                                    }
                                    return "";
                                }

                                function hasNativeScreen(themeFamilies, themePath, screen) {
                                    for (let [name, family] of Object.entries(themeFamilies)) {
                                        const screenData = family.screens[screen];
                                        if (family.path === themePath && screenData && !screenData.aliased) {
                                            return true;
                                        }
                                    }
                                    return false;
                                }

                                function shouldShowTheme(themeFamilies, family, screen) {
                                    const screenData = family.screens[screen];
                                    if (!screenData) {
                                        return false;
                                    }

                                    const baseScreen = baseScreenForAlias(screen);
                                    if (baseScreen === "" || !screenData.aliased || !family.screens[baseScreen]) {
                                        return true;
                                    }

                                    return !hasNativeScreen(themeFamilies, family.path, screen);
                                }

                                function themeChoiceIndex(themeChoices, themeName) {
                                    for (let i = 0; i < themeChoices.length; ++i) {
                                        if (themeChoices[i].name === themeName) {
                                            return i;
                                        }
                                    }
                                    return -1;
                                }

                                property var themeChoices: {
                                    let themeFamilies = Rg.themes.availableThemeFamilies;
                                    let choices = [];
                                    for (let [name, family] of Object.entries(themeFamilies)) {
                                        if (shouldShowTheme(themeFamilies, family, modelData)) {
                                            choices.push({
                                                label: themeDisplayName(name),
                                                name: name
                                            });
                                        }
                                    }
                                    return choices;
                                }
                                model: themeChoices.map((choice) => choice.label)
                                currentIndex: themeChoiceIndex(themeChoices, Rg.profileList.mainProfile.themeConfig[modelData])

                                onActivated: (index) => {
                                    Rg.profileList.mainProfile.themeConfig[modelData] = themeChoices[index].name;
                                }
                            }
                        }

                        ScrollView {
                            id: scrollView

                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            clip: true
                            contentWidth: Math.max(width, 620)

                            ScreenSettings {
                                screen: modelData
                                width: scrollView.contentWidth
                            }
                        }
                    }
                }
            }
        }
    }
}
