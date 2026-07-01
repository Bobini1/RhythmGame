import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import "../common/helpers.js" as Helpers
import "../common"

Image {
    id: graph

    required property var  bpms
    required property var  histogramData
    required property real mainBpm
    required property real maxBpm
    required property real minBpm
    required property real length
    required property bool gapsEnabled
    property real bpmConnectorOpacity: 0.4

    required property int normalCount
    required property int scratchCount
    required property int lnCount
    required property int bssCount

    ThemeFont {
        id: graphFont
        fileName: root.themeVars.songInfoFont
    }

    asynchronous: true
    source: root.iniImagesUrl + "parts.png/graph"

    // ── Graph area (inside the decorative frame) ──────────────────────────
    DensityGraphContent {
        id: graphContent
        anchors {
            fill:         parent
            leftMargin:   14
            rightMargin:  14
            topMargin:    14
            bottomMargin: 54
        }

        histogramData: graph.histogramData
        bpms:          graph.bpms
        mainBpm:       graph.mainBpm
        maxBpm:        graph.maxBpm
        minBpm:        graph.minBpm
        length:        graph.length
        gapsEnabled:            graph.gapsEnabled
        bpmConnectorOpacity:    graph.bpmConnectorOpacity
        // vertical orientation is not used in song select
        vertical: false
    }

    // ── Note-count row (select-only; uses assets from the select images folder) ──
    component CountText: Text {
        font.family: graphFont.fontFamily
        font.weight: graphFont.fontWeight
        font.italic: graphFont.italic
        font.pixelSize: 26
        fontSizeMode: Text.HorizontalFit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        minimumPixelSize: 8
        clip: true
        Layout.fillHeight: true
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.preferredWidth: 1
    }

    RowLayout {
        anchors {
            left:      graph.left
            right:     graph.right
            bottom:    graph.bottom
            top:       graphContent.bottom
            margins:   14
            topMargin: 7
        }
        height: 42
        spacing: 2

        Image {
            source: root.iniImagesUrl + "parts.png/note_grey"
            Layout.alignment: Qt.AlignVCenter
        }
        CountText {
            text: graph.normalCount
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_red"
            Layout.alignment: Qt.AlignVCenter
        }
        CountText {
            text: graph.scratchCount
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_blue"
            Layout.alignment: Qt.AlignVCenter
        }
        CountText {
            text: graph.lnCount
        }
        Image {
            source: root.iniImagesUrl + "parts.png/note_green"
            Layout.alignment: Qt.AlignVCenter
        }
        CountText {
            text: graph.bssCount
        }
    }
}
