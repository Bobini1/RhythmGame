pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml
import QtQuick.Controls
import QtQml

Row {
    id: replayAutoplay
    required property var current
    required property var currentItem
    required property StandardSelectController selection
    readonly property bool arenaSeated: Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting
    visible: !arenaSeated
    Image {
        id: auto
        source: root.iniImagesUrl + "parts.png/auto"
        enabled: replayAutoplay.selection.autoplayAvailable
        opacity: enabled ? 1 : 0.5
        MouseArea {
            anchors.fill: parent
            cursorShape: enabled ? Qt.PointingHandCursor : undefined
            onClicked: replayAutoplay.selection.openAutoplay()
        }
    }
    Repeater {
        model: replayAutoplay.selection.replayOptions
        delegate: Image {
            id: replay
            required property var modelData
            source: root.iniImagesUrl + "parts.png/replay"
            opacity: enabled ? 1 : 0.5
            enabled: ((replayAutoplay.current?.path || (replayAutoplay.current instanceof course && courseSongs.item?.canPlay)) && replayAutoplay.currentItem?.results?.length) || false
            MouseArea {
                anchors.fill: parent
                cursorShape: enabled ? Qt.PointingHandCursor : undefined
                acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                hoverEnabled: true
                ToolTip.visible: containsMouse
                ToolTip.text: replay.modelData.text
                ToolTip.delay: 500
                onClicked: (event) => {
                    root.openReplay(replay.modelData.value, event.button);
                }
            }
        }
    }
}
