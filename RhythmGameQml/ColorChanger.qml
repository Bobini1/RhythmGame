import QtQuick 2.15
import QtMultimedia 5.15

ShaderEffect {
    property color from: "black"
    property variant source
    property color to: "transparent"
    property real tolerance: 0.03125

    fragmentShader: "qrc:/ColorChanger.frag.qsb"
}