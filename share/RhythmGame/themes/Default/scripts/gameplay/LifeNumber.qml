import QtQuick
import "../common"

Text {
    id: lifeText

    required property var score
    property string fontFile: "file:NotoSansJP-VariableFont_wght.ttf"

    ThemeFont {
        id: lifeNumberFont
        fileName: lifeText.fontFile
    }

    function getLifeText() {
        let gauges = lifeText.score.gauges;
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
    font.family: lifeNumberFont.fontFamily
    font.weight: lifeNumberFont.fontWeight
    font.italic: lifeNumberFont.italic
    font.pixelSize: 32
    fontSizeMode: Text.Fit
    text: lifeText.getLifeText()
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
}
