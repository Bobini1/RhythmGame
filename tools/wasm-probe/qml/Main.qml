import QtQuick
import QtMultimedia
import QtQuick.Controls.Basic

Window {
    id: root

    property real animatedPhase: 0.0

    width: 640
    height: 360
    visible: true
    color: "#101216"
    title: qsTr("RhythmGame Wasm Gate 1B")

    ShaderEffect {
        id: shader

        anchors.fill: parent
        property real phase: probeState.phaseFrozen
                             ? probeState.shaderPhase
                             : root.animatedPhase
        fragmentShader: "qrc:/qt/qml/RhythmGame/WasmProbe/shaders/pulse.frag.qsb"
    }

    NumberAnimation {
        target: root
        property: "animatedPhase"
        from: 0.0
        to: 1.0
        duration: 1000
        loops: Animation.Infinite
        running: root.visible && !probeState.phaseFrozen
    }

    Rectangle {
        id: mediaFrame

        x: 24
        y: 24
        width: 240
        height: 135
        color: "black"

        VideoOutput {
            id: videoOutput

            objectName: "gate1bMediaVideoOutput"
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
        }
    }

    Text {
        anchors.centerIn: parent
        color: "white"
        text: probeState.exceptionPassed && probeState.threadPassed
              ? qsTr("Gate 1B core ready")
              : qsTr("Gate 1B core starting")
    }

    Button {
        x: 24
        y: 296
        width: 200
        height: 40
        text: qsTr("Start browser probes")
        onClicked: probeState.beginUserActivatedProbes()
    }

    Component.onCompleted: {
        probeState.attachMediaVideoSink(videoOutput.videoSink)
    }
}
