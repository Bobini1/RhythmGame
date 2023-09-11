import QtQuick

Item {
    height: childrenRect.height
    width: childrenRect.width

    Text {
        id: lifeText

        function getLifeText() {
            let gauges = chart.score.gauges;
            for (let gauge of gauges) {
                if (gauge.gauge > gauge.threshold) {
                    return gauge.gauge.toFixed(1) + "%";
                }
            }
            return gauges[gauges.length - 1].gauge.toFixed(1) + "%";
        }

        anchors.right: parent.right
        color: "white"
        font.pixelSize: 24
        text: lifeText.getLifeText()

        Connections {
            function onJudgementCountsChanged() {
                lifeText.text = lifeText.getLifeText();
            }

            target: chart.score
        }
    }
}