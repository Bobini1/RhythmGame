import QtQuick
import "../common"

// Gameplay density graph: same content as the song-select density graph but
// with a minimal 1 px white border instead of the decorative INI frame.
Rectangle {
    id: root

    // --- visibility / style ---
    property bool contentVisible: true
    property bool vertical: false
    property bool gapsEnabled: false
    property bool showNotes: true
    property bool showBpm:   true
    property bool frameEnabled:      true
    property real backgroundOpacity: 0.8

    // --- chart data ---
    required property var  histogramData
    required property var  bpms
    required property real mainBpm
    required property real maxBpm
    required property real minBpm
    required property real length

    // --- playback position ---
    property real elapsed: -1

    visible: contentVisible
    color:   "transparent"
    border {
        color: root.frameEnabled ? "white" : "transparent"
        width: root.frameEnabled ? 1 : 0
    }

    DensityGraphContent {
        anchors {
            fill:    parent
            margins: root.frameEnabled ? 1 : 0
        }

        histogramData: root.histogramData
        bpms:          root.bpms
        mainBpm:       root.mainBpm
        maxBpm:        root.maxBpm
        minBpm:        root.minBpm
        length:        root.length
        gapsEnabled:        root.gapsEnabled
        showNotes:          root.showNotes
        showBpm:            root.showBpm
        backgroundOpacity:  root.backgroundOpacity
        vertical:           root.vertical
        positionRatio: root.elapsed >= 0 && root.length > 0
                       ? Math.min(root.elapsed / root.length, 1.0)
                       : -1
    }
}








