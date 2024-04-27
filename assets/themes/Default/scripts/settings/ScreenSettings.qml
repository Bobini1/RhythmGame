import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQml
import QtQuick.Controls.Basic

Frame {
    id: settingsFrame

    required property string screen
    readonly property var screenSettings: screenSettingsJson ? JSON.parse(openFile(screenSettingsJson)) : []
    readonly property string screenSettingsJson: Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig[screen]].screens[screen].settings

    function openFile(fileUrl) {
        let request = new XMLHttpRequest();
        request.open("GET", fileUrl, false);
        request.send(null);
        return request.responseText;
    }

    ColumnLayout {
        anchors.fill: parent

        Repeater {
            id: propertyRepeater

            function capitalizeFirstLetter(string) {
                return string.charAt(0).toUpperCase() + string.slice(1);
            }

            model: settingsFrame.screenSettings

            Loader {
                property var destination: ProfileList.currentProfile.vars.themeVars[settingsFrame.screen]
                property var props: modelData

                active: true
                source: "SettingsProperties/" + propertyRepeater.capitalizeFirstLetter(modelData.type) + ".qml"

                Component.onCompleted: {
                    print(JSON.stringify(ProfileList.currentProfile.vars.themeVars), settingsFrame.screen);
                }
            }
        }
    }
}