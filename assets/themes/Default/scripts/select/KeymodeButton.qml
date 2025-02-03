import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    property int current: 0
    property var options: ProfileList.battleActive ? [7] : [7, 14, null]
    onOptionsChanged: {
        if (current >= options.length) {
            current = 0;
        }
        mouseArea.setFilter();
    }
    enabled: options.length > 1


    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: (keymodeButton.options[keymodeButton.current] ? keymodeButton.options[keymodeButton.current] : "ALL") + " keys"
    }
    MouseArea {
        id: mouseArea
        function setFilter() {
            let currentKeymode = keymodeButton.options[keymodeButton.current];
            if (currentKeymode) {
                songList.filter = function (chart) {
                    return chart.keymode === currentKeymode;
                };
            } else {
                songList.filter = null;
            }
        }

        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor

        Component.onCompleted: {
            setFilter();
        }
        onClicked: {
            keymodeButton.current = (keymodeButton.current + 1) % keymodeButton.options.length;
            setFilter();
        }
    }
}