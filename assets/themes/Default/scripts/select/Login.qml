import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Shapes
import "./playOptions"

Rectangle {
    id: bg

    color: "white"
    radius: 32

    scale: 0

    width: 1600
    height: 900

    Connections {
        target: Rg.profileList

        function onBattleActiveChanged() {
            if (Rg.profileList.battleActive) {
                bg.isConfiguringDp = true;
            }
        }
    }

    TapHandler {
        gesturePolicy: TapHandler.WithinBounds
    }

    onScaleChanged: {
        if (scale === 0) {
            isConfiguringDp = false;
        }
    }

    property bool isConfiguringDp: false

    onEnabledChanged: {
        if (enabled) {
            isConfiguringDp = Rg.profileList.battleActive;
        }
    }

    Loader {
        id: loader
        active: true
        anchors.fill: parent

        Component {
            id: mainProfileComponent
            Item {
                DarkLabel {
                    id: keys
                    anchors.top: parent.top
                    anchors.topMargin: 32
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 300
                    text: qsTr("Select Profile")
                    highlightedKeys: [1, 2]
                }

                ProfileCard {
                    anchors.centerIn: parent
                    profile: Rg.profileList.mainProfile
                    playerNumber: 0

                    onProfileChanged: Rg.profileList.mainProfile = profile;
                }
            }
        }

        Component {
            id: battleProfilesComponent
            Item {
                clip: true

                Rectangle {
                    id: mainProfile
                    color: "white"
                    radius: 32
                    width: 400
                    height: 132
                    border.color: "black"
                    border.width: 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: -32
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                    }

                    property int margin: 16

                    Rectangle {
                        id: avatarFrame
                        color: "MediumSeaGreen"
                        clip: true
                        radius: 12
                        anchors.top: parent.top
                        anchors.topMargin: mainProfile.radius + mainProfile.margin
                        height: 64
                        anchors.left: parent.left
                        anchors.leftMargin: mainProfile.margin
                        width: height

                        Image {
                            id: image
                            source: Rg.programSettings.avatarFolder + Rg.profileList.mainProfile.vars.globalVars.avatar
                            anchors.fill: parent
                            anchors.margins: 1
                            fillMode: Image.PreserveAspectFit
                            layer.enabled: true
                            layer.effect: MultiEffect {
                                shadowEnabled: true
                            }
                        }
                    }

                    Rectangle {
                        color: "MediumSeaGreen"
                        clip: true
                        radius: 12
                        anchors.top: avatarFrame.top
                        anchors.bottom: avatarFrame.bottom
                        anchors.left: avatarFrame.right
                        anchors.leftMargin: mainProfile.margin
                        anchors.right: parent.right
                        anchors.rightMargin: mainProfile.margin
                        width: height

                        TextEdit {
                            text: Rg.profileList.mainProfile.vars.globalVars.name
                            readOnly: true
                            font.pixelSize: 24
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            anchors.fill: parent
                        }
                    }
                }

                ProfileCard {
                    id: p1card
                    anchors.left: parent.left
                    anchors.leftMargin: 100
                    anchors.verticalCenter: parent.verticalCenter
                    profile: Rg.profileList.battleProfiles.player1Profile
                    playerNumber: 1
                    grayedOut: Rg.profileList.battleProfiles.player2Profile

                    Binding {
                        target: Rg.profileList.battleProfiles
                        property: "player1Profile"
                        delayed: true
                        value: {
                            if (Rg.profileList.battleProfiles.player2Profile === p1card.profile) {
                                return null;
                            } else {
                                return p1card.profile;
                            }
                        }
                    }
                }

                DarkLabel {
                    id: keys
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: mainProfile.bottom
                    anchors.topMargin: 32
                    width: 300
                    text: qsTr("Select Profiles")
                    highlightedKeys: [1, 2]
                }

                ProfileCard {
                    id: p2card
                    anchors.right: parent.right
                    anchors.rightMargin: 100
                    anchors.verticalCenter: parent.verticalCenter
                    profile: Rg.profileList.battleProfiles.player2Profile
                    playerNumber: 2
                    grayedOut: Rg.profileList.battleProfiles.player1Profile

                    Binding {
                        target: Rg.profileList.battleProfiles
                        property: "player2Profile"

                        delayed: true
                        value: {
                            if (Rg.profileList.battleProfiles.player1Profile === p2card.profile) {
                                return null;
                            } else {
                                return p2card.profile;
                            }
                        }
                    }
                }
            }
        }

        sourceComponent: bg.isConfiguringDp ? battleProfilesComponent : mainProfileComponent
    }

    property bool bothPlayersSet: Rg.profileList.battleProfiles.player1Profile && Rg.profileList.battleProfiles.player2Profile
    onBothPlayersSetChanged: {
        if (bothPlayersSet && bg.isConfiguringDp) {
            Rg.profileList.battleActive = true;
        }
    }

    states: State {
        name: "shown"; when: bg.enabled
        PropertyChanges {
            target: bg
            scale: 1
        }
    }

    transitions: Transition {
        NumberAnimation {
            properties: "scale"
            easing.type: Easing.InOutQuad
            duration: 150
        }
    }

    DarkLabel {
        id: addPlayerButton
        text: bg.isConfiguringDp ? qsTr("Single Mode") : qsTr("Battle Mode")
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter
        highlightedKeys: [7]
        width: 300

        function switchMode() {
            if (bg.isConfiguringDp) {
                Rg.profileList.battleActive = false;
                bg.isConfiguringDp = false;
            } else {
                if (Rg.profileList.battleProfiles.player1Profile && Rg.profileList.battleProfiles.player2Profile) {
                    Rg.profileList.battleActive = true;
                }
                bg.isConfiguringDp = true;
            }
        }

        Input.onCol17Pressed: {
            addPlayerButton.switchMode();
        }

        Input.onCol27Pressed: {
            addPlayerButton.switchMode();
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                addPlayerButton.switchMode();
            }
        }
    }
}