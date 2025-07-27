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
                name: screenSettingsLoader.name,
                items: items,
                destination: screenSettingsLoader.destination
            }
            setSource("settingsProperties/Group.qml", props);
        }
    }
    readonly property string name: qsTr("%1 Settings").arg(Helpers.capitalizeFirstLetter(screenSettingsLoader.screen))
    onNameChanged: {
        if (screenSettingsLoader.item) {
            screenSettingsLoader.item.name = screenSettingsLoader.name;
        }
    }
    Component.onCompleted: {
        refresh();
    }
    readonly property var destination: {
        let scr = Rg.profileList.mainProfile.vars.themeVars[screen]
        if (scr && scr[currentTheme]) {
            return scr[currentTheme];
        }
        return undefined;
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
    onDestinationChanged: {
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