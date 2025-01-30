import RhythmGameQml
import QtQuick
import QtQuick.Effects
import "../common/helpers.js" as Helpers

Rectangle {
    id: card
    color: "white"
    layer.enabled: true
    layer.effect: MultiEffect {
        shadowEnabled: true
    }
    height: 600
    width: 500
    radius: 32

    signal profileSelected(profile: Profile)

    required property Profile profile
    required property int playerNumber
    property Profile grayedOut: null

    readonly property color accent: {
        switch (playerNumber) {
            case 1:
                return "RoyalBlue"
            case 2:
                return "IndianRed"
            default:
                return "MediumSeaGreen"
        }
    }

    Rectangle {
        id: playerLabel
        color: card.accent
        anchors.top: parent.top
        anchors.topMargin: 32
        anchors.horizontalCenter: parent.horizontalCenter
        height: 64
        width: 160

        radius: 15

        TextEdit {
            text: card.playerNumber === 0 ? "Player" : "Player " + card.playerNumber
            readOnly: true
            font.pixelSize: 24
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: parent
        }
    }

    PathView {
        id: pathView

        anchors.top: playerLabel.top
        anchors.topMargin: 128
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true

        property real padding: 50

        path: Path {
            startX: -pathView.itemWidth / 2 - pathView.padding
            startY: pathView.height / 2
            PathAttribute {
                name: "opacity"
                value: 0.3
            }
            PathLine {
                x: pathView.width / 2; y: pathView.height / 2
            }
            PathAttribute {
                name: "opacity"
                value: 1
            }
            PathLine {
                x: pathView.width + pathView.itemWidth / 2 + pathView.padding; y: pathView.height / 2
            }
            PathAttribute {
                name: "opacity"
                value: 0.3
            }
        }

        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5

        pathItemCount: 3

        model: {
            let profiles = ProfileList.profiles;
            if (profiles.length === 1) {
                return [profiles[0], profiles[0], profiles[0]];
            }
            if (profiles.length === 2) {
                return [profiles[0], profiles[1], profiles[0], profiles[1]];
            }
            return profiles;
        }
        property real itemWidth: 256

        Binding {
            delayed: true
            when: Helpers.getIndex(pathView.model, card.profile, pathView.currentIndex) !== null;
            pathView.currentIndex: Helpers.getIndex(pathView.model, card.profile, pathView.currentIndex);
        }

        onCurrentIndexChanged: {
            card.profile = pathView.model[pathView.currentIndex];
        }

        Component.onCompleted: {
            let index = Helpers.getIndex(pathView.model, card.profile, pathView.currentIndex);
            if (index === null) {
                index = Helpers.getIndex(pathView.model, card.grayedOut, pathView.currentIndex) || 0;
            }
            pathView.positionViewAtIndex(index, PathView.SnapPosition);
        }

        delegate: Item {
            width: pathView.itemWidth
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            opacity: {
                if (card.grayedOut === modelData) {
                    return 0.5333333333333333;
                }
                return PathView.opacity;
            }
            Rectangle {
                id: avatarFrame
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter

                color: card.accent

                radius: 15

                height: 256
                width: pathView.itemWidth

                Image {
                    id: image
                    source: ProgramSettings.avatarFolder + modelData.vars.globalVars.avatar
                    asynchronous: true
                    anchors.fill: parent
                    fillMode: Image.PreserveAspectFit
                    anchors.margins: 16
                    layer.enabled: true
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                    }
                }
            }


            Rectangle {
                id: nameFrame
                anchors.top: avatarFrame.bottom
                anchors.topMargin: 32
                anchors.horizontalCenter: parent.horizontalCenter

                color: card.accent
                radius: 15
                height: 64
                width: pathView.itemWidth

                TextEdit {
                    id: name
                    text: modelData.vars.globalVars.name
                    readOnly: true
                    font.pixelSize: 24
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    anchors.fill: parent
                }
            }
        }
    }

    TextEdit {
        id: scoreCount
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        font.pixelSize: 24
        property var profiles: pathView.model
        text: "Scores: " + profiles[pathView.currentIndex].scoreDb.getTotalScoreCount()
    }
}