import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    property int current: 1
    property var options: ProfileList.battleActive ? [null, 7] : [null, 7, 14]
    onOptionsChanged: {
        mouseArea.setFilter();
    }

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

        Component.onCompleted: {
            setFilter();
        }
        onClicked: {
            keymodeButton.current = (keymodeButton.current + 1) % keymodeButton.options.length;
            setFilter();
        }
    }
}