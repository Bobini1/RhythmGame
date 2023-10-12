import QtQml
import QtQuick

Item {
    height: childrenRect.height
    width: childrenRect.width

    Repeater {
        model: chart.score.gauges

        Item {
            id: gauge

            property double gauge: modelData.gauge
            property string gaugeName: modelData.objectName
            property double threshold: modelData.threshold

            function getSource(index) {
                let above = index >= gauge.threshold / 2;
                let img = root.iniImagesUrl + "gauge.png/";
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

            height: childrenRect.height
            visible: (index === chart.score.gauges.length - 1) || (modelData.gauge > threshold)
            width: childrenRect.width
            z: (chart.score.gauges.length - index - 1) * 2

            Row {
                z: parent.z

                Repeater {
                    model: Math.floor(modelData.gaugeMax / 2)

                    Image {
                        id: emptyChunk

                        source: gauge.getSource(index) + "_empty"
                    }
                }
            }
            Row {
                z: parent.z + 1

                Repeater {
                    model: Math.floor(modelData.gauge / 2)

                    Image {
                        id: fullChunk

                        source: gauge.getSource(index)
                        visible: index < gauge.gauge / 2
                    }
                }
            }
        }
    }
}
