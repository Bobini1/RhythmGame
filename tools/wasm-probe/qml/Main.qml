import QtQuick
import QtMultimedia

Window {
    id: root
    width: 640
    height: 360
    visible: true
    color: "#101216"
    title: "RhythmGame Wasm Gate 1A"

    MediaPlayer {
        id: mediaProbe
    }

    ShaderEffect {
        id: shader
        anchors.fill: parent
        property real phase: 0.0
        fragmentShader: "qrc:/qt/qml/RhythmGame/WasmProbe/shaders/pulse.frag.qsb"

        NumberAnimation on phase {
            from: 0.0
            to: 1.0
            duration: 1000
            loops: Animation.Infinite
        }
    }

    Text {
        anchors.centerIn: parent
        color: "white"
        text: probeState.exceptionPassed && probeState.threadPassed
              ? "gate-1a-ready"
              : "gate-1a-starting"
    }
}
