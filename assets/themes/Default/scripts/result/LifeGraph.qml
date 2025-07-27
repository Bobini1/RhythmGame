pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

Image {
    id: lifeGraph

    required property var clearType
    required property var gaugeHistory
    required property var gaugeInfo
    required property var chartData

    property int clearIndex: {
        let index = clearTypes.indexOf(lifeGraph.clearType);
        if (index === -1) {
            if (lifeGraph.clearType === "FAILED") {
                return 0;
            } else {
                return clearTypes.indexOf("FC");
            }
        } else {
            return index;
        }
    }
    property var clearTypes: Object.keys(lifeGraph.gaugeHistory)
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

        Repeater {
            model: lifeGraph.clearTypes

            LifePath {
                required property int index
                required property string modelData

                anchors.bottomMargin: 20
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                anchors.topMargin: 20
                history: lifeGraph.gaugeHistory[modelData]
                maxGauge: lifeGraph.gaugeInfo[modelData].maxGauge
                songLength: lifeGraph.chartData.length
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
                        let secondary = lifeGraph.secondaryColor[modelData];
                        if (secondary) {
                            return secondary;
                        }
                        return lifeGraph.primaryColor[modelData];
                    }
                    height: (1 - (lifeGraph.gaugeInfo[modelData].threshold / lifeGraph.gaugeInfo[modelData].maxGauge)) * parent.height
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
                    color: lifeGraph.primaryColor[modelData]
                    opacity: 0.5
                    z: -1
                }
            }
        }
    }
}
