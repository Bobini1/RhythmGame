import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

Rectangle {
    id: screen
    color: "#18232e"

    // [profile]
    readonly property Profile profile: Rg.profileList.mainProfile
    readonly property var generalVars: screen.profile.vars.generalVars
    readonly property var themeVars:
        screen.profile.vars.themeVars["main"][screen.QmlUtils.themeName]
    // [profile]

    StandardMainActions { id: actions; enabled: screen.enabled }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.max(0, Math.min(400, screen.width - 32))
        spacing: 16

        // [profile-label]
        Label {
            Layout.fillWidth: true
            text: qsTr("Hello, %1").arg(screen.generalVars.name)
            color: screen.themeVars.accentColor
            font.pixelSize: 32
            wrapMode: Text.Wrap
        }
        // [profile-label]
        Button {
            Layout.fillWidth: true
            text: qsTr("Song select")
            focus: true
            onClicked: actions.openSelect()
        }
        // [arena-menu]
        Button {
            Layout.fillWidth: true
            text: qsTr("Arena")
            onClicked: actions.openArena()
        }
        // [arena-menu]
        Button {
            Layout.fillWidth: true
            text: qsTr("Settings")
            onClicked: actions.openSettings()
        }
        Button {
            Layout.fillWidth: true
            text: qsTr("Quit")
            onClicked: actions.quit()
        }
    }
}
