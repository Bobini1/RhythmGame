import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts

RowLayout {
    TabBar {
        id: themeTabView

        Layout.fillHeight: true
        Layout.preferredWidth: 100

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
            model: ProfileList.mainProfile.themeConfig.keys()

            TabButton {
                text: modelData
                width: 100
            }
        }
    }
    StackLayout {
        id: themesFrame

        Layout.fillHeight: true
        Layout.fillWidth: true
        currentIndex: themeTabView.currentIndex

        Repeater {
            model: ProfileList.mainProfile.themeConfig.keys()

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

                        property bool loaded: false
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 200
                        model: {
                            let themeFamilies = Themes.availableThemeFamilies;
                            let themeNames = [];
                            for (let [name, family] of Object.entries(themeFamilies)) {
                                if (family.screens[modelData]) {
                                    themeNames.push(name);
                                }
                            }
                            return themeNames;
                        }

                        Component.onCompleted: {
                            let themeFamilies = Themes.availableThemeFamilies;
                            let themeNames = [];
                            for (let [name, family] of Object.entries(themeFamilies)) {
                                if (family.screens[modelData]) {
                                    themeNames.push(name);
                                }
                            }
                            let index = themeNames.indexOf(ProfileList.mainProfile.themeConfig[modelData]);
                            currentIndex = index;
                            loaded = true;
                        }
                        onCurrentTextChanged: {
                            if (themeComboBox.loaded) {
                                ProfileList.mainProfile.themeConfig[modelData] = themeComboBox.currentText;
                            }
                        }
                    }
                    ScrollView {
                        id: scrollView
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        clip: true
                        contentWidth: Math.max(width, 450)
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
