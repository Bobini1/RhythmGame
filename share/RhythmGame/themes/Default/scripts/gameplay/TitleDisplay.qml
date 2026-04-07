import QtQuick

Item {
    id: titleDisplay

    property string title: ""
    property string subtitle: ""
    property bool contentVisible: true

    Column {
        anchors.fill: parent
        spacing: 2
        visible: titleDisplay.contentVisible

        // Title
        Text {
            width: parent.width
            height: titleDisplay.subtitle !== "" ? parent.height * 0.58 : parent.height
            text: titleDisplay.title
            font.pixelSize: height * 0.85
            fontSizeMode: Text.Fit
            minimumPixelSize: 6
            color: "white"
            font.bold: true
            elide: Text.ElideRight
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        // Subtitle
        Text {
            width: parent.width
            height: parent.height * 0.38
            text: titleDisplay.subtitle
            font.pixelSize: height * 0.8
            fontSizeMode: Text.Fit
            minimumPixelSize: 6
            color: "#cccccc"
            elide: Text.ElideRight
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            visible: titleDisplay.subtitle !== ""
        }
    }
}



