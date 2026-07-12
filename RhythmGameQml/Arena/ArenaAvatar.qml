import QtQuick

Rectangle {
    id: root

    required property string displayName
    property url avatarUrl: ""
    property bool connected: true
    readonly property string initial: {
        const name = root.displayName.trim();
        return name.length > 0 ? name.charAt(0).toUpperCase() : "?";
    }

    Accessible.ignored: true
    border.color: root.connected ? "#7f93aa" : "#b06f6a"
    border.width: 1
    clip: true
    color: "#283444"
    radius: Math.min(width, height) / 2

    Image {
        id: avatarImage

        Accessible.ignored: true
        anchors.fill: parent
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        source: root.avatarUrl
        sourceSize.height: Math.max(1, Math.ceil(height))
        sourceSize.width: Math.max(1, Math.ceil(width))
        visible: status === Image.Ready
    }

    Text {
        objectName: "arenaAvatarFallback"
        Accessible.ignored: true
        anchors.centerIn: parent
        color: "white"
        font.bold: true
        font.pixelSize: Math.max(10, Math.floor(Math.min(root.width, root.height) * 0.48))
        text: root.initial
        textFormat: Text.PlainText
        visible: avatarImage.status !== Image.Ready
    }
}
