pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQuick.Controls.Basic
import QtQml

Row {
    id: replayAutoplay
    required property var current
    Image {
        id: auto
        source: root.iniImagesUrl + "parts.png/auto"
        enabled: replayAutoplay.current?.path || (replayAutoplay.current instanceof course && courseSongs.item?.canPlay) || false
        opacity: enabled ? 1 : 0.5
        MouseArea {
            anchors.fill: parent
            cursorShape: enabled ? Qt.PointingHandCursor : undefined
            onClicked: {
                let func = replayAutoplay.current instanceof course ? globalRoot.openCourse : globalRoot.openChart;
                let path = replayAutoplay.current instanceof course ? replayAutoplay.current : replayAutoplay.current.path;
                if (Rg.profileList.battleActive) {
                    func(path, Rg.profileList.battleProfiles.player1Profile, true, null, Rg.profileList.battleProfiles.player2Profile, true, null);
                } else {
                    func(path, Rg.profileList.mainProfile, true, null, null, false, null);
                }
            }
        }
    }
    Repeater {
        model: 4
        delegate: Image {
            id: replay
            source: root.iniImagesUrl + "parts.png/replay"
            opacity: enabled ? 1 : 0.5
            enabled: ((replayAutoplay.current?.path || (replayAutoplay.current instanceof course && courseSongs.item?.canPlay)) && replayAutoplay.currentItem?.scores?.length) || false
            MouseArea {
                anchors.fill: parent
                cursorShape: enabled ? Qt.PointingHandCursor : undefined
                hoverEnabled: true
                ToolTip.visible: containsMouse
                ToolTip.text: {
                    switch (modelData) {
                        case 0:
                            return qsTr("NEWEST");
                        case 1:
                            return qsTr("BEST SCORE");
                        case 2:
                            return qsTr("BEST CLEAR");
                        case 3:
                            return qsTr("BEST COMBO");
                    }
                }
                ToolTip.delay: 500
                onClicked: {
                    root.openReplay(modelData);
                }
            }
        }
    }
}