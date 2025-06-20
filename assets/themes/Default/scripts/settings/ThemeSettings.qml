import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts

RowLayout {
    TabBar {
        id: themeTabView

        Layout.fillHeight: true
        width: 200
        implicitWidth: 200
        Layout.maximumWidth: 200

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

        Repeater {
            model: Rg.profileList.mainProfile.themeConfig.keys()

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
            model: Rg.profileList.mainProfile.themeConfig.keys()

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
