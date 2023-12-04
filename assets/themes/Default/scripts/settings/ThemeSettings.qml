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
            model: themesFrame.themeConfig.keys()

            TabButton {
                text: modelData
                width: 100
            }
        }
    }
    StackLayout {
        id: themesFrame

        readonly property var themeConfig: ProfileList.currentProfile.themeConfig

        Layout.fillHeight: true
        Layout.fillWidth: true
        currentIndex: themeTabView.currentIndex

        Repeater {
            model: themesFrame.themeConfig.keys()

            Frame {
                ComboBox {
                    id: themeComboBox

                    property bool loaded: false

                    Layout.fillWidth: true
                    model: {
                        let themeFamilies = Themes.availableThemeFamilies;
                        let themeNames = [];
                        for (let [name, family] of Object.entries(themeFamilies)) {
                            if (family.themes[modelData]) {
                                themeNames.push(name);
                            }
                        }
                        return themeNames;
                    }

                    Component.onCompleted: {
                        let themeFamilies = Themes.availableThemeFamilies;
                        let themeNames = [];
                        for (let [name, family] of Object.entries(themeFamilies)) {
                            if (family.themes[modelData]) {
                                themeNames.push(name);
                            }
                        }
                        let index = themeNames.indexOf(themesFrame.themeConfig[modelData]);
                        currentIndex = index;
                        loaded = true;
                    }
                    onCurrentTextChanged: {
                        if (themeComboBox.loaded) {
                            themesFrame.themeConfig[modelData] = themeComboBox.currentText;
                        }
                    }
                }
            }
        }
    }
}
