import RhythmGameQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Shapes

Rectangle {
    id: bg

    color: "white"
    radius: 32

    scale: 0

    width: 1600
    height: 900

    Connections {
        target: ProfileList

        function onBattleActiveChanged() {
            if (ProfileList.battleActive) {
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
            isConfiguringDp = ProfileList.battleActive;
        }
    }

    Loader {
        id: loader
        active: true
        anchors.fill: parent

        Component {
            id: mainProfileComponent
            Item {
                ProfileCard {
                    anchors.centerIn: parent
                    profile: ProfileList.mainProfile
                    playerNumber: 0

                    onProfileChanged: ProfileList.mainProfile = profile;
                }
            }
        }

        Component {
            id: battleProfilesComponent
            Item {
                clip: true
                // a rectangle with its two bottom corners rounded
                Rectangle {
                    id: bg
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
                        anchors.topMargin: bg.radius + bg.margin
                        height: 64
                        anchors.left: parent.left
                        anchors.leftMargin: bg.margin
                        width: height

                        Image {
                            id: image
                            source: ProgramSettings.avatarFolder + ProfileList.mainProfile.vars.globalVars.avatar
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
                        anchors.leftMargin: bg.margin
                        anchors.right: parent.right
                        anchors.rightMargin: bg.margin
                        width: height

                        TextEdit {
                            text: ProfileList.mainProfile.vars.globalVars.name
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
                    profile: ProfileList.battleProfiles.player1Profile
                    playerNumber: 1
                    grayedOut: ProfileList.battleProfiles.player2Profile

                    Binding {
                        target: ProfileList.battleProfiles
                        property: "player1Profile"
                        delayed: true
                        value: {
                            if (ProfileList.battleProfiles.player2Profile === p1card.profile) {
                                return null;
                            } else {
                                return p1card.profile;
                            }
                        }
                    }
                }

                ProfileCard {
                    id: p2card
                    anchors.right: parent.right
                    anchors.rightMargin: 100
                    anchors.verticalCenter: parent.verticalCenter
                    profile: ProfileList.battleProfiles.player2Profile
                    playerNumber: 2
                    grayedOut: ProfileList.battleProfiles.player1Profile

                    Binding {
                        target: ProfileList.battleProfiles
                        property: "player2Profile"

                        delayed: true
                        value: {
                            if (ProfileList.battleProfiles.player1Profile === p2card.profile) {
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

    property bool bothPlayersSet: ProfileList.battleProfiles.player1Profile && ProfileList.battleProfiles.player2Profile
    onBothPlayersSetChanged: {
        if (bothPlayersSet && bg.isConfiguringDp) {
            ProfileList.battleActive = true;
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

    Button {
        id: addPlayerButton
        text: bg.isConfiguringDp ? "Back to Single Mode" : "Switch to Battle Mode"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter

        onClicked: {
            if (bg.isConfiguringDp) {
                ProfileList.battleActive = false;
                bg.isConfiguringDp = false;
            } else {
                if (ProfileList.battleProfiles.player1Profile && ProfileList.battleProfiles.player2Profile) {
                    ProfileList.battleActive = true;
                }
                bg.isConfiguringDp = true;
            }
        }
    }
}