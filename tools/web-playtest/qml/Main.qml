pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root

    required property string buildInputSha256
    readonly property color pageColor: "#0b0d12"
    readonly property color panelColor: "#141821"
    readonly property color lineColor: "#303746"
    readonly property color accentColor: "#d8ff55"

    visible: true
    width: 1280
    height: 800
    minimumWidth: 1040
    minimumHeight: 680
    title: qsTr("RhythmGame — Dstorv web playtest")
    color: root.pageColor

    FrameAnimation {
        running: true
        onTriggered: webPlaytest.refreshSnapshot()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 74
            color: root.panelColor
            border.color: root.lineColor
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 18

                Column {
                    Layout.fillWidth: true
                    spacing: 3

                    Label {
                        color: "#f5f7fb"
                        font.pixelSize: 22
                        font.bold: true
                        text: webPlaytest.chartTitle.length > 0
                              ? webPlaytest.chartTitle
                              : qsTr("Dstorv — loading")
                    }

                    Label {
                        color: "#98a2b3"
                        font.pixelSize: 13
                        text: qsTr("%1  ·  %2 BPM")
                              .arg(webPlaytest.chartArtist.length > 0
                                   ? webPlaytest.chartArtist
                                   : qsTr("Unknown artist"))
                              .arg(webPlaytest.chartBpm.toFixed(2))
                    }
                }

                Label {
                    color: webPlaytest.phase === 7 ? "#ff6b72" : root.accentColor
                    font.pixelSize: 17
                    font.bold: true
                    text: webPlaytest.startPending
                          ? qsTr("Starting audio…")
                          : webPlaytest.phaseText
                }

                ComboBox {
                    id: presetChooser
                    Layout.preferredWidth: 208
                    enabled: !webPlaytest.inputPresetLocked
                    model: [
                        qsTr("Native: A S D Space J K L"),
                        qsTr("LR2: Z S X D C F V")
                    ]
                    currentIndex: webPlaytest.inputPreset
                    onActivated: index => webPlaytest.inputPreset = index
                    Accessible.name: qsTr("Keyboard preset")
                }

                Button {
                    enabled: webPlaytest.canStart
                    text: webPlaytest.phase === 5 || webPlaytest.phase === 6
                          ? qsTr("Retry")
                          : qsTr("Start")
                    onClicked: webPlaytest.startFromTrustedGesture()
                }

                Button {
                    enabled: webPlaytest.canAbort
                    text: qsTr("Abort")
                    onClicked: webPlaytest.abort()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            Rectangle {
                Layout.preferredWidth: 250
                Layout.fillHeight: true
                color: root.panelColor
                border.color: root.lineColor
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Label {
                        Layout.fillWidth: true
                        color: "#f5f7fb"
                        font.pixelSize: 16
                        font.bold: true
                        text: qsTr("RUN")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 8

                        Label { color: "#8d98aa"; text: qsTr("Score") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: qsTr("%1 / %2")
                                  .arg(webPlaytest.score.toFixed(0))
                                  .arg(webPlaytest.maxScoreNow.toFixed(0))
                        }
                        Label { color: "#8d98aa"; text: qsTr("Combo") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: root.accentColor
                            font.bold: true
                            text: webPlaytest.combo
                        }
                        Label { color: "#8d98aa"; text: qsTr("Judgement") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: webPlaytest.latestJudgement.length > 0
                                  ? qsTr("%1  %2 ms")
                                    .arg(webPlaytest.latestJudgement)
                                    .arg(webPlaytest.latestDeviationMs.toFixed(1))
                                  : qsTr("—")
                        }
                        Label { color: "#8d98aa"; text: qsTr("Elapsed") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: qsTr("%1 s").arg(webPlaytest.elapsedSeconds.toFixed(2))
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        color: "#8d98aa"
                        text: qsTr("NORMAL gauge")
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: 100
                        value: webPlaytest.gauge
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        color: "#f5f7fb"
                        text: qsTr("%1%").arg(webPlaytest.gauge.toFixed(1))
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.lineColor
                    }

                    Label {
                        Layout.fillWidth: true
                        color: "#f5f7fb"
                        font.bold: true
                        text: qsTr("CONTROLS")
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: "#aeb6c4"
                        lineHeight: 1.35
                        text: webPlaytest.inputPreset === 0
                              ? qsTr("Scratch: Left Shift / Left Control\nKeys: A S D Space J K L\nEnter: start · Esc: abort")
                              : qsTr("Scratch: Left Shift\nKeys: Z S X D C F V\nEnter: start · Esc: abort")
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WrapAnywhere
                        color: "#596274"
                        font.pixelSize: 9
                        text: qsTr("Build input: %1").arg(root.buildInputSha256)
                    }
                }
            }

            Rectangle {
                id: playfield

                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.minimumWidth: 480
                color: "#080a0e"
                border.color: root.lineColor
                radius: 4
                clip: true

                readonly property real laneWidth: width / 8
                readonly property real pixelsPerScroll: Math.max(34, height / 15)
                readonly property real judgementY: height * 0.82

                Repeater {
                    model: 8

                    Rectangle {
                        required property int index

                        x: index * playfield.laneWidth
                        width: playfield.laneWidth
                        height: playfield.height
                        color: (webPlaytest.pressedLaneMask & (1 << index)) !== 0
                               ? (index === 0 ? "#493e12" : "#1f3444")
                               : (index === 0
                                  ? "#17140a"
                                  : index % 2 === 0 ? "#0d1118" : "#151a23")
                        border.color: "#252b36"
                        Accessible.ignored: true
                    }
                }

                Repeater {
                    model: webPlaytest.noteModel

                    delegate: Item {
                        id: noteDelegate

                        required property int stableId
                        required property int displayColumn
                        required property int noteType
                        required property double scrollPosition
                        required property double pairedScrollPosition
                        required property bool hasPairedScrollPosition
                        required property bool holding
                        readonly property real headY: playfield.judgementY
                                                        - (scrollPosition
                                                           - webPlaytest.currentScrollPosition)
                                                          * playfield.pixelsPerScroll
                        readonly property real tailY: playfield.judgementY
                                                        - (pairedScrollPosition
                                                           - webPlaytest.currentScrollPosition)
                                                          * playfield.pixelsPerScroll
                        readonly property bool longNote: noteType === 0

                        x: displayColumn * playfield.laneWidth + 4
                        y: longNote ? Math.min(headY, tailY) : headY - 6
                        width: playfield.laneWidth - 8
                        height: longNote ? Math.max(12, Math.abs(tailY - headY)) : 12
                        visible: y + height >= 0 && y <= playfield.height

                        Rectangle {
                            anchors.fill: parent
                            radius: noteDelegate.longNote ? 3 : 2
                            color: noteDelegate.holding
                                   ? "#7cffbd"
                                   : noteDelegate.noteType === 2
                                     ? "#ff5f68"
                                     : noteDelegate.displayColumn === 0
                                       ? "#ffd85d"
                                       : "#d8f5ff"
                            border.color: "#ffffff"
                            border.width: 1
                            Accessible.ignored: true
                        }
                    }
                }

                Rectangle {
                    x: 0
                    y: playfield.judgementY
                    width: playfield.width
                    height: 3
                    color: root.accentColor
                    Accessible.ignored: true
                }

                Row {
                    x: 0
                    y: playfield.height - 32

                    Repeater {
                        model: [
                            qsTr("SCR"), qsTr("1"), qsTr("2"), qsTr("3"),
                            qsTr("4"), qsTr("5"), qsTr("6"), qsTr("7")
                        ]

                        Label {
                            required property string modelData

                            width: playfield.laneWidth
                            horizontalAlignment: Text.AlignHCenter
                            color: "#98a2b3"
                            font.pixelSize: 11
                            text: modelData
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: webPlaytest.phase === 3
                    color: "#ffffff"
                    font.pixelSize: 72
                    font.bold: true
                    text: Math.max(1, Math.ceil(webPlaytest.countdownSeconds))
                }
            }

            Rectangle {
                Layout.preferredWidth: 270
                Layout.fillHeight: true
                color: root.panelColor
                border.color: root.lineColor
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 11

                    Label {
                        Layout.fillWidth: true
                        color: "#f5f7fb"
                        font.pixelSize: 16
                        font.bold: true
                        text: qsTr("LATENCY")
                    }

                    Label {
                        Layout.fillWidth: true
                        color: root.accentColor
                        font.pixelSize: 26
                        font.bold: true
                        text: webPlaytest.inputLatencyAvailable
                              ? qsTr("%1 ms").arg(webPlaytest.inputLatencyMs.toFixed(2))
                              : qsTr("Waiting for a keysound")
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 8
                        rowSpacing: 8

                        Label { color: "#8d98aa"; text: qsTr("Late clamp") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: qsTr("%1 ms").arg(webPlaytest.lateInputClampMs.toFixed(3))
                        }
                        Label { color: "#8d98aa"; text: qsTr("Late frames") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: webPlaytest.lateFrames
                        }
                        Label { color: "#8d98aa"; text: qsTr("Input drops") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: webPlaytest.droppedInputs > 0 ? "#ff6b72" : "#f5f7fb"
                            text: webPlaytest.droppedInputs
                        }
                        Label { color: "#8d98aa"; text: qsTr("Active voices") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: webPlaytest.activeVoices
                        }
                        Label { color: "#8d98aa"; text: qsTr("Underruns") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: webPlaytest.underrunTelemetryAvailable
                                   && webPlaytest.underruns > 0
                                   ? "#ff6b72" : "#f5f7fb"
                            text: webPlaytest.underrunTelemetryAvailable
                                  ? webPlaytest.underruns : qsTr("n/a")
                        }
                        Label { color: "#8d98aa"; text: qsTr("Heap") }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "#f5f7fb"
                            text: qsTr("%1 MiB")
                                  .arg((webPlaytest.heapBytes / 1048576).toFixed(0))
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: root.lineColor
                    }

                    Label {
                        Layout.fillWidth: true
                        color: "#f5f7fb"
                        font.bold: true
                        text: qsTr("LOAD")
                    }

                    Label {
                        Layout.fillWidth: true
                        color: "#aeb6c4"
                        text: webPlaytest.totalAssets > 0
                              ? qsTr("%1 / %2 keysounds")
                                .arg(webPlaytest.decodedAssets)
                                .arg(webPlaytest.totalAssets)
                              : qsTr("Waiting for AudioContext")
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, webPlaytest.totalAssets)
                        value: webPlaytest.decodedAssets
                    }

                    Item { Layout.fillHeight: true }

                    Label {
                        Layout.fillWidth: true
                        visible: webPlaytest.terminalError.length > 0
                        wrapMode: Text.Wrap
                        color: "#ff6b72"
                        text: webPlaytest.terminalError
                    }
                }
            }
        }
    }
}
