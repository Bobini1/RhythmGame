import QtQuick 2.15
import QtMultimedia 5.15

ShaderEffect {
    property color from: "black"
    property variant source
    property color to: "transparent"
    property double tolerance: 0.10

    fragmentShader: "qrc:/ColorChanger.frag.qsb"
}