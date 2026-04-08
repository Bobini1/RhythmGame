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
    property real notesOpacity:          1.0
    property real bpmOpacity:            1.0
    property real frameOpacity:          1.0
    property real backgroundOpacity:     0.8
    property real bpmConnectorOpacity:   0.75

    // --- chart data ---
    required property var  histogramData
    required property var  bpms
    required property real mainBpm
    required property real maxBpm
    required property real minBpm
    required property real length

    // --- playback position ---
    property real elapsed: -1
    property real positionLineOpacity: 1.0

    visible: contentVisible
    color:   "transparent"
    border {
        color: Qt.rgba(1, 1, 1, root.frameOpacity)
        width: root.frameOpacity > 0 ? 1 : 0
    }

    DensityGraphContent {
        anchors {
            fill:    parent
            margins: root.frameOpacity > 0 ? 1 : 0
        }

        histogramData: root.histogramData
        bpms:          root.bpms
        mainBpm:       root.mainBpm
        maxBpm:        root.maxBpm
        minBpm:        root.minBpm
        length:        root.length
        gapsEnabled:            root.gapsEnabled
        notesOpacity:           root.notesOpacity
        bpmOpacity:             root.bpmOpacity
        backgroundOpacity:      root.backgroundOpacity
        bpmConnectorOpacity:    root.bpmConnectorOpacity
        vertical:               root.vertical
        positionRatio: root.elapsed >= 0 && root.length > 0
                       ? Math.min(root.elapsed / root.length, 1.0)
                       : -1
        positionLineOpacity: root.positionLineOpacity
    }
}








