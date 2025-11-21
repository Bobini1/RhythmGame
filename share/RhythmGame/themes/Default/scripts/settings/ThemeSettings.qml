import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.ScrollablePage {
    RowLayout {
        TabBar {
            id: themeTabView

            Layout.fillHeight: true
            width: 200
            implicitWidth: 200
            Layout.maximumWidth: 200

            property var orderedScreens: {
                let configKeys = Rg.profileList.mainProfile.themeConfig.keys();
                // We don't want screens to be alphabetically sorted, that's not friendly
                let order = ["k7", "k7battle", "k14", "main", "settings", "select", "result", "courseResult"];
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
                    "k7": QT_TR_NOOP("7k"),
                    "k7battle": QT_TR_NOOP("7k Battle"),
                    "k14": QT_TR_NOOP("14k"),
                    "main": QT_TR_NOOP("Main Menu"),
                    "settings": QT_TR_NOOP("Settings"),
                    "select": QT_TR_NOOP("Select"),
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
        StackLayout {
            id: themesFrame

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.maximumWidth: 1500
            currentIndex: themeTabView.currentIndex

            Repeater {
                id: screenRepeater
                model: themeTabView.orderedScreens

                Frame {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    RowLayout {
                        anchors.fill: parent
                        ComboBox {
                            id: themeComboBox

                            palette {
                                window: "white"
                                Component.onCompleted: {
                                    light = palette.button
                                }
                            }

                            Layout.alignment: Qt.AlignTop
                            Layout.maximumWidth: 300
                            Layout.preferredWidth: 200
                            Layout.fillWidth: true
                            property var themeNames: {
                                let themeFamilies = Rg.themes.availableThemeFamilies;
                                let themeNames = [];
                                for (let [name, family] of Object.entries(themeFamilies)) {
                                    if (family.screens[modelData]) {
                                        themeNames.push(name);
                                    }
                                }
                                return themeNames;
                            }
                            model: themeNames
                            currentIndex: themeNames.indexOf(Rg.profileList.mainProfile.themeConfig[modelData])

                            onCurrentTextChanged: {
                                Rg.profileList.mainProfile.themeConfig[modelData] = themeComboBox.currentText;
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
                                width: scrollView.contentWidth
                            }
                        }
                    }
                }
            }
        }
    }
}
