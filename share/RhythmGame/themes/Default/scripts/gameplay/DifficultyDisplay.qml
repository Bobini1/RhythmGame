import QtQuick

Item {
    id: difficultyDisplay

    property int difficulty: 0
    property int playLevel: 0
    property bool contentVisible: true

    readonly property var difficultyNames: ["INSANE", "BEGINNER", "NORMAL", "HYPER", "ANOTHER", "INSANE"]
    readonly property string difficultyName: difficulty >= 1 && difficulty <= 5
        ? difficultyNames[difficulty]
        : (difficulty > 0 ? String(difficulty) : "INSANE")
    readonly property var difficultyColors: ["#888888", "#44CC44", "#44AAFF", "#FF9900", "#FF3300", "#9900FF"]
    readonly property color difficultyColor: difficulty >= 0 && difficulty <= 5
        ? difficultyColors[difficulty]
        : "#888888"

    // Invisible sizer at a fixed reference size to capture the badge's natural proportions.
    Text {
        id: diffSizer
        visible: false
        text: difficultyDisplay.difficultyName
        font.pixelSize: 100
        font.bold: true
    }

    // Natural badge dimensions at the 100 px reference font.
    readonly property real naturalBadgeWidth:  Math.max(1, diffSizer.implicitWidth  + 24)
    readonly property real naturalBadgeHeight: Math.max(1, diffSizer.implicitHeight + 12)

    // Uniform scale = min of two constraints:
    //  1. fit inside container width
    //  2. occupy at most 40 % of container height  (leaves room for playlevel;
    //     with playlevel = 1.5× badge this ensures they never overlap)
    readonly property real badgeScale: Math.min(
        width  / naturalBadgeWidth,
        height * 0.4 / naturalBadgeHeight
    )
    readonly property real badgeActualWidth:  naturalBadgeWidth  * badgeScale
    readonly property real badgeActualHeight: naturalBadgeHeight * badgeScale
    readonly property real badgeFontSize:     100 * badgeScale

    // ── Difficulty badge ── anchored to top; grows until it fills the container
    // width, after which extra height only spreads the two elements apart.
    Rectangle {
        id: diffBadge
        visible: difficultyDisplay.contentVisible
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        width:  difficultyDisplay.badgeActualWidth
        height: difficultyDisplay.badgeActualHeight
        color:  difficultyDisplay.difficultyColor
        radius: height * 0.18

        Text {
            anchors.fill: parent
            text: difficultyDisplay.difficultyName
            font.pixelSize: Math.max(1, difficultyDisplay.badgeFontSize)
            font.bold: true
            color: "white"
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // ── Play level ── sized proportional to the badge; anchored to bottom so
    // the gap between the two elements grows as the container gets taller.
    Text {
        anchors.bottom: parent.bottom
        visible: difficultyDisplay.contentVisible
        width: parent.width
        height: difficultyDisplay.badgeActualHeight * 1.5
        text: "Lv." + difficultyDisplay.playLevel
        font.pixelSize: Math.max(1, difficultyDisplay.badgeFontSize * 1.3)
        font.bold: true
        color: "white"
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
    }
}

