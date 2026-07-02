import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts

ColumnLayout {
    required property color tabTextColor

    Layout.fillHeight: true
    Layout.fillWidth: true
    spacing: 12

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

            TabBar {
                id: themeTabView

                Layout.fillHeight: true
                Layout.fillWidth: true
                palette.buttonText: tabTextColor

                property var orderedScreens: {
                    let configKeys = Rg.profileList.mainProfile.themeConfig.keys();
                    // We don't want screens to be alphabetically sorted, that's not friendly
                    let order = ["k7", "k7battle", "k5", "k5battle", "k10", "k14", "main", "settings", "select", "decide", "result", "courseResult"];
                    return configKeys.sort((a, b) => {
                        let indexA = order.indexOf(a);
                        let indexB = order.indexOf(b);
                        if (indexA === -1) indexA = Infinity;
                        if (indexB === -1) indexB = Infinity;
                        return indexA - indexB;
                    });
                }

                contentItem: ListView {
                    boundsBehavior: Flickable.StopAtBounds
                    currentIndex: themeTabView.currentIndex
                    flickableDirection: Flickable.AutoFlickIfNeeded
                    highlightMoveDuration: 0
                    highlightRangeMode: ListView.ApplyRange
                    model: themeTabView.contentModel
                    preferredHighlightBegin: 40
                    preferredHighlightEnd: height - 40
                    snapMode: ListView.SnapToItem
                    spacing: themeTabView.spacing
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

                Repeater {
                    id: themeTabRepeater
                    model: themeTabView.orderedScreens.map((str) => qsTr(themeTabView.displayNames[str] || str))

                    TabButton {
                        text: modelData
                        width: themeTabView.width
                    }
                }
            }
        }

        StackLayout {
            id: themesFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.maximumWidth: 1500
            currentIndex: themeTabView.currentIndex

            Repeater {
                id: screenRepeater
                model: themeTabView.orderedScreens

                WorkbenchPanel {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    title: themeTabView.displayNames[modelData] ? qsTr(themeTabView.displayNames[modelData]) : modelData
                    subtitle: qsTr("Theme family and settings for this screen.")

                    RowLayout {
                        Layout.fillHeight: true
                        Layout.fillWidth: true

                        ComboBox {
                            id: themeComboBox

                            Layout.alignment: Qt.AlignTop
                            Layout.minimumWidth: 220
                            Layout.preferredWidth: 260
                            Layout.maximumWidth: 360
                            Layout.fillWidth: false

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

                        ScrollView {
                            id: scrollView

                            Layout.fillHeight: true
                            Layout.fillWidth: true
                            clip: true
                            contentWidth: Math.max(width, 550)

                            ScreenSettings {
                                id: screenSettings

                                screen: modelData
                                anchors.left: parent.left
                                anchors.right: parent.right
                            }
                        }
                    }
                }
            }
        }
    }
}
