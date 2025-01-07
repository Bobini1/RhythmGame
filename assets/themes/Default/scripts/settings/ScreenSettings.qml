import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

Loader {
    id: screenSettingsLoader
    active: true
    property var destination: ProfileList.mainProfile.vars.themeVars[screen]
    property var props: {
        "items": screenSettings.filter((item) => item.type !== "hidden"),
        "name": Helpers.capitalizeFirstLetter(screen) + " Settings",
        "type": "group"
    }
    source: script ? script : (screenSettings.length > 0 ? "settingsProperties/Group.qml" : "")

    function openFile(fileUrl) {
        let request = new XMLHttpRequest();
        request.open("GET", fileUrl, false);
        request.send(null);
        return request.responseText;
    }

    readonly property string script: Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig[screen]].screens[screen].settingsScript
    required property string screen
    readonly property var screenSettings: screenSettingsJson ? JSON.parse(openFile(screenSettingsJson)) : []
    readonly property string screenSettingsJson: Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig[screen]].screens[screen].settings
}