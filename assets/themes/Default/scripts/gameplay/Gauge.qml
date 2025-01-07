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
            readonly property string gaugeName: modelData.objectName
            readonly property double threshold: modelData.threshold
            readonly property double gaugeMax: modelData.gaugeMax

            function getSource(index) {
                let above = index >= gauge.threshold / 2;
                let img = root.iniImagesUrl + "gauge/" + gaugeRoot.gaugeImage + "/";
                switch (gauge.gaugeName) {
                case "FC":
                    return img + "orange";
                case "EXHARD":
                    return img + "yellow";
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

            Row {
                z: 0
                anchors.fill: parent

                Repeater {
                    model: Math.floor(gauge.gaugeMax / 2)

                    Image {
                        id: emptyChunk
                        width: parent.width / (gauge.gaugeMax / 2)
                        height: parent.height

                        source: gauge.getSource(index) + "_empty"
                    }
                }
            }
            Row {
                z: 1
                anchors.fill: parent

                Repeater {
                    model: Math.floor(gauge.gauge / 2)

                    Image {
                        id: fullChunk
                        width: parent.width / (gauge.gaugeMax / 2)
                        height: parent.height

                        source: gauge.getSource(index)
                        visible: index < gauge.gauge / 2
                    }
                }
            }
        }
    }
}
