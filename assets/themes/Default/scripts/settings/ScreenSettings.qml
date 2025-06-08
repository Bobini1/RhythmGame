import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQml
import QtQuick.Controls.Basic
import "../common/helpers.js" as Helpers

Loader {
    id: screenSettingsLoader
    active: true
    function refresh() {
        if (screenSettingsLoader.script) {
            setSource(script);
        } else {
            let items = screenSettingsLoader.screenSettings.filter((item) => item.type !== "hidden");
            if (items.length === 0) {
                return;
            }
            let props = {
                name: Helpers.capitalizeFirstLetter(screenSettingsLoader.screen) + " Settings",
                items: items,
                destination: Rg.profileList.mainProfile.vars.themeVars[screenSettingsLoader.screen][currentTheme]
            }
            setSource("settingsProperties/Group.qml", props);
        }
    }
    Component.onCompleted: {
        refresh();
    }

    // we only want to reload when all of those finish changing, hence the callLater
    onScriptChanged: {
        Qt.callLater(refresh);
    }
    onScreenSettingsChanged: {
        Qt.callLater(refresh);
    }
    onCurrentThemeChanged: {
        Qt.callLater(refresh);
    }

    function openFile(fileUrl) {
        let request = new XMLHttpRequest();
        request.open("GET", fileUrl, false);
        request.send(null);
        return request.responseText;
    }

    readonly property string script: Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].screens[screen].settingsScript
    required property string screen
    readonly property var screenSettings: screenSettingsJson ? JSON.parse(openFile(screenSettingsJson)) : []
    readonly property string screenSettingsJson: Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].screens[screen].settings
    readonly property string currentTheme: Rg.profileList.mainProfile.themeConfig[screen]
}