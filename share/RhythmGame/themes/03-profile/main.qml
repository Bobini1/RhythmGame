import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: screen
    color: "#18232e"

    // [profile]
    readonly property Profile profile: Rg.profileList.mainProfile
    readonly property var generalVars: screen.profile.vars.generalVars
    readonly property var themeVars:
        screen.profile.vars.themeVars["main"][QmlUtils.themeName]
    // [profile]

    StandardMainController { id: actions }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, Math.min(400, screen.width - 32))
        spacing: 16

        // [profile-label]
        Label {
            Layout.fillWidth: true
            text: "Hello, " + screen.generalVars.name
            color: screen.themeVars.accentColor
            font.pixelSize: 32
            wrapMode: Text.Wrap
        }
        // [profile-label]
        Button {
            Layout.fillWidth: true
            text: "Song select"
            focus: true
            onClicked: actions.openSelect()
        }
        Button {
            Layout.fillWidth: true
            text: "Multiplayer"
            onClicked: actions.openArena()
        }
        Button {
            Layout.fillWidth: true
            text: "Settings"
            onClicked: actions.openSettings()
        }
        Button {
            Layout.fillWidth: true
            text: "Quit"
            onClicked: actions.quit()
        }
    }
}
