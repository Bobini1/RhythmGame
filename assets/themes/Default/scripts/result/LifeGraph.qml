pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

Image {
    id: lifeGraph

    required property var clearType
    required property var gaugeInfo
    required property var length
    required property var lengths

    property int clearIndex: {
        let index = clearTypes.indexOf(lifeGraph.clearType);
        if (index === -1) {
            if (lifeGraph.clearType === "FAILED" || lifeGraph.clearType === "NOPLAY") {
                return 0;
            } else {
                return clearTypes.indexOf("FC");
            }
        } else {
            return index;
        }
    }
    property var clearTypes: lifeGraph.gaugeInfo.map(info => info.name).reverse()
    property var primaryColor: {
        return {
            "AEASY": "#d844ff",
            "EASY": "#6eff3a",
            "NORMAL": "#3affff",
            "HARD": "#ff3a3a",
            "DAN": "#ff3a3a",
            "EXHARD": "#ff9a3a",
            "EXDAN": "#ff9a3a",
            "FC": "#cbcbcb",
            "EXHARDDAN": "#cbcbcb",
            "PERFECT": "#cbcbcb",
            "MAX": "#cbcbcb"
        };
    }
    property var secondaryColor: {
        return {
            "AEASY": "#6eff3a",
            "EASY": "#3affff",
            "NORMAL": "#ff3a3a"
        };
    }

    function decrementIndex() {
        if (clearIndex === 0) {
            clearIndex = clearTypes.length - 1;
        } else {
            clearIndex--;
        }
    }
    function incrementIndex() {
        if (clearIndex === clearTypes.length - 1) {
            clearIndex = 0;
        } else {
            clearIndex++;
        }
    }

    source: root.iniImagesUrl + "parts.png/life_graph_frame"

    Item {
        anchors.fill: parent
        z: -1

        Item {
            anchors.margins: 20
            anchors.fill: parent
            Repeater {
                property var cumulativeLengths: lifeGraph.lengths.reduce((acc, val) => {
                    if (acc.length === 0) {
                        acc.push(val);
                    } else {
                        acc.push(acc[acc.length - 1] + val);
                    }
                    return acc;
                }, []);
                model: cumulativeLengths.slice(0, cumulativeLengths.length - 1)
                // draw a vertical line for each length
                Rectangle {
                    required property string modelData
                    x: modelData * (parent.width / lifeGraph.length)
                    width: 2
                    height: parent.height
                    color: "red"
                }
            }
        }

        Repeater {
            model: lifeGraph.gaugeInfo.slice().reverse()

            LifePath {
                required property int index
                required property var modelData

                anchors.margins: 20
                anchors.fill: parent
                history: modelData.gaugeHistory
                maxGauge: modelData.maxGauge
                songLength: lifeGraph.length
                visible: index === lifeGraph.clearIndex

                Rectangle {
                    id: topRect

                    anchors.left: parent.left
                    anchors.leftMargin: -2
                    anchors.right: parent.right
                    anchors.rightMargin: -2
                    anchors.top: parent.top
                    anchors.topMargin: -2
                    color: {
                        let secondary = lifeGraph.secondaryColor[modelData.name];
                        if (secondary) {
                            return secondary;
                        }
                        return lifeGraph.primaryColor[modelData.name];
                    }
                    height: (1 - (modelData.threshold / modelData.maxGauge)) * parent.height
                    opacity: 0.5
                    z: -1
                }
                Rectangle {
                    id: bottomRect

                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: -2
                    anchors.left: parent.left
                    anchors.leftMargin: -2
                    anchors.right: parent.right
                    anchors.rightMargin: -2
                    anchors.top: topRect.bottom
                    color: lifeGraph.primaryColor[modelData.name]
                    opacity: 0.5
                    z: -1
                }
            }
        }
    }
}
