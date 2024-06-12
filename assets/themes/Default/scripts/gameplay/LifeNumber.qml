import QtQuick

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

    TextMetrics {
        id: textMetrics
        text: "100%"
        font: lifeText.font
    }
    readonly property real hundredPercentWidth: textMetrics.width
    color: "white"
    font.pixelSize: 24
    fontSizeMode: Text.Fit
    text: lifeText.getLifeText()
    font.pointSize: Infinity
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter

    Connections {
        function onJudgementCountsChanged() {
            lifeText.text = lifeText.getLifeText();
        }

        target: chart.score
    }
}
