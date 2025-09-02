import QtQml
import QtQuick
import QtQuick.Layouts
import RhythmGameQml

Item {
    id: gaugeRoot
    required property string gaugeImage
    required property var gauges
    Repeater {
        model: gaugeRoot.gauges

        Item {
            id: gauge

            readonly property double gauge: modelData.gauge
            readonly property string gaugeName: modelData.name
            readonly property double threshold: modelData.threshold
            readonly property double gaugeMax: modelData.gaugeMax

            function getSource(index) {
                let above = index >= gauge.threshold / 2;
                let img = root.iniImagesUrl + "gauge/" + gaugeRoot.gaugeImage + "/";
                switch (gauge.gaugeName) {
                case "FC":
                    return img + "orange";
                case "EXHARDDAN":
                case "EXDAN":
                case "EXHARD":
                    return img + "yellow";
                case "DAN":
                case "HARD":
                    return img + "red";
                case "NORMAL":
                    return img + (above ? "red" : "blue");
                case "EASY":
                    return img + (above ? "red" : "green");
                case "AEASY":
                    return img + (above ? "red" : "purple");
                default:
                    return img + (above ? "red" : "purple");
                }
            }

            anchors.fill: parent
            visible: (index === gaugeRoot.gauges.length - 1) || (modelData.gauge > threshold)
            z: gaugeRoot.gauges.length - index - 1

            Timer {
                id: blinking
                property real random
                running: true
                repeat: true
                interval: 33
                onTriggered: {
                    random = Math.random();
                }
            }
            Row {
                z: 1
                anchors.fill: parent

                Repeater {
                    id: fullChunks
                    model: Math.floor(gauge.gaugeMax / 2)

                    Image {
                        id: emptyChunk
                        width: parent.width / (gauge.gaugeMax / 2)
                        height: parent.height

                        source: gauge.getSource(index) + "_empty"

                        Image {
                            id: fullChunk
                            width: parent.width
                            height: parent.height
                            visible: {
                                let count = Math.floor(gauge.gauge / 2)
                                if (index >= count) {
                                    return false;
                                }
                                if (index === count - 4) {
                                    return blinking.random > 0.25;
                                }
                                if (index === count - 3) {
                                    return blinking.random > 0.5
                                }
                                if (index === count - 2) {
                                    return blinking.random > 0.75;
                                }
                                return true;
                            }

                            source: gauge.getSource(index)
                        }
                    }

                }
            }
        }
    }
}
